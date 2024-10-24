/*
 * Copyright 2021 Alibaba Group Holding Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdio.h>

#include <iostream>
#include <vector>

#if defined(__ANDROID__) || defined(__linux__)
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifndef __ANDRIOD__
#include <iconv.h>
#endif
#endif

#ifdef __GNUC__
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#endif

#include "Config.h"
#include "connectNode.h"
#include "iNlsRequest.h"
#include "iNlsRequestParam.h"
#include "nlog.h"
#include "nlsClientImpl.h"
#include "nlsGlobal.h"
#include "nodeManager.h"
#include "text_utils.h"
#include "utility.h"
#include "workThread.h"
#ifdef ENABLE_REQUEST_RECORDING
#include "json/json.h"
#include "text_utils.h"
#endif

namespace AlibabaNls {

ConnectNode::ConnectNode(INlsRequest *request,
                         HandleBaseOneParamWithReturnVoid<NlsEvent> *handler,
                         bool isLongConnection)
    : _request(request),
      _handler(handler),
      _isLongConnection(isLongConnection),
      _dnsRequest(NULL),
      _dnsRequestCallbackStatus(0),
      _retryConnectCount(0),
      _socketFd(INVALID_SOCKET),
      _binaryEvBuffer(NULL),
      _readEvBuffer(NULL),
      _cmdEvBuffer(NULL),
      _wwvEvBuffer(NULL),
      _nlsEncoder(NULL),
      _encoderType(ENCODER_NONE),
      _audioFrame(NULL),
      _audioFrameSize(0),
      _maxFrameSize(0),
      _isFirstAudioFrame(true),
      _eventThread(NULL),
      _isDestroy(false),
      _isWakeStop(false),
      _isStop(false),
      _isFirstBinaryFrame(true),
      _isConnected(false),
      _workStatus(NodeCreated),
      _exitStatus(ExitInvalid),
      _instance(NULL),
      _syncCallTimeoutMs(0),
#ifdef __LINUX__
      _nodename(NULL),
      _servname(NULL),
      _dnsThread(0),
      _dnsThreadExit(false),
      _dnsErrorCode(0),
      _addrinfo(NULL),
      _dnsThreadRunning(false),
      _dnsEvent(NULL),
#endif
      _sslHandle(NULL),
      _enableRecvTv(false),
      _enableOnMessage(false),
      _launchEvent(NULL),
      _connectEvent(NULL),
      _readEvent(NULL),
      _writeEvent(NULL),
#ifdef ENABLE_CONTINUED
      _reconnectEvent(NULL),
#endif
      _inEventCallbackNode(false),
      _releasingFlag(false),
      _waitEventCallbackAbnormally(false) {
  _binaryEvBuffer = evbuffer_new();
  if (_binaryEvBuffer == NULL) {
    LOG_ERROR("_binaryEvBuffer is nullptr");
  }
  evbuffer_enable_locking(_binaryEvBuffer, NULL);

  _readEvBuffer = evbuffer_new();
  if (_readEvBuffer == NULL) {
    LOG_ERROR("_readEvBuffer is nullptr");
  }
  _cmdEvBuffer = evbuffer_new();
  if (_cmdEvBuffer == NULL) {
    LOG_ERROR("_cmdEvBuffer is nullptr");
  }
  _wwvEvBuffer = evbuffer_new();
  if (_wwvEvBuffer == NULL) {
    LOG_ERROR("_wwvEvBuffer is nullptr");
  }

  evbuffer_enable_locking(_readEvBuffer, NULL);
  evbuffer_enable_locking(_cmdEvBuffer, NULL);
  evbuffer_enable_locking(_wwvEvBuffer, NULL);

  _sslHandle = new SSLconnect();
  if (_sslHandle == NULL) {
    LOG_ERROR("Node(%p) _sslHandle is nullptr.", this);
  } else {
    LOG_DEBUG("Node(%p) new SSLconnect:%p.", this, _sslHandle);
  }

  LOG_INFO("Node(%p) create ConnectNode include webSocketTcp:%p.", this,
           &_webSocket);

  // will update parameters in updateParameters()
  _enableRecvTv = request->getRequestParam()->getEnableRecvTimeout();
  utility::TextUtils::GetTimevalFromMs(
      &_recvTv, request->getRequestParam()->getRecvTimeout());
  utility::TextUtils::GetTimevalFromMs(
      &_sendTv, request->getRequestParam()->getSendTimeout());
  utility::TextUtils::GetTimevalFromMs(
      &_connectTv, request->getRequestParam()->getTimeout());

  _enableOnMessage = request->getRequestParam()->getEnableOnMessage();

#ifdef ENABLE_HIGH_EFFICIENCY
  _connectTimerTv.tv_sec = 0;
  _connectTimerTv.tv_usec = ConnectTimerIntervalMs * 1000;
  _connectTimerFlag = true;
  _connectTimerEvent = NULL;
#endif

#ifdef __LINUX__
  _gaicbRequest[0] = NULL;
#endif

#if defined(_MSC_VER)
  _mtxNode = CreateMutex(NULL, FALSE, NULL);
  _mtxCloseNode = CreateMutex(NULL, FALSE, NULL);
  _mtxEventCallbackNode = CreateEvent(NULL, FALSE, FALSE, NULL);
  _mtxInvokeSyncCallNode = CreateEvent(NULL, FALSE, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxNode, NULL);
  pthread_mutex_init(&_mtxCloseNode, NULL);
  pthread_mutex_init(&_mtxEventCallbackNode, NULL);
  pthread_mutex_init(&_mtxInvokeSyncCallNode, NULL);
  pthread_cond_init(&_cvEventCallbackNode, NULL);
  pthread_cond_init(&_cvInvokeSyncCallNode, NULL);
#endif

#ifdef ENABLE_REQUEST_RECORDING
  _nodeProcess.last_status = NodeCreated;
  _nodeProcess.create_timestamp_ms = utility::TextUtils::GetTimestampMs();
  _nodeProcess.last_op_timestamp_ms = _nodeProcess.create_timestamp_ms;
#endif

  _nodeUUID = utility::TextUtils::getRandomUuid();

  LOG_INFO(
      "Node(%p) create ConnectNode done with long connection flag:%s, the UUID "
      "is %s",
      this, _isLongConnection ? "True" : "False", _nodeUUID.c_str());
}

ConnectNode::~ConnectNode() {
  LOG_DEBUG("Node(%p) destroy ConnectNode begin.", this);

#ifdef __LINUX__
  if (_url._enableSysGetAddr) {
    if (_dnsThread) {
      LOG_WARN("Node(%p) dnsThread(%lu) still exist, waiting exiting", this,
               _dnsThread);
      _dnsThreadExit = true;
      pthread_join(_dnsThread, NULL);
      LOG_WARN("Node(%p) dnsThread(%lu) exited.", this, _dnsThread);
      _dnsThread = 0;
      if (_gaicbRequest[0]) {
        free(_gaicbRequest[0]);
        _gaicbRequest[0] = NULL;
      }
    } else {
      LOG_DEBUG("Node(%p) dnsThread has exited.", this);
    }
  }
#endif

  waitEventCallback();
  closeConnectNode();
  if (_eventThread) {
    _eventThread->freeListNode(_eventThread, _request);
  }
  _request = NULL;

  if (_sslHandle) {
    LOG_INFO("Node(%p) delete _sslHandle:%p.", this, _sslHandle);
    delete _sslHandle;
    _sslHandle = NULL;
  }

  _handler = NULL;

  if (_cmdEvBuffer) {
    evbuffer_free(_cmdEvBuffer);
    _cmdEvBuffer = NULL;
  }
  if (_readEvBuffer) {
    evbuffer_free(_readEvBuffer);
    _readEvBuffer = NULL;
  }
  if (_binaryEvBuffer) {
    evbuffer_free(_binaryEvBuffer);
    _binaryEvBuffer = NULL;
  }
  if (_wwvEvBuffer) {
    evbuffer_free(_wwvEvBuffer);
    _wwvEvBuffer = NULL;
  }
  if (_launchEvent) {
    event_free(_launchEvent);
    _launchEvent = NULL;
  }
#ifdef ENABLE_CONTINUED
  if (_reconnectEvent) {
    event_free(_reconnectEvent);
    _reconnectEvent = NULL;
  }
#endif

  if (_eventThread) {
    if (_dnsRequest && _dnsRequestCallbackStatus == 1) {
      LOG_DEBUG(
          "Node(%p) cancel _dnsRequest(%p), current "
          "event_count_active_added_virtual:%d",
          this, _dnsRequest,
          event_base_get_num_events(_eventThread->_workBase,
                                    EVENT_BASE_COUNT_ACTIVE |
                                        EVENT_BASE_COUNT_ADDED |
                                        EVENT_BASE_COUNT_VIRTUAL));
      LOG_DEBUG(
          "Node(%p) cancel _dnsRequest(%p), current event_count_active:%d",
          this, _dnsRequest,
          event_base_get_num_events(_eventThread->_workBase,
                                    EVENT_BASE_COUNT_ACTIVE));
      LOG_DEBUG(
          "Node(%p) cancel _dnsRequest(%p), current event_count_virtual:%d",
          this, _dnsRequest,
          event_base_get_num_events(_eventThread->_workBase,
                                    EVENT_BASE_COUNT_VIRTUAL));
      LOG_DEBUG("Node(%p) cancel _dnsRequest(%p), current event_count_added:%d",
                this, _dnsRequest,
                event_base_get_num_events(_eventThread->_workBase,
                                          EVENT_BASE_COUNT_ADDED));

      if (_waitEventCallbackAbnormally && _workStatus == NodeConnecting) {
        LOG_WARN(
            "Node(%p) is in an exception and NodeConnecting, skipping "
            "evdns_getaddrinfo_cancel.");
      } else {
        evdns_getaddrinfo_cancel(_dnsRequest);
      }
    }
    LOG_DEBUG(
        "Node(%p) get event_count_active_added_virtual %d in deconstructing "
        "ConnectNode.",
        this,
        event_base_get_num_events(_eventThread->_workBase,
                                  EVENT_BASE_COUNT_ACTIVE |
                                      EVENT_BASE_COUNT_ADDED |
                                      EVENT_BASE_COUNT_VIRTUAL));
    LOG_DEBUG(
        "Node(%p) get event_count_active %d in deconstructing ConnectNode.",
        this,
        event_base_get_num_events(_eventThread->_workBase,
                                  EVENT_BASE_COUNT_ACTIVE));
    LOG_DEBUG(
        "Node(%p) get event_count_virtual %d in deconstructing ConnectNode.",
        this,
        event_base_get_num_events(_eventThread->_workBase,
                                  EVENT_BASE_COUNT_VIRTUAL));
    LOG_DEBUG(
        "Node(%p) get event_count_added %d in deconstructing ConnectNode.",
        this,
        event_base_get_num_events(_eventThread->_workBase,
                                  EVENT_BASE_COUNT_ADDED));
  }
  _eventThread = NULL;

  if (_nlsEncoder) {
    _nlsEncoder->destroyNlsEncoder();
    delete _nlsEncoder;
    _nlsEncoder = NULL;
  }
  if (_audioFrame) {
    free(_audioFrame);
    _audioFrame = NULL;
  }
  _audioFrameSize = 0;
  _maxFrameSize = 0;
  _isFirstAudioFrame = true;

#if defined(_MSC_VER)
  CloseHandle(_mtxNode);
  CloseHandle(_mtxCloseNode);
  CloseHandle(_mtxEventCallbackNode);
  CloseHandle(_mtxInvokeSyncCallNode);
#else
  pthread_mutex_destroy(&_mtxNode);
  pthread_mutex_destroy(&_mtxCloseNode);
  pthread_mutex_destroy(&_mtxEventCallbackNode);
  pthread_mutex_destroy(&_mtxInvokeSyncCallNode);
  pthread_cond_destroy(&_cvEventCallbackNode);
  pthread_cond_destroy(&_cvInvokeSyncCallNode);
#endif
  _inEventCallbackNode = false;

  _instance = NULL;

  LOG_DEBUG("Node(%p) destroy ConnectNode done.", this);
}

/**
 * @brief: 长链接模式下完成一轮交互初始化参数而非释放
 * @return:
 */
void ConnectNode::initAllStatus() {
  MUTEX_LOCK(_mtxNode);

  _isFirstBinaryFrame = true;
  _isStop = false;
  _isDestroy = false;
  _isWakeStop = false;

  _workStatus = NodeCreated;
  _exitStatus = ExitInvalid;

  MUTEX_UNLOCK(_mtxNode);
}

/**
 * @brief: 获得用于启动当前node的libevent, 用于启动当前请求
 * @return: libevent的event指针
 */
struct event *ConnectNode::getLaunchEvent(bool init) {
  if (_launchEvent == NULL) {
    _launchEvent = event_new(_eventThread->_workBase, -1, EV_READ,
                             WorkThread::launchEventCallback, this);
    if (NULL == _launchEvent) {
      LOG_ERROR("Node(%p) new event(_launchEvent) failed.", this);
    } else {
      LOG_DEBUG("Node(%p) new event(_launchEvent).", this);
    }
  } else {
    if (init) {
      event_del(_launchEvent);
      int assign_ret =
          event_assign(_launchEvent, _eventThread->_workBase, -1, EV_READ,
                       WorkThread::launchEventCallback, this);
      LOG_DEBUG("Node(%p) new event_assign(_launchEvent) with ret:%d.", this,
                assign_ret);
    }
  }
  return _launchEvent;
}

/**
 * @brief: 获得当前node的运行状态
 * @return: 当前node的运行状态枚举值
 */
ConnectStatus ConnectNode::getConnectNodeStatus() {
  MUTEX_LOCK(_mtxNode);
  ConnectStatus status = _workStatus;
  MUTEX_UNLOCK(_mtxNode);
  return status;
}

/**
 * @brief: 获得当前node的运行状态
 * @return: 当前node的运行状态枚举值对应字符串
 */
std::string ConnectNode::getConnectNodeStatusString() {
  MUTEX_LOCK(_mtxNode);
  std::string ret_str = getConnectNodeStatusString(_workStatus);
  MUTEX_UNLOCK(_mtxNode);
  return ret_str;
}

std::string ConnectNode::getConnectNodeStatusString(ConnectStatus status) {
  std::string ret_str("Unknown");
  switch (status) {
    case NodeInvalid:
      ret_str.assign("NodeInvalid");
      break;
    case NodeCreated:
      ret_str.assign("NodeCreated");
      break;
    case NodeInvoking:
      ret_str.assign("NodeInvoking");
      break;
    case NodeInvoked:
      ret_str.assign("NodeInvoked");
      break;
    case NodeConnecting:
      ret_str.assign("NodeConnecting");
      break;
    case NodeConnected:
      ret_str.assign("NodeConnected");
      break;
    case NodeHandshaking:
      ret_str.assign("NodeHandshaking");
      break;
    case NodeHandshaked:
      ret_str.assign("NodeHandshaked");
      break;
    case NodeStarting:
      ret_str.assign("NodeStarting");
      break;
    case NodeStarted:
      ret_str.assign("NodeStarted");
      break;
    case NodeWakeWording:
      ret_str.assign("NodeWakeWording");
      break;
    case NodeFailed:
      ret_str.assign("NodeFailed");
      break;
    case NodeCompleted:
      ret_str.assign("NodeCompleted");
      break;
    case NodeClosed:
      ret_str.assign("NodeClosed");
      break;
    case NodeReleased:
      ret_str.assign("NodeReleased");
      break;
    case NodeStop:
      ret_str.assign("NodeStop");
      break;
    case NodeCancel:
      ret_str.assign("NodeCancel");
      break;
    case NodeSendAudio:
      ret_str.assign("NodeSendAudio");
      break;
    case NodeSendControl:
      ret_str.assign("NodeSendControl");
      break;
    case NodePlayAudio:
      ret_str.assign("NodePlayAudio");
      break;
    case NodeSendText:
      ret_str.assign("NodeSendText");
      break;
    default:
      LOG_ERROR("Current invalid node status:%d.", status);
  }
  return ret_str;
}

/**
 * @brief: 设置当前node的运行状态
 * @return:
 */
void ConnectNode::setConnectNodeStatus(ConnectStatus status) {
  MUTEX_LOCK(_mtxNode);
  _workStatus = status;
#ifdef ENABLE_REQUEST_RECORDING
  _nodeProcess.last_status = _workStatus;
#endif
  MUTEX_UNLOCK(_mtxNode);
}

/**
 * @brief: 获得当前node的退出状态
 * @return:
 */
ExitStatus ConnectNode::getExitStatus() {
  MUTEX_LOCK(_mtxNode);
  ExitStatus ret = _exitStatus;
  MUTEX_UNLOCK(_mtxNode);
  return ret;
}

/**
 * @brief: 获得当前node的退出状态
 * @return: 当前node的退出状态枚举值对应字符串
 */
std::string ConnectNode::getExitStatusString() {
  MUTEX_LOCK(_mtxNode);

  std::string ret_str = "Unknown";
  switch (_exitStatus) {
    case ExitInvalid:
      ret_str.assign("ExitInvalid");
      break;
    case ExitStopping:
      ret_str.assign("ExitStopping");
      break;
    case ExitCancel:
      ret_str.assign("ExitCancel");
      break;
    default:
      LOG_ERROR("Current invalid exit status:%d.", _exitStatus);
  }

  MUTEX_UNLOCK(_mtxNode);
  return ret_str;
}

/**
 * @brief: node运行指令对应的字符串
 * @return:
 */
std::string ConnectNode::getCmdTypeString(int type) {
  std::string ret_str = "Unknown";

  switch (type) {
    case CmdStart:
      ret_str.assign("CmdStart");
      break;
    case CmdStop:
      ret_str.assign("CmdStop");
      break;
    case CmdStControl:
      ret_str.assign("CmdStControl");
      break;
    case CmdTextDialog:
      ret_str.assign("CmdTextDialog");
      break;
    case CmdExecuteDialog:
      ret_str.assign("CmdExecuteDialog");
      break;
    case CmdWarkWord:
      ret_str.assign("CmdWarkWord");
      break;
    case CmdCancel:
      ret_str.assign("CmdCancel");
      break;
    case CmdSendText:
      ret_str.assign("CmdSendText");
      break;
    case CmdSendPing:
      ret_str.assign("CmdSendPing");
      break;
    case CmdSendFlush:
      ret_str.assign("CmdSendFlush");
      break;
  }

  return ret_str;
}

bool ConnectNode::updateDestroyStatus() {
  MUTEX_LOCK(_mtxNode);

  bool ret = true;
  if (!_isDestroy) {
#ifdef __LINUX__
    if (_url._enableSysGetAddr) {
      if (_dnsThread) {
        LOG_WARN("Node(%p) dnsThread(%lu) still exist, waiting exiting", this,
                 _dnsThread);
        _dnsThreadExit = true;
        pthread_join(_dnsThread, NULL);
        LOG_WARN("Node(%p) dnsThread(%lu) exited.", this, _dnsThread);
        _dnsThread = 0;
      } else {
        LOG_DEBUG("Node(%p) dnsThread has exited.", this);
      }
    }
#endif

    _isDestroy = true;
    ret = false;
  } else {
    LOG_DEBUG("Node(%p) _isDestroy is true, do nothing ...", this);
  }

  MUTEX_UNLOCK(_mtxNode);
  return ret;
}

bool ConnectNode::getWakeStatus() {
  MUTEX_LOCK(_mtxNode);
  bool ret = _isWakeStop;
  MUTEX_UNLOCK(_mtxNode);
  return ret;
}

bool ConnectNode::checkConnectCount() {
  MUTEX_LOCK(_mtxNode);

  bool result = false;
  if (_retryConnectCount < RetryConnectCount) {
    _retryConnectCount++;
    result = true;
  } else {
    _retryConnectCount = 0;
    // return false : restart connect failed
  }
  LOG_INFO("Node(%p) check connection count: %d.", this, _retryConnectCount);

  MUTEX_UNLOCK(_mtxNode);
  return result;
}

bool ConnectNode::parseUrlInformation(char *directIp) {
  if (_request == NULL) {
    LOG_ERROR("Node(%p) this request is nullptr.", this);
    return false;
  }

  const char *address = _request->getRequestParam()->_url.c_str();
  const char *token = _request->getRequestParam()->_token.c_str();
  size_t tokenSize = _request->getRequestParam()->_token.size();

  memset(&_url, 0x0, sizeof(struct urlAddress));

  if (directIp && strnlen(directIp, 64) > 0) {
    LOG_DEBUG("Node(%p) direct ip address: %s.", this, directIp);

    if (sscanf(directIp, "%256[^:/]:%d", _url._address, &_url._port) == 2) {
      _url._directIp = true;
    } else if (sscanf(directIp, "%255s", _url._address) == 1) {
      _url._directIp = true;
    } else {
      LOG_ERROR("Node(%p) could not parse WebSocket direct ip:%s.", this,
                directIp);
      return false;
    }
  }

  LOG_INFO("Node(%p) address: %s.", this, address);

  if (WebSocketTcp::parseUrlAddress(_url, address) != Success) {
    LOG_ERROR("Node(%p) could not parse WebSocket url: %s.", this, address);
    return false;
  }

  memcpy(_url._token, token, tokenSize);

  LOG_INFO("Node(%p) type:%s, host:%s, port:%d, path:%s.", this, _url._type,
           _url._host, _url._port, _url._path);

  return true;
}

/**
 * @brief: 关闭ssl并释放event, 调用后会进行重链操作
 * @return:
 */
void ConnectNode::disconnectProcess() {
  bool lock_ret = true;
  MUTEX_TRY_LOCK(_mtxCloseNode, 2000, lock_ret);
  if (!lock_ret) {
    LOG_ERROR("Node(%p) disconnectProcess, deadlock has occurred", this);

    if (_releasingFlag || _exitStatus == ExitCancel) {
      LOG_ERROR(
          "Node(%p) in the process of releasing/canceling, skip "
          "disconnectProcess.",
          this);

      _isConnected = false;

      LOG_DEBUG(
          "Node(%p) disconnectProcess done, current node status:%s exit "
          "status:%s.",
          this, getConnectNodeStatusString().c_str(),
          getExitStatusString().c_str());
      return;
    }
  }

  LOG_DEBUG(
      "Node(%p) disconnectProcess begin, current node status:%s exit "
      "status:%s.",
      this, getConnectNodeStatusString().c_str(),
      getExitStatusString().c_str());

  if (_socketFd != INVALID_SOCKET) {
    if (_url._isSsl) {
      _sslHandle->sslClose();
    }
    evutil_closesocket(_socketFd);
    _socketFd = INVALID_SOCKET;

    if (_url._enableSysGetAddr && _dnsEvent) {
      event_free(_dnsEvent);
      _dnsEvent = NULL;
    }
  }

  _isConnected = false;

  LOG_DEBUG(
      "Node(%p) disconnectProcess done, current node status:%s exit status:%s.",
      this, getConnectNodeStatusString().c_str(),
      getExitStatusString().c_str());

  if (lock_ret) {
    MUTEX_UNLOCK(_mtxCloseNode);
  }
}

/**
 * @brief: 当前Node切换到close状态, 而不进行断网, 用于长链接模式
 * @return:
 */
void ConnectNode::closeStatusConnectNode() {
  MUTEX_LOCK(_mtxCloseNode);

  LOG_DEBUG(
      "Node(%p) closeStatusConnectNode begin, current node status:%s exit "
      "status:%s.",
      this, getConnectNodeStatusString().c_str(),
      getExitStatusString().c_str());

  if (_nlsEncoder) {
    _nlsEncoder->nlsEncoderSoftRestart();
  }

  if (_audioFrame) {
    free(_audioFrame);
    _audioFrame = NULL;
  }
  _audioFrameSize = 0;
  _maxFrameSize = 0;
  _isFirstAudioFrame = true;

  LOG_DEBUG(
      "Node(%p) closeStatusConnectNode done, current node status:%s exit "
      "status:%s.",
      this, getConnectNodeStatusString().c_str(),
      getExitStatusString().c_str());

  MUTEX_UNLOCK(_mtxCloseNode);
}

/**
 * @brief: 关闭ssl并释放event, 并设置node状态, 调用后往往进行释放操作.
 * @return:
 */
void ConnectNode::closeConnectNode() {
  bool lock_ret = true;
  MUTEX_TRY_LOCK(_mtxCloseNode, 2000, lock_ret);
  if (!lock_ret) {
    LOG_ERROR("Node(%p) closeConnectNode, deadlock has occurred", this);

    if (_releasingFlag || _exitStatus == ExitCancel) {
      LOG_ERROR(
          "Node(%p) in the process of releasing/canceling, skip "
          "closeConnectNode.",
          this);

      _isConnected = false;

      if (_audioFrame) {
        free(_audioFrame);
        _audioFrame = NULL;
      }
      _audioFrameSize = 0;
      _maxFrameSize = 0;
      _isFirstAudioFrame = true;

      LOG_INFO(
          "Node(%p) closeConnectNode done, current node status:%s exit "
          "status:%s.",
          this, getConnectNodeStatusString().c_str(),
          getExitStatusString().c_str());
      return;
    }
  }

  LOG_DEBUG(
      "Node(%p) closeConnectNode begin, current node status:%s exit status:%s.",
      this, getConnectNodeStatusString().c_str(),
      getExitStatusString().c_str());

  if (_socketFd != INVALID_SOCKET) {
    if (_url._isSsl) {
      _sslHandle->sslClose();
    }
    evutil_closesocket(_socketFd);
    _socketFd = INVALID_SOCKET;
  }

  _isConnected = false;

  if (_url._enableSysGetAddr && _dnsEvent) {
    event_del(_dnsEvent);
    event_free(_dnsEvent);
    _dnsEvent = NULL;
  }
  if (_readEvent) {
    event_del(_readEvent);
    event_free(_readEvent);
    _readEvent = NULL;
  }
  if (_writeEvent) {
    event_del(_writeEvent);
    event_free(_writeEvent);
    _writeEvent = NULL;
  }
  if (_connectEvent) {
    event_del(_connectEvent);
    event_free(_connectEvent);
    _connectEvent = NULL;
  }
  if (_launchEvent) {
    event_del(_launchEvent);
    event_free(_launchEvent);
    _launchEvent = NULL;
  }

#ifdef ENABLE_HIGH_EFFICIENCY
  if (_connectTimerEvent != NULL) {
    if (_connectTimerFlag) {
      evtimer_del(_connectTimerEvent);
      _connectTimerFlag = false;
    }
    event_free(_connectTimerEvent);
    _connectTimerEvent = NULL;
  }
#endif

  if (_audioFrame) {
    free(_audioFrame);
    _audioFrame = NULL;
  }
  _audioFrameSize = 0;
  _maxFrameSize = 0;
  _isFirstAudioFrame = true;

  LOG_INFO(
      "Node(%p) closeConnectNode done, current node status:%s exit status:%s.",
      this, getConnectNodeStatusString().c_str(),
      getExitStatusString().c_str());

  if (lock_ret) {
    MUTEX_UNLOCK(_mtxCloseNode);
  }
}

int ConnectNode::socketWrite(const uint8_t *buffer, size_t len) {
#if defined(__ANDROID__) || defined(__linux__)
  int wLen = send(_socketFd, (const char *)buffer, len, MSG_NOSIGNAL);
#else
  int wLen = send(_socketFd, (const char *)buffer, len, 0);
#endif

  if (wLen < 0) {
    int errorCode = utility::getLastErrorCode();
    if (NLS_ERR_RW_RETRIABLE(errorCode)) {
      // LOG_DEBUG("Node(%p) socketWrite continue.", this);

      return Success;
    } else {
      return -(SocketWriteFailed);
    }
  } else {
    return wLen;
  }
}

int ConnectNode::socketRead(uint8_t *buffer, size_t len) {
  int rLen = recv(_socketFd, (char *)buffer, len, 0);

  if (rLen <= 0) {  // rLen == 0, close socket, need do
    int errorCode = utility::getLastErrorCode();
    if (NLS_ERR_RW_RETRIABLE(errorCode)) {
      // LOG_DEBUG("Node(%p) socketRead continue.", this);

      return Success;
    } else {
      return -(SocketReadFailed);
    }
  } else {
    return rLen;
  }
}

int ConnectNode::gatewayRequest() {
  REQUEST_CHECK(_request, this);
  if (NULL == _readEvent) {
    LOG_ERROR("Node(%p) _readEvent is nullptr.", this);
    return -(EventEmpty);
  }

  if (_enableRecvTv) {
    utility::TextUtils::GetTimevalFromMs(
        &_recvTv, _request->getRequestParam()->getRecvTimeout());
    event_add(_readEvent, &_recvTv);
  } else {
    event_add(_readEvent, NULL);
  }

  char tmp[NodeFrameSize] = {0};
  int tmpLen = _webSocket.requestPackage(
      &_url, tmp, _request->getRequestParam()->GetHttpHeader());
  if (tmpLen < 0) {
    LOG_DEBUG("Node(%p) WebSocket request string failed.", this);
    return -(GetHttpHeaderFailed);
  };

  evbuffer_add(_cmdEvBuffer, (void *)tmp, tmpLen);

  return Success;
}

/**
 * @brief: 获取gateway的响应
 * @return: 成功则返回收到的字节数, 失败则返回负值.
 */
int ConnectNode::gatewayResponse() {
  int ret = 0;
  int read_len = 0;
  uint8_t *frame = (uint8_t *)calloc(ReadBufferSize, sizeof(char));
  if (frame == NULL) {
    LOG_ERROR("Node(%p) %s %d calloc failed.", this, __func__, __LINE__);
    return -(MallocFailed);
  }

  read_len = nlsReceive(frame, ReadBufferSize);
  if (read_len < 0) {
    LOG_ERROR("Node(%p) nlsReceive failed, read_len:%d", this, read_len);
    free(frame);
    return -(NlsReceiveFailed);
  } else if (read_len == 0) {
    LOG_WARN("Node(%p) nlsReceive empty, read_len:%d", this, read_len);
    free(frame);
    return -(NlsReceiveEmpty);
  }

  int frameSize = evbuffer_get_length(_readEvBuffer);
  if (frameSize > ReadBufferSize) {
    uint8_t *tmp = (uint8_t *)realloc(frame, frameSize + 1);
    if (NULL == tmp) {
      LOG_ERROR("Node(%p) %s %d realloc failed.", this, __func__, __LINE__);
      free(frame);
      return -(ReallocFailed);
    } else {
      frame = tmp;
    }
  }

  evbuffer_copyout(_readEvBuffer, frame, frameSize);  // evbuffer_peek

  ret = _webSocket.responsePackage((const char *)frame, frameSize);
  if (ret == 0) {
    evbuffer_drain(_readEvBuffer, frameSize);
  } else if (ret > 0) {
    LOG_DEBUG("Node(%p) GateWay Middle response: %d\n %s", this, frameSize,
              frame);
  } else {
    _nodeErrMsg = _webSocket.getFailedMsg();
    LOG_ERROR("Node(%p) webSocket.responsePackage: %s", this,
              _nodeErrMsg.c_str());
  }

  if (frame) free(frame);
  frame = NULL;

  return ret;
}

/**
 * @brief: 将buffer中剩余音频数据ws封包并发送
 * @return: 成功发送的字节数, 失败则返回负值.
 */
int ConnectNode::addRemainAudioData() {
  int ret = 0;
  if (_audioFrame != NULL && _audioFrameSize > 0) {
    ret = addAudioDataBuffer(_audioFrame, _audioFrameSize);
    memset(_audioFrame, 0, _maxFrameSize);
    _audioFrameSize = 0;
  }
  return ret;
}

/**
 * @brief: 将音频数据切片或填充后再ws封包并发送
 * @param frame	用户传入的数据
 * @param length	用户传入的数据字节数
 * @return: 成功发送的字节数(可能为0, 留下一包数据发送), 失败则返回负值.
 */
int ConnectNode::addSlicedAudioDataBuffer(const uint8_t *frame, size_t length) {
  int filling_ret = 0;

  if (_nlsEncoder && _encoderType != ENCODER_NONE) {
#ifdef ENABLE_NLS_DEBUG
    // LOG_DEBUG("Node(%p) addSlicedAudioDataBuffer input data %d bytes.", this,
    //           length);
#endif
    _maxFrameSize = _nlsEncoder->getFrameSampleBytes();
    if (_maxFrameSize <= 0) {
      return _maxFrameSize;
    }
    if (_audioFrame == NULL) {
      _audioFrame =
          (unsigned char *)calloc(_maxFrameSize, sizeof(unsigned char *));
      if (_audioFrame == NULL) {
        LOG_ERROR("Node(%p) malloc audio_data_buffer failed.", this);
        return -(MallocFailed);
#ifdef ENABLE_NLS_DEBUG
      } else {
        LOG_DEBUG("Node(%p) create audio frame data %d bytes.", this,
                  _maxFrameSize);
#endif
      }
      _audioFrameSize = 0;
    }

    int ret = 0;
    size_t frame_used_size = 0;        /*frame已经传入buffer的字节数*/
    size_t frame_remain_size = length; /*frame未传入buffer的字节数*/
    do {
      size_t buffer_space_size =
          _maxFrameSize -
          _audioFrameSize; /*buffer中空闲空间, 最大为_maxFrameSize*/
      if (frame_remain_size < buffer_space_size) {
        memcpy(_audioFrame + _audioFrameSize, frame + frame_used_size,
               frame_remain_size);
        _audioFrameSize += frame_remain_size;
        frame_used_size += frame_remain_size;
        frame_remain_size = 0;
      } else {
        memcpy(_audioFrame + _audioFrameSize, frame + frame_used_size,
               buffer_space_size);
        _audioFrameSize += buffer_space_size;
        frame_used_size += buffer_space_size;
        frame_remain_size -= buffer_space_size;
      }

      if (_audioFrameSize >= _maxFrameSize) {
        /*每次填充完整的一包数据*/
        ret = addAudioDataBuffer(_audioFrame, _maxFrameSize);
        memset(_audioFrame, 0, _maxFrameSize);
        filling_ret += _maxFrameSize;
        _audioFrameSize = 0;
#ifdef ENABLE_NLS_DEBUG
        // LOG_DEBUG(
        //     "Node(%p) ready to push audio data(%d) into nls_buffer, has
        //     digest "
        //     "%dbytes, ret:%d.",
        //     this, _maxFrameSize, frame_used_size, ret);
#endif
        if (ret < 0) {
          filling_ret = ret;
          break;
        }
      } else {
        if (_isFirstAudioFrame == false && _encoderType != ENCODER_OPU) {
          /*数据不足一包, 且非第一包数据. OPU第一包如果未满, 则会编码失败*/
          ret = addAudioDataBuffer(_audioFrame, _audioFrameSize);
          filling_ret += _audioFrameSize;
#ifdef ENABLE_NLS_DEBUG
          // LOG_DEBUG(
          //     "Node(%p) ready to push audio data(%d) into nls_buffer, has "
          //     "digest %dbytes, ret:%d.",
          //     this, _audioFrameSize, frame_used_size, ret);
#endif
          memset(_audioFrame, 0, _maxFrameSize);
          _audioFrameSize = 0;
          if (ret < 0) {
            filling_ret = ret;
            break;
          }
        } else {
#ifdef ENABLE_NLS_DEBUG
          // LOG_DEBUG("Node(%p) leave audio data(%d) for the next round.",
          // this,
          //           _audioFrameSize);
#endif
          break;
        }
      }
    } while (frame_used_size < length);
  } else {
    filling_ret = addAudioDataBuffer(frame, length);
  }

#ifdef ENABLE_NLS_DEBUG
  // LOG_DEBUG("Node(%p) pushed audio data(%d:%d) into audio_data_tmp_buffer.",
  //           this, length, filling_ret);
#endif
  return filling_ret;
}

/**
 * @brief: 将音频数据进行ws封包并发送
 * @param frame	用户传入的数据
 * @param frameSize	用户传入的数据字节数
 * @return: 成功发送的字节数(可能为0, 留下一包数据发送), 失败则返回负值.
 */
int ConnectNode::addAudioDataBuffer(const uint8_t *frame, size_t frameSize) {
  REQUEST_CHECK(_request, this);
  int ret = 0;
  uint8_t *tmp = NULL;
  size_t tmpSize = 0;
  size_t length = 0;
  struct evbuffer *buff = NULL;
  if (frame == NULL || frameSize == 0) {
    return -(NlsEncodingFailed);
  }
  if (_nlsEncoder && _encoderType != ENCODER_NONE) {
    uint8_t *outputBuffer = new uint8_t[frameSize];
    if (outputBuffer == NULL) {
      LOG_ERROR("Node(%p) new outputBuffer failed.", this);
      return -(NewOutputBufferFailed);
    } else {
      memset(outputBuffer, 0, frameSize);
      int nSize = _nlsEncoder->nlsEncoding(frame, (int)frameSize, outputBuffer,
                                           (int)frameSize);
#ifdef ENABLE_NLS_DEBUG
      // LOG_DEBUG(
      //     "Node(%p) Opus encoder(%d) encoding %dbytes data, and return "
      //     "nSize:%d.",
      //     this, _encoderType, frameSize, nSize);
#endif
      if (nSize < 0) {
        LOG_ERROR("Node(%p) Opus encoder failed:%d.", this, nSize);
        delete[] outputBuffer;
        return -(NlsEncodingFailed);
      }
      _webSocket.binaryFrame(outputBuffer, nSize, &tmp, &tmpSize);
      delete[] outputBuffer;
    }
  } else {
    // pack frame data
    _webSocket.binaryFrame(frame, frameSize, &tmp, &tmpSize);
  }

  if (_request && _request->getRequestParam()->_enableWakeWord == true &&
      !getWakeStatus()) {
    buff = _wwvEvBuffer;
  } else {
    buff = _binaryEvBuffer;
  }

  evbuffer_lock(buff);
  length = evbuffer_get_length(buff);
  if (length >= _limitSize) {
    LOG_WARN("Too many audio data in evbuffer.");
    evbuffer_unlock(buff);
    return -(EvbufferTooMuch);
  }

  evbuffer_add(buff, (void *)tmp, tmpSize);

  if (tmp) free(tmp);
  tmp = NULL;

  evbuffer_unlock(buff);

  if (length == 0 && _workStatus == NodeStarted) {
    MUTEX_LOCK(_mtxNode);
    if (!_isStop) {
      ret = nlsSendFrame(buff);
    }
    MUTEX_UNLOCK(_mtxNode);
  }

  if (length == 0 && _workStatus == NodeWakeWording) {
    MUTEX_LOCK(_mtxNode);
    if (!_isStop) {
      ret = nlsSendFrame(buff);
    }
    MUTEX_UNLOCK(_mtxNode);
  }

  if (ret == 0) {
    ret = sendControlDirective();
  }

  if (ret < 0) {
    disconnectProcess();
    handlerTaskFailedEvent(getErrorMsg());
  } else {
    ret = frameSize;
    _isFirstAudioFrame = false;
  }

  return ret;
}

/**
 * @brief: 发送控制命令
 * @return: 成功发送的字节数, 失败则返回负值.
 */
int ConnectNode::sendControlDirective() {
  MUTEX_LOCK(_mtxNode);

  int ret = 0;
  if (_workStatus == NodeStarted && _exitStatus == ExitInvalid) {
    size_t length = evbuffer_get_length(getCmdEvBuffer());
    if (length != 0) {
      // LOG_DEBUG("Node(%p) cmd buffer isn't empty.", this);
      ret = nlsSendFrame(getCmdEvBuffer());
    }
  } else {
    if (_exitStatus == ExitStopping && _isStop == false) {
      // LOG_DEBUG("Node(%p) audio has send done. And invoke stop command.",
      // this);
      addCmdDataBuffer(CmdStop);
      ret = nlsSendFrame(getCmdEvBuffer());
      _isStop = true;
    }
  }

  MUTEX_UNLOCK(_mtxNode);
  return ret;
}

/**
 * @brief: 将命令字符串加入buffer用于发送
 * @param type	CmdType
 * @param message	待发送命令
 * @return:
 */
void ConnectNode::addCmdDataBuffer(CmdType type, const char *message) {
  char *cmd = NULL;

  LOG_DEBUG("Node(%p) get command type: %s.", this,
            getCmdTypeString(type).c_str());

  if (_request == NULL) {
    LOG_ERROR("The rquest of node(%p) is nullptr.", this);
    return;
  }
  if (_request->getRequestParam() == NULL) {
    LOG_ERROR("The requestParam of request(%p) node(%p) is nullptr.", _request,
              this);
    return;
  }

  switch (type) {
    case CmdStart:
      if (_reconnection.state == NodeReconnection::TriggerReconnection) {
        // setting tw_time_offset and tw_index_offset
        Json::Value root;
        Json::FastWriter writer;
        root["tw_time_offset"] =
            Json::UInt64(_reconnection.interruption_timestamp_ms -
                         _reconnection.first_audio_timestamp_ms);
        root["tw_index_offset"] = (Json::UInt64)_reconnection.tw_index_offset;
        std::string buf = writer.write(root);
        _request->getRequestParam()->setPayloadParam(buf.c_str());
      }
      cmd = (char *)_request->getRequestParam()->getStartCommand();
      if (_reconnection.state == NodeReconnection::TriggerReconnection) {
        // cleaning tw_time_offset and tw_index_offset
        _request->getRequestParam()->removePayloadParam("tw_time_offset");
        _request->getRequestParam()->removePayloadParam("tw_index_offset");
      }
      _reconnection.state = NodeReconnection::NewReconnectionStarting;
      break;
    case CmdStControl:
      cmd = (char *)_request->getRequestParam()->getControlCommand(message);
      break;
    case CmdStop:
      cmd = (char *)_request->getRequestParam()->getStopCommand();
      break;
    case CmdTextDialog:
      cmd = (char *)_request->getRequestParam()->getExecuteDialog();
      break;
    case CmdWarkWord:
      cmd = (char *)_request->getRequestParam()->getStopWakeWordCommand();
      break;
    case CmdCancel:
      LOG_DEBUG("Node(%p) add cancel command, do nothing.", this);
      return;
    case CmdSendText:
      cmd = (char *)_request->getRequestParam()->getRunFlowingSynthesisCommand(
          message);
      break;
    case CmdSendPing:
      cmd = (char *)"{ping}";
      break;
    case CmdSendFlush:
      cmd = (char *)_request->getRequestParam()->getFlushFlowingTextCommand();
      break;
    default:
      LOG_WARN("Node(%p) add unknown command, do nothing.", this);
      return;
  }

  if (cmd) {
    std::string buf_str;
    LOG_INFO("Node(%p) get command: %s, and add into evbuffer.", this,
             utility::TextUtils::securityDisposalForLog(cmd, &buf_str,
                                                        "appkey\":\"", 8, 'Z'));

    uint8_t *frame = NULL;
    size_t frameSize = 0;
    if (type == CmdSendPing) {
      _webSocket.pingFrame(&frame, &frameSize);
    } else {
      _webSocket.textFrame((uint8_t *)cmd, strlen(cmd), &frame, &frameSize);
    }

    evbuffer_add(_cmdEvBuffer, (void *)frame, frameSize);

    if (frame) free(frame);
    frame = NULL;
  }
}

/**
 * @brief: 命令发送
 * @param type	CmdType
 * @param message	待发送命令
 * @return: 成功则返回发送字节数, 失败则返回负值
 */
int ConnectNode::cmdNotify(CmdType type, const char *message) {
  int ret = Success;

  LOG_DEBUG("Node(%p) invoke CmdNotify: %s.", this,
            getCmdTypeString(type).c_str());

  if (type == CmdStop) {
#ifdef ENABLE_REQUEST_RECORDING
    updateNodeProcess("stop", NodeStop, true, 0);
#endif
    addRemainAudioData();
    _exitStatus = ExitStopping;
    if (_workStatus == NodeStarted) {
      size_t length = evbuffer_get_length(_binaryEvBuffer);
      if (length == 0) {
        ret = sendControlDirective();
      } else {
        LOG_DEBUG(
            "Node(%p) invoke CmdNotify: %s, and continue send audio data "
            "%zubytes.",
            this, getCmdTypeString(type).c_str(), length);
      }
    }
  } else if (type == CmdCancel) {
#ifdef ENABLE_REQUEST_RECORDING
    updateNodeProcess("cancel", NodeCancel, true, 0);
#endif
    _exitStatus = ExitCancel;
  } else if (type == CmdStControl) {
#ifdef ENABLE_REQUEST_RECORDING
    updateNodeProcess("ctrl", NodeSendControl, true, 0);
#endif
    addCmdDataBuffer(CmdStControl, message);
    if (_workStatus == NodeStarted) {
      size_t length = evbuffer_get_length(_binaryEvBuffer);
      if (length == 0) {
        ret = nlsSendFrame(_cmdEvBuffer);
      }
    }
  } else if (type == CmdWarkWord) {
    _isWakeStop = true;
    size_t length = evbuffer_get_length(_wwvEvBuffer);
    if (length == 0) {
      addCmdDataBuffer(CmdWarkWord);
      ret = nlsSendFrame(_cmdEvBuffer);
    }
  } else if (type == CmdSendText) {
#ifdef ENABLE_REQUEST_RECORDING
    updateNodeProcess("send_text", NodeSendText, true, 0);
#endif
    addCmdDataBuffer(CmdSendText, message);
    if (_workStatus == NodeStarted) {
      ret = nlsSendFrame(_cmdEvBuffer);
    }
  } else if (type == CmdSendPing) {
    addCmdDataBuffer(CmdSendPing, NULL);
    ret = nlsSendFrame(_cmdEvBuffer);
  } else if (type == CmdSendFlush) {
    addCmdDataBuffer(CmdSendFlush, NULL);
    ret = nlsSendFrame(_cmdEvBuffer);
  } else {
    LOG_ERROR("Node(%p) invoke unknown command.", this);
  }

  if (ret < 0) {
    disconnectProcess();
    handlerTaskFailedEvent(getErrorMsg());
  }

#ifdef ENABLE_REQUEST_RECORDING
  if (type == CmdStop) {
    updateNodeProcess("stop", NodeStop, false, 0);
  } else if (type == CmdCancel) {
    updateNodeProcess("cancel", NodeCancel, false, 0);
  } else if (type == CmdStControl) {
    updateNodeProcess("ctrl", NodeSendControl, false, 0);
  } else if (type == CmdSendText) {
    updateNodeProcess("send_text", NodeSendText, false, 0);
  }
#endif
  return ret;
}

int ConnectNode::nlsSend(const uint8_t *frame, size_t length) {
  int sLen = 0;
  if ((frame == NULL) || (length == 0)) {
    return 0;
  }

  if (_url._isSsl) {
    sLen = _sslHandle->sslWrite(frame, length);
  } else {
    sLen = socketWrite(frame, length);
  }

  if (sLen < 0) {
    if (_url._isSsl) {
      _nodeErrMsg = _sslHandle->getFailedMsg();
    } else {
      _nodeErrMsg =
          evutil_socket_error_to_string(evutil_socket_geterror(_socketFd));
    }

    LOG_ERROR("Node(%p) send failed: %s.", this, _nodeErrMsg.c_str());
  }

  return sLen;
}

/**
 * @brief: 发送一帧数据
 * @return: 成功发送的字节数, 失败则返回负值.
 */
int ConnectNode::nlsSendFrame(struct evbuffer *eventBuffer, bool audio_frame) {
  int sLen = 0;
  uint8_t buffer[NodeFrameSize] = {0};
  size_t bufferSize = 0;

  evbuffer_lock(eventBuffer);
  size_t length = evbuffer_get_length(eventBuffer);
  if (length == 0) {
    // LOG_DEBUG("Node(%p) eventBuffer is NULL.", this);
    evbuffer_unlock(eventBuffer);
    return 0;
  }

  if (length > NodeFrameSize) {
    bufferSize = NodeFrameSize;
  } else {
    bufferSize = length;
  }
  evbuffer_copyout(eventBuffer, buffer, bufferSize);  // evbuffer_peek

  if (bufferSize > 0) {
    sLen = nlsSend(buffer, bufferSize);
  }

  if (sLen < 0) {
    LOG_ERROR("Node(%p) nlsSend failed, nlsSend return:%d.", this, sLen);
    evbuffer_unlock(eventBuffer);
    return -(NlsSendFailed);
  } else {
    // send data success
    if (audio_frame && _isFirstBinaryFrame) {
      _isFirstBinaryFrame = false;
#ifdef ENABLE_CONTINUED
      _reconnection.first_audio_timestamp_ms =
          utility::TextUtils::GetTimestampMs();
#endif
    }

    evbuffer_drain(eventBuffer, sLen);
    length = evbuffer_get_length(eventBuffer);
    if (length > 0) {
      if (NULL == _writeEvent) {
        LOG_ERROR("Node(%p) event is nullptr.", this);
        evbuffer_unlock(eventBuffer);
        return -(EventEmpty);
      }

      utility::TextUtils::GetTimevalFromMs(
          &_sendTv, _request->getRequestParam()->getSendTimeout());
      event_add(_writeEvent, &_sendTv);
    }
    evbuffer_unlock(eventBuffer);
    return length;
  }
}

int ConnectNode::nlsReceive(uint8_t *buffer, int max_size) {
  int rLen = 0;
  int read_buffer_size = max_size;
  if (_url._isSsl) {
    rLen = _sslHandle->sslRead((uint8_t *)buffer, read_buffer_size);
  } else {
    rLen = socketRead((uint8_t *)buffer, read_buffer_size);
  }

  if (rLen < 0) {
    if (_url._isSsl) {
      _nodeErrMsg = _sslHandle->getFailedMsg();
    } else {
      _nodeErrMsg =
          evutil_socket_error_to_string(evutil_socket_geterror(_socketFd));
    }
    LOG_ERROR("Node(%p) recv failed: %s.", this, _nodeErrMsg.c_str());
    return -(ReadFailed);
  }

  evbuffer_add(_readEvBuffer, (void *)buffer, rLen);

  return rLen;
}

/**
 * @brief: 接收一帧数据
 * @return: 成功接收的字节数, 失败则返回负值.
 */
int ConnectNode::webSocketResponse() {
  int ret = 0;
  int read_len = 0;

  if (_releasingFlag) {
    LOG_WARN("Node(%p) is releasing!!! skipping ...", this);
    return -(InvalidStatusWhenReleasing);
  }

  uint8_t *frame = (uint8_t *)calloc(ReadBufferSize, sizeof(char));
  if (frame == NULL) {
    LOG_ERROR("%s %d calloc failed.", __func__, __LINE__);
    return 0;
  }

  read_len = nlsReceive(frame, ReadBufferSize);
  if (read_len < 0) {
    LOG_ERROR("Node(%p) nlsReceive failed, read_len:%d", this, read_len);
    free(frame);
    return -(NlsReceiveFailed);
  } else if (read_len == 0) {
    // LOG_DEBUG("Node(%p) nlsReceive empty, read_len:%d", this, read_len);
    free(frame);
    return 0;
  }

  bool eLoop = false;
  do {
    ret = 0;
    size_t frameSize = evbuffer_get_length(_readEvBuffer);
    if (frameSize == 0) {
      free(frame);
      frame = NULL;
      ret = 0;
      break;
    } else if (frameSize > ReadBufferSize) {
      uint8_t *tmp = (uint8_t *)realloc(frame, frameSize + 1);
      if (NULL == tmp) {
        LOG_ERROR("Node(%p) realloc failed.", this);
        free(frame);
        frame = NULL;
        ret = -(ReallocFailed);
        break;
      } else {
        frame = tmp;
        LOG_WARN("Node(%p) websocket frame realloc, new size:%d.", this,
                 frameSize + 1);
      }
    }

    size_t cur_data_size = frameSize;
    evbuffer_copyout(_readEvBuffer, frame, frameSize);

    WebSocketFrame wsFrame;
    memset(&wsFrame, 0x0, sizeof(struct WebSocketFrame));
    if (_webSocket.receiveFullWebSocketFrame(frame, frameSize, &_wsType,
                                             &wsFrame) == Success) {
      // LOG_DEBUG("Node(%p) parse websocket frame, len:%zu, frame size:%zu,
      // _wsType.opCode:%d, wsFrame.type:%d.",
      //     this, wsFrame.length, frameSize, _wsType.opCode, wsFrame.type);

      if (_releasingFlag) {
        LOG_WARN("Node(%p) is releasing!!! skipping ...", this);
        ret = -(InvalidStatusWhenReleasing);
        break;
      }

      if (_wsType.opCode == WebSocketHeaderType::PONG) {
        LOG_DEBUG("Node(%p) receive PONG.", this);
        // memset(&_wsType, 0x0, sizeof(struct WebSocketHeaderType));
        wsFrame.type = _wsType.opCode;
      }

      /*
       * Will invoke callback in parseFrame.
       * If blocking in callback, will block in parseFrame
       */
      int result = parseFrame(&wsFrame);
      if (result) {
        LOG_ERROR("Node(%p) parse WS frame failed:%d.", this, result);
        ret = result;
        break;
      }

      /* Should check node here */
      if (_instance == NULL) {
        /* Maybe user has released instance */
        ret = -(EventClientEmpty);
        break;
      } else {
        /* Maybe user has released request.*/
        NlsNodeManager *node_manager = _instance->getNodeManger();
        int status = NodeStatusInvalid;
        ret = node_manager->checkNodeExist(this, &status);
        if (ret != Success) {
          LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", this, ret);
          break;
        }
      }

      evbuffer_drain(_readEvBuffer, wsFrame.length + _wsType.headerSize);
      cur_data_size = cur_data_size - (wsFrame.length + _wsType.headerSize);

      ret = wsFrame.length + _wsType.headerSize;
    }

    /* 解析成功并还有剩余数据, 则尝试再解析 */
    if (ret > 0 && cur_data_size > 0) {
      LOG_DEBUG(
          "Node(%p) current data remainder size:%d, ret:%d, receive ws frame "
          "continue...",
          this, cur_data_size, ret);
      eLoop = true;
    } else {
      eLoop = false;
    }
  } while (eLoop);

  if (frame) free(frame);
  frame = NULL;

  return ret;
}

NlsEvent *ConnectNode::convertResult(WebSocketFrame *wsFrame, int *ret) {
  NlsEvent *wsEvent = NULL;

  if (_request == NULL) {
    LOG_ERROR("Node(%p) this request is nullptr.", this);
    *ret = -(RequestEmpty);
    return NULL;
  }

  if (wsFrame->type == WebSocketHeaderType::BINARY_FRAME) {
    if (wsFrame->length > 0) {
      std::vector<unsigned char> data = std::vector<unsigned char>(
          wsFrame->data, wsFrame->data + wsFrame->length);

      wsEvent = new NlsEvent(data, Success, NlsEvent::Binary,
                             _request->getRequestParam()->_task_id);
      if (wsEvent == NULL) {
        LOG_ERROR("Node(%p) new NlsEvent failed!", this);
        handlerEvent(TASKFAILED_NEW_NLSEVENT_FAILED, MemNotEnough,
                     NlsEvent::TaskFailed, _enableOnMessage);
        *ret = -(NewNlsEventFailed);
      }
    } else {
      LOG_WARN("Node(%p) this ws frame length is invalid %d.", wsFrame->length);
      *ret = -(WsFrameBodyEmpty);
      return NULL;
    }
  } else if (wsFrame->type == WebSocketHeaderType::TEXT_FRAME) {
    /* 打印这个string，可能会因为太长而崩溃 */
    std::string result((char *)wsFrame->data, wsFrame->length);
    if (wsFrame->length > 1024) {
      std::string part_result((char *)wsFrame->data, 1024);
      LOG_DEBUG(
          "Node(%p) ws frame len:%d is too long, part response(1024): %s.",
          this, wsFrame->length, part_result.c_str());
    } else if (wsFrame->length == 0) {
      LOG_ERROR("Node(%p) ws frame len is zero!", this);
    } else {
      LOG_DEBUG("Node(%p) response(ws frame len:%d): %s", this, wsFrame->length,
                result.c_str());
    }

    if ("GBK" == _request->getRequestParam()->_outputFormat) {
      result = utility::TextUtils::utf8ToGbk(result);
    }
    if (result.empty()) {
      LOG_ERROR("Node(%p) response result is empty!", this);
      handlerEvent(TASKFAILED_UTF8_JSON_STRING, Utf8ConvertError,
                   NlsEvent::TaskFailed, _enableOnMessage);
      *ret = -(WsResponsePackageEmpty);
      return NULL;
    }

    wsEvent = new NlsEvent(result);
    if (wsEvent == NULL) {
      LOG_ERROR("Node(%p) new NlsEvent failed!", this);
      handlerEvent(TASKFAILED_NEW_NLSEVENT_FAILED, MemNotEnough,
                   NlsEvent::TaskFailed, _enableOnMessage);
      *ret = -(NewNlsEventFailed);
    } else {
      *ret = wsEvent->parseJsonMsg(_enableOnMessage);
      if (*ret < 0) {
        LOG_ERROR("Node(%p) parseJsonMsg(%s) failed! ret:%d.", this,
                  wsEvent->getAllResponse(), ret);
        delete wsEvent;
        wsEvent = NULL;
        handlerEvent(TASKFAILED_PARSE_JSON_STRING, JsonStringParseFailed,
                     NlsEvent::TaskFailed, _enableOnMessage);
      } else {
        // LOG_DEBUG("Node(%p) parseJsonMsg success.", this);
      }
    }
  } else {
    LOG_ERROR("Node(%p) unknow WebSocketHeaderType:%d", this, wsFrame->type);
    handlerEvent(TASKFAILED_WS_JSON_STRING, UnknownWsHeadType,
                 NlsEvent::TaskFailed, _enableOnMessage);
    *ret = -(UnknownWsFrameHeadType);
  }

  return wsEvent;
}

/**
 * @brief: 解析websocket帧, 产出当前node的事件帧(frameEvent)
 * @return:
 */
int ConnectNode::parseFrame(WebSocketFrame *wsFrame) {
  REQUEST_CHECK(_request, this);
  int result = Success;
  NlsEvent *frameEvent = NULL;

  if (wsFrame->type == WebSocketHeaderType::CLOSE) {
    LOG_INFO("Node(%p) get CLOSE wsFrame closeCode:%d.", this,
             wsFrame->closeCode);
    if (NodeClosed != _workStatus) {
      std::string msg((char *)wsFrame->data);
      char tmp_msg[2048] = {0};
      snprintf(tmp_msg, 2048 - 1, "{\"TaskFailed\":\"%s\"}", msg.c_str());
      std::string failedMsg = tmp_msg;

      LOG_ERROR("Node(%p) failed msg:%s.", this, failedMsg.c_str());

      frameEvent = new NlsEvent(failedMsg.c_str(), wsFrame->closeCode,
                                NlsEvent::TaskFailed,
                                _request->getRequestParam()->_task_id);
      if (frameEvent == NULL) {
        LOG_ERROR("Node(%p) new NlsEvent failed!", this);
        handlerEvent(TASKFAILED_NEW_NLSEVENT_FAILED, MemNotEnough,
                     NlsEvent::TaskFailed, _enableOnMessage);
        return -(NewNlsEventFailed);
      }
    } else {
      LOG_INFO("Node(%p) NlsEvent::Close has invoked, skip CLOSE_FRAME.", this);
    }
  } else if (wsFrame->type == WebSocketHeaderType::PONG) {
    LOG_DEBUG("Node(%p) get PONG.", this);
    return Success;
  } else {
    frameEvent = convertResult(wsFrame, &result);
  }

  if (frameEvent == NULL) {
    if (result == -(WsFrameBodyEmpty)) {
      LOG_WARN(
          "Node(%p) convert result failed, result:%d. Maybe recv dirty data, "
          "skip here ...",
          this, result);
      return Success;
    } else {
      LOG_ERROR("Node(%p) convert result failed, result:%d.", this, result);
      closeConnectNode();
      if (result != Success) {
        std::string tmp_buf;
        handlerEvent(genCloseMsg(&tmp_buf), CloseCode, NlsEvent::Close,
                     _enableOnMessage);
      }
      return -(NlsEventEmpty);
    }
  }

  LOG_DEBUG(
      "Node(%p) begin HandlerFrame, msg type:%s node status:%s exit status:%s.",
      this, frameEvent->getMsgTypeString().c_str(),
      getConnectNodeStatusString().c_str(), getExitStatusString().c_str());

  // invoked cancel()
  if (_exitStatus == ExitCancel) {
    LOG_WARN("Node(%p) has been canceled.", this);
    if (frameEvent) delete frameEvent;
    frameEvent = NULL;
    return -(InvalidExitStatus);
  }

  result = handlerFrame(frameEvent);
  if (result) {
    delete frameEvent;
    frameEvent = NULL;
    return result;
  }

  LOG_DEBUG(
      "Node(%p) HandlerFrame finish, current node status:%s, ready to set "
      "workStatus.",
      this, getConnectNodeStatusString().c_str());

  bool closeFlag = false;
  int msg_type = frameEvent->getMsgType();
  switch (msg_type) {
    case NlsEvent::RecognitionStarted:
    case NlsEvent::TranscriptionStarted:
    case NlsEvent::SynthesisStarted:
      if (_request->getRequestParam()->_requestType != SpeechWakeWordDialog) {
        _workStatus = NodeStarted;
      } else {
        _workStatus = NodeWakeWording;
      }
#ifdef ENABLE_CONTINUED
      // reconnecting finished
      _reconnection.state = NodeReconnection::NoReconnection;
#endif
      break;
    case NlsEvent::Close:
    case NlsEvent::RecognitionCompleted:
      _workStatus = NodeCompleted;
      if (_request->getRequestParam()->_mode == TypeDialog) {
        closeFlag = false;
      } else {
        closeFlag = true;
      }
      break;
    case NlsEvent::TaskFailed:
      _workStatus = NodeFailed;
      closeFlag = true;
      break;
    case NlsEvent::TranscriptionCompleted:
      _workStatus = NodeCompleted;
      closeFlag = true;
      break;
    case NlsEvent::SynthesisCompleted:
      _workStatus = NodeCompleted;
      closeFlag = true;
      break;
    case NlsEvent::DialogResultGenerated:
      closeFlag = true;
      break;
    case NlsEvent::WakeWordVerificationCompleted:
      _workStatus = NodeStarted;
      break;
    case NlsEvent::Binary:
      if (_isFirstBinaryFrame) {
        _isFirstBinaryFrame = false;
        _workStatus = NodeStarted;
      }
      break;
    default:
      closeFlag = false;
      break;
  }

  if (frameEvent) delete frameEvent;
  frameEvent = NULL;

  if (closeFlag) {
    if (!_isLongConnection || _workStatus == NodeFailed) {
      closeConnectNode();
    } else {
      closeStatusConnectNode();
    }

    std::string tmp_buf;
    handlerEvent(genCloseMsg(&tmp_buf), CloseCode, NlsEvent::Close,
                 _enableOnMessage);
  }

  return Success;
}

/**
 * @brief: 触发回调，将事件送给用户
 * @return:
 */
int ConnectNode::handlerFrame(NlsEvent *frameEvent) {
  if (_workStatus == NodeInvalid) {
    LOG_ERROR("Node(%p) current node status:%s is invalid, skip callback.",
              this, getConnectNodeStatusString().c_str());
    return -(InvaildNodeStatus);
  } else {
    if (_handler == NULL) {
      LOG_ERROR("Node(%p) _handler is nullptr!", this)
      return -(NlsEventEmpty);
    }

#ifdef ENABLE_REQUEST_RECORDING
    updateNodeProcess("callback", frameEvent->getMsgType(), true,
                      frameEvent->getMsgType() == NlsEvent::Binary
                          ? frameEvent->getBinaryData().size()
                          : 0);
#endif

    // LOG_DEBUG("Node:%p current node status:%s is valid, msg type:%s handle
    // message ...",
    //     this, getConnectNodeStatusString().c_str(),
    //     frameEvent->getMsgTypeString().c_str());
    // LOG_DEBUG("Node:%p current response:%s.", this,
    // frameEvent->getAllResponse());

    bool ignore_flag = false;
#ifdef ENABLE_CONTINUED
    updateTwIndexOffset(frameEvent);

    ignore_flag = ignoreCallbackWhenReconnecting(frameEvent->getMsgType(),
                                                 frameEvent->getStatusCode());
#endif

    if (_enableOnMessage) {
      sendFinishCondSignal(NlsEvent::Message);
      if (!ignore_flag) {
        handlerMessage(frameEvent->getAllResponse(), NlsEvent::Message);
      }
    } else {
      sendFinishCondSignal(frameEvent->getMsgType());
      if (!ignore_flag) {
        _handler->handlerFrame(*frameEvent);
      }
    }

#ifdef ENABLE_REQUEST_RECORDING
    updateNodeProcess("callback", NodeInvalid, false, 0);
#endif

    if (frameEvent->getMsgType() == NlsEvent::Close) {
#ifdef ENABLE_CONTINUED
      nodeReconnecting();
      if (!ignore_flag) {
        _reconnection.reconnected_count = 0;
        LOG_INFO("Node(%p) reconnected_count reset.", this);
      }
#endif
      _retryConnectCount = 0;
    }
  }
  return Success;
}

/**
 * @brief: 事件最终处理，并进行回调
 * @return:
 */
void ConnectNode::handlerEvent(const char *errorMsg, int errorCode,
                               NlsEvent::EventType eventType, bool ignore) {
  LOG_DEBUG("Node(%p) 's exit status:%s, eventType:%d.", this,
            getExitStatusString().c_str(), eventType);
  if (_exitStatus == ExitCancel) {
    LOG_WARN("Node(%p) invoke cancel command, callback won't be invoked.",
             this);
    return;
  }

  if (_request == NULL) {
    LOG_ERROR("The request of this node(%p) is nullptr.", this);
    return;
  }

  if (errorCode != CloseCode) {
    _nodeErrCode = errorCode;
  }

  std::string error_str(errorMsg);
  if (error_str.empty()) {
    LOG_WARN("Node(%p) errorMsg is empty!", this);
  }
#ifdef ENABLE_REQUEST_RECORDING
  if (eventType == NlsEvent::Close || eventType == NlsEvent::TaskFailed) {
    if (eventType == NlsEvent::TaskFailed) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.failed_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
    } else if (eventType == NlsEvent::Close) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.closed_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
    }
    error_str.assign(replenishNodeProcess(errorMsg));
    if (eventType == NlsEvent::TaskFailed) {
      LOG_ERROR("Node(%p) trigger message: %s", this, error_str.c_str());
    } else if (eventType == NlsEvent::Close) {
      LOG_INFO("Node(%p) trigger message: %s", this, error_str.c_str());
    }
  }
#endif
  NlsEvent *useEvent = NULL;
  useEvent = new NlsEvent(error_str.c_str(), errorCode, eventType,
                          _request->getRequestParam()->_task_id);
  if (useEvent == NULL) {
    LOG_ERROR("Node(%p) new NlsEvent failed.", this);
    return;
  }

  if (eventType == NlsEvent::Close) {
    LOG_INFO("Node(%p) will callback NlsEvent::Close frame.", this);
  }

  if (_handler == NULL) {
    LOG_ERROR("Node(%p) event type:%d 's _handler is nullptr!", this, eventType)
  } else {
    if (NodeClosed == _workStatus) {
      LOG_WARN("Node(%p) NlsEvent::Close has invoked, skip CloseCallback.",
               this);
    } else {
      handlerFrame(useEvent);
      if (eventType == NlsEvent::Close) {
        _workStatus = NodeClosed;
        LOG_INFO("Node(%p) callback NlsEvent::Close frame done.", this);
      } else {
        LOG_INFO("Node(%p) callback NlsEvent::%s frame done.", this,
                 useEvent->getMsgTypeString().c_str());
      }
    }
  }

  delete useEvent;
  useEvent = NULL;

  return;
}

void ConnectNode::handlerMessage(const char *response,
                                 NlsEvent::EventType eventType) {
  NlsEvent *useEvent = NULL;
  useEvent = new NlsEvent(response, Success, eventType,
                          _request->getRequestParam()->_task_id);
  if (useEvent == NULL) {
    LOG_ERROR("Node(%p) new NlsEvent failed.", this);
    return;
  }

  if (_workStatus == NodeInvalid) {
    LOG_ERROR("Node(%p) node status:%s is invalid, skip callback.", this,
              getConnectNodeStatusString().c_str());
  } else {
    _handler->handlerFrame(*useEvent);
  }

  delete useEvent;
  useEvent = NULL;
  return;
}

/**
 * @brief: 解析错误信息获得对应错误码
 * @return: 错误码
 */
int ConnectNode::getErrorCodeFromMsg(const char *msg) {
  int code = DefaultErrorCode;

  try {
    Json::Reader reader;
    Json::Value root(Json::objectValue);

    // parse json if existent
    if (reader.parse(msg, root)) {
      if (!root["header"].isNull() && root["header"].isObject()) {
        Json::Value head = root["header"];
        if (!head["status"].isNull() && head["status"].isInt()) {
          code = head["status"].asInt();
        }
      }
    } else {
      if (strstr(msg, "return of SSL_read:")) {
        if (strstr(msg, "error:00000000:lib(0):func(0):reason(0)") ||
            strstr(msg, "shutdown while in init")) {
          code = DefaultErrorCode;
        }
      } else if (strstr(msg, "ACCESS_DENIED")) {
        if (strstr(msg, "The token")) {
          if (strstr(msg, "has expired")) {
            code = TokenHasExpired;
          } else if (strstr(msg, "is invalid")) {
            code = TokenIsInvalid;
          }
        } else if (strstr(msg, "No privilege to this voice")) {
          code = NoPrivilegeToVoice;
        } else if (strstr(msg, "Missing authorization header")) {
          code = MissAuthHeader;
        }
      } else if (strstr(msg, "Got bad status")) {
        if (strstr(msg, "403 Forbidden")) {
          code = HttpGotBadStatusWith403;
        }
      } else if (strstr(msg, "Operation now in progress")) {
        code = OpNowInProgress;
      } else if (strstr(msg, "Broken pipe")) {
        code = BrokenPipe;
      } else if (strstr(msg, "connect failed")) {
        code = SysConnectFailed;
      }
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return code;
  }
  return code;
}

void ConnectNode::handlerTaskFailedEvent(std::string failedInfo, int code) {
  char tmp_msg[1024] = {0};

  if (failedInfo.empty()) {
    strcpy(tmp_msg, "{\"TaskFailed\":\"Unknown failed.\"}");
    code = DefaultErrorCode;
  } else {
    snprintf(tmp_msg, 1024 - 1, "{\"TaskFailed\":\"%s\"}", failedInfo.c_str());
    if (code == DefaultErrorCode) {
      code = getErrorCodeFromMsg(failedInfo.c_str());
    }
  }

#ifdef ENABLE_DNS_IP_CACHE
  // clear IP cache
  _eventThread->setIpCache(NULL, NULL);
#endif

  handlerEvent(tmp_msg, code, NlsEvent::TaskFailed, _enableOnMessage);

  std::string tmp_buf;
  handlerEvent(genCloseMsg(&tmp_buf), CloseCode, NlsEvent::Close,
               _enableOnMessage);
  return;
}

#ifdef __LINUX__
void *ConnectNode::async_dns_resolve_thread_fn(void *arg) {
  pthread_detach(pthread_self());
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  LOG_DEBUG("Node(%p) dnsThread(%lu) is working ...", node, pthread_self());

  if (node->_gaicbRequest[0] == NULL) {
    node->_gaicbRequest[0] =
        (struct gaicb *)malloc(sizeof(*node->_gaicbRequest[0]));
  }

  if (node->_gaicbRequest[0] == NULL) {
    LOG_ERROR("Node(%p) malloc _gaicbRequest failed.", arg);
  } else {
    memset(node->_gaicbRequest[0], 0, sizeof(*node->_gaicbRequest[0]));
    node->_gaicbRequest[0]->ar_name = node->_nodename;

    int err = getaddrinfo_a(GAI_NOWAIT, node->_gaicbRequest, 1, NULL);
    if (err) {
      LOG_ERROR("Node(%p) getaddrinfo_a failed, err:%d(%s).", arg, err,
                gai_strerror(err));
    } else {
      /* check this node is alive. */
      if (node->_request == NULL) {
        LOG_ERROR("The request of this node(%p) is nullptr.", node);
        goto dnsExit;
      }
      NlsClientImpl *instance = node->getInstance();
      NlsNodeManager *node_manager = instance->getNodeManger();
      int status = NodeStatusInvalid;
      int result = node_manager->checkNodeExist(node, &status);
      if (result != Success) {
        LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", node, result);
        goto dnsExit;
      }

      time_t timeout_ms =
          node->getRequest()->getRequestParam()->getTimeout();  // ms
      struct timespec outtime;
      utility::TextUtils::GetTimespecFromMs(&outtime, timeout_ms);
      // LOG_DEBUG("Node(%p) ready to gai_suspend.", arg);
      err = gai_suspend(node->_gaicbRequest, 1, &outtime);
      // LOG_DEBUG("Node(%p) gai_suspend finish, err:%d(%s).", node, err,
      // gai_strerror(err));
      if (node->_dnsThreadExit) {
        LOG_WARN("Node(%p) ConnectNode is exiting, skip gai.", node);
        goto dnsExit;
      }
      if (node->_dnsThread == 0) {
        LOG_WARN("Node(%p) ConnectNode has exited, skip gai.", node);
        goto dnsExit;
      }
      if (err) {
        LOG_ERROR("Node(%p) gai_suspend failed, err:%d(%s).", arg, err,
                  gai_strerror(err));
        if (err == EAI_SYSTEM || err == EAI_AGAIN) {
          LOG_ERROR(
              "Node(%p) gai_suspend err:%d is mean timeout. please try again "
              "...",
              arg, err);
        }
      } else {
        /* check this node is alive again after gai_suspend. */
        result = node_manager->checkNodeExist(node, &status);
        if (result != Success) {
          LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", node, result);
          goto dnsExit;
        }

        err = gai_error(node->_gaicbRequest[0]);
        if (err) {
          LOG_WARN("Node(%p) cannot get addrinfo, err:%d(%s).", arg, err,
                   gai_strerror(err));
        } else {
          node->_addrinfo = node->_gaicbRequest[0]->ar_result;
        }
      }
    }

    node->_dnsErrorCode = err;
    if (node->_dnsEvent) {
      event_del(node->_dnsEvent);
      event_assign(node->_dnsEvent, node->_eventThread->_workBase, -1, EV_READ,
                   WorkThread::sysDnsEventCallback, node);
    } else {
      node->_dnsEvent = event_new(node->_eventThread->_workBase, -1, EV_READ,
                                  WorkThread::sysDnsEventCallback, node);
      if (NULL == node->_dnsEvent) {
        LOG_ERROR("Node(%p) new event(_dnsEvent) failed.", node);
        goto dnsExit;
      }
    }
    event_add(node->_dnsEvent, NULL);
    event_active(node->_dnsEvent, EV_READ, 0);
    LOG_INFO("Node(%p) dnsThread(%lu) event_active done.", node,
             pthread_self());
  }

dnsExit:
  if (node->_gaicbRequest[0]) {
    free(node->_gaicbRequest[0]);
    node->_gaicbRequest[0] = NULL;
  }
  node->_dnsThread = 0;
  node->_dnsThreadRunning = false;
  LOG_INFO("Node(%p) dnsThread(%lu) is exited.", node, pthread_self());
  pthread_exit(NULL);
}

/**
 * @brief: 异步方式使用系统的getaddrinfo()方法获得dns
 * @return: 成功则为0, 否则为失败
 */
static int native_getaddrinfo(const char *nodename, const char *servname,
                              evdns_getaddrinfo_cb cb, void *arg) {
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  NlsNodeManager *node_manager = node->getInstance()->getNodeManger();
  int status = NodeStatusInvalid;
  int result = node_manager->checkNodeExist(node, &status);
  if (result != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", node, result);
    return result;
  } else {
    LOG_DEBUG(
        "Node(%p) use native_getaddrinfo, ready to create dnsThread(%lu).",
        node, node->_dnsThread);
  }

  if (node->getExitStatus() == ExitCancel) {
    LOG_WARN("Node(%p) is ExitCancel, skip here ...", node);
    return -(CancelledExitStatus);
  }

  MUTEX_LOCK(node->_mtxNode);

  node->_dnsThreadRunning = true;
  node->_nodename = (char *)nodename;
  node->_servname = (char *)servname;

  if (node->_dnsThread) {
    pthread_join(node->_dnsThread, NULL);
  }
  int err = pthread_create(&node->_dnsThread, NULL,
                           &ConnectNode::async_dns_resolve_thread_fn, arg);
  if (err) {
    node->_dnsThreadRunning = false;
    LOG_ERROR("Node(%p) dnsThread(%lu) create failed.", node, node->_dnsThread);
  } else {
    LOG_DEBUG("Node(%p) dnsThread(%lu) create success.", node,
              node->_dnsThread);
  }

  MUTEX_UNLOCK(node->_mtxNode);
  return err;
}
#endif

/**
 * @brief: 进行DNS解析
 * @return: 成功则为非负, 失败则为负值
 */
int ConnectNode::dnsProcess(int aiFamily, char *directIp, bool sysGetAddr) {
  EXIT_CANCEL_CHECK(_exitStatus, this);
  struct evutil_addrinfo hints;
  NlsNodeManager *node_manager = _instance->getNodeManger();
  int status = NodeStatusInvalid;
  int result = node_manager->checkNodeExist(this, &status);
  if (result != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", this, result);
    return result;
  }

  /* 当node处于长链接模式且已经链接, 无需进入握手阶段, 直接进入starting阶段. */
  if (_isLongConnection && _isConnected) {
    LOG_DEBUG(
        "Node(%p) has connected, current is longConnection and connected.",
        this);
    _workStatus = NodeStarting;
    node_manager->updateNodeStatus(this, NodeStatusRunning);
    if (_request->getRequestParam()->_requestType == SpeechTextDialog) {
      addCmdDataBuffer(CmdTextDialog);
    } else {
      addCmdDataBuffer(CmdStart);
    }
    result = nlsSendFrame(getCmdEvBuffer());
    if (result < 0) {
      LOG_ERROR("Node(%p) response failed, result:%d.", this, result);
      handlerTaskFailedEvent(getErrorMsg());
      closeConnectNode();
      return result;
    }
    return Success;
  }

  /* 尝试链接校验 */
  if (!checkConnectCount()) {
    LOG_ERROR("Node(%p) restart connect failed.", this);
    handlerTaskFailedEvent(TASKFAILED_CONNECT_JSON_STRING, SysConnectFailed);
    return -(ConnectFailed);
  }

  _workStatus = NodeConnecting;
  node_manager->updateNodeStatus(this, NodeStatusConnecting);

  if (!parseUrlInformation(directIp)) {
    return -(ParseUrlFailed);
  }

  _url._enableSysGetAddr = sysGetAddr;
  if (_url._isSsl) {
    LOG_INFO("Node(%p) _url._isSsl is True, _url._enableSysGetAddr is %s.",
             this, _url._enableSysGetAddr ? "True" : "False");
  } else {
    LOG_INFO("Node(%p) _url._isSsl is False, _url._enableSysGetAddr is %s.",
             this, _url._enableSysGetAddr ? "True" : "False");
  }

  if (_url._directIp) {
    LOG_INFO("Node(%p) _url._directIp is True.", this);
    WorkThread::directConnect(this, _url._address);
  } else {
    LOG_INFO("Node(%p) _url._directIp is False.", this);

    if (aiFamily != AF_UNSPEC && aiFamily != AF_INET && aiFamily != AF_INET6) {
      LOG_WARN("Node(%p) aiFamily is invalid, use default AF_INET.", this,
               aiFamily);
      aiFamily = AF_INET;
    }

#ifdef ENABLE_DNS_IP_CACHE
    std::string tmp_ip = _eventThread->getIpFromCache(
        (char *)_request->getRequestParam()->_url.c_str());
    if (tmp_ip.length() > 0) {
      LOG_INFO("Node(%p) find IP in cache, connect directly.", this);
      // 从dns cache中获得IP进行直连
      WorkThread::directConnect(this, (char *)tmp_ip.c_str());
    } else
#endif
    {
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = aiFamily;
      hints.ai_flags = EVUTIL_AI_CANONNAME;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;

      LOG_INFO("Node(%p) dns url:%s, enableSysGetAddr:%s.", this,
               _request->getRequestParam()->_url.c_str(),
               _url._enableSysGetAddr ? "True" : "False");

      if (_url._enableSysGetAddr) {
#ifdef __LINUX__
        /*
         * 在内部ws协议下或者主动使用系统getaddrinfo_a的情况下,
         * 使用系统的getaddrinfo_a()
         */
        result = native_getaddrinfo(_url._host, NULL,
                                    WorkThread::dnsEventCallback, this);
        if (result != Success) {
          result = -(GetAddrinfoFailed);
        }
#else
        if (NULL == _eventThread || NULL == _eventThread->_dnsBase) {
          LOG_ERROR("Node:%p dns source is invalid.", this);
          return -(InvalidDnsSource);
        }
        _dnsRequest =
            evdns_getaddrinfo(_eventThread->_dnsBase, _url._host, NULL, &hints,
                              WorkThread::dnsEventCallback, this);
#endif
      } else {
        if (NULL == _eventThread || NULL == _eventThread->_dnsBase) {
          LOG_ERROR("Node(%p) dns source is invalid.", this);
          return -(InvalidDnsSource);
        }
        _dnsRequest =
            evdns_getaddrinfo(_eventThread->_dnsBase, _url._host, NULL, &hints,
                              WorkThread::dnsEventCallback, this);
        if (_dnsRequest == NULL) {
          LOG_ERROR("Node:%p dnsRequest evdns_getaddrinfo failed!", this);
          /*
           * No need to free user_data ordecrement n_pending_requests; that
           * happened in the callback.
           */
          return -(InvalidDnsSource);
        }
      }
    }
  }

  return result;
}

/**
 * @brief: 开始进行链接处理, 创建socket, 设置libevent, 并开始socket链接.
 * @return: 成功则为0, 阻塞则为1, 失败则负值.
 */
int ConnectNode::connectProcess(const char *ip, int aiFamily) {
  EXIT_CANCEL_CHECK(_exitStatus, this);
  evutil_socket_t sockFd = socket(aiFamily, SOCK_STREAM, 0);
  if (sockFd < 0) {
    LOG_ERROR("Node(%p) socket failed. aiFamily:%d, sockFd:%d. error mesg:%s.",
              this, aiFamily, sockFd,
              evutil_socket_error_to_string(evutil_socket_geterror(sockFd)));
    return -(SocketFailed);
  } else {
    // LOG_DEBUG("Node(%p) socket success, sockFd:%d.", this, sockFd);
  }

  struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = 0;
  if (setsockopt(sockFd, SOL_SOCKET, SO_LINGER, (char *)&so_linger,
                 sizeof(struct linger)) < 0) {
    LOG_ERROR("Node(%p) set SO_LINGER failed.", this);
    return -(SetSocketoptFailed);
  }

  if (evutil_make_socket_nonblocking(sockFd) < 0) {
    LOG_ERROR("Node(%p) evutil_make_socket_nonblocking failed.", this);
    return -(EvutilSocketFalied);
  }

  LOG_INFO("Node(%p) new socket ip:%s port:%d Fd:%d.", this, ip, _url._port,
           sockFd);

  short events = EV_READ | EV_WRITE | EV_TIMEOUT | EV_FINALIZE;
  // LOG_DEBUG("Node(%p) set events(%d) for connectEventCallback.", this,
  // events);
  if (NULL == _connectEvent) {
    _connectEvent = event_new(_eventThread->_workBase, sockFd, events,
                              WorkThread::connectEventCallback, this);
    if (NULL == _connectEvent) {
      LOG_ERROR("Node(%p) new event(_connectEvent) failed.", this);
      return -(EventEmpty);
    }
  } else {
    event_del(_connectEvent);
    event_assign(_connectEvent, _eventThread->_workBase, sockFd, events,
                 WorkThread::connectEventCallback, this);
  }

#ifdef ENABLE_HIGH_EFFICIENCY
  if (NULL == _connectTimerEvent) {
    _connectTimerEvent = evtimer_new(
        _eventThread->_workBase, WorkThread::connectTimerEventCallback, this);
    if (NULL == _connectTimerEvent) {
      LOG_ERROR("Node(%p) new event(_connectTimerEvent) failed.", this);
      return -(EventEmpty);
    }
  }
#endif

  if (_enableRecvTv) {
    events = EV_READ | EV_TIMEOUT | EV_PERSIST | EV_FINALIZE;
  } else {
    events = EV_READ | EV_PERSIST | EV_FINALIZE;
  }
  // LOG_DEBUG("Node(%p) set events(%d) for readEventCallback", this, events);
  if (NULL == _readEvent) {
    _readEvent = event_new(_eventThread->_workBase, sockFd, events,
                           WorkThread::readEventCallBack, this);
    if (NULL == _readEvent) {
      LOG_ERROR("Node(%p) new event(_readEvent) failed.", this);
      return -(EventEmpty);
    }
  } else {
    event_del(_readEvent);
    event_assign(_readEvent, _eventThread->_workBase, sockFd, events,
                 WorkThread::readEventCallBack, this);
  }

  events = EV_WRITE | EV_TIMEOUT | EV_FINALIZE;
  // LOG_DEBUG("Node(%p) set events(%d) for writeEventCallback", this, events);
  if (NULL == _writeEvent) {
    _writeEvent = event_new(_eventThread->_workBase, sockFd, events,
                            WorkThread::writeEventCallBack, this);
    if (NULL == _writeEvent) {
      LOG_ERROR("Node(%p) new event(_writeEvent) failed.", this);
      return -(EventEmpty);
    }
  } else {
    event_del(_writeEvent);
    event_assign(_writeEvent, _eventThread->_workBase, sockFd, events,
                 WorkThread::writeEventCallBack, this);
  }

  _aiFamily = aiFamily;
  if (aiFamily == AF_INET) {
    memset(&_addrV4, 0, sizeof(_addrV4));
    _addrV4.sin_family = AF_INET;
    _addrV4.sin_port = htons(_url._port);

    if (inet_pton(AF_INET, ip, &_addrV4.sin_addr) <= 0) {
      LOG_ERROR("Node(%p) IpV4 inet_pton failed.", this);
      evutil_closesocket(sockFd);
      return -(SetSocketoptFailed);
    }
  } else if (aiFamily == AF_INET6) {
    memset(&_addrV6, 0, sizeof(_addrV6));
    _addrV6.sin6_family = AF_INET6;
    _addrV6.sin6_port = htons(_url._port);

    if (inet_pton(AF_INET6, ip, &_addrV6.sin6_addr) <= 0) {
      LOG_ERROR("Node(%p) IpV6 inet_pton failed.", this);
      evutil_closesocket(sockFd);
      return -(SetSocketoptFailed);
    }
  }

  _socketFd = sockFd;

  return socketConnect();
}

/**
 * @brief:进行socket链接
 * @return: 成功则为0, 阻塞则为1, 失败则负值.
 */
int ConnectNode::socketConnect() {
  int retCode = 0;
  NlsClientImpl *client = _instance;
  NlsNodeManager *node_manager = client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(this, &status);
  if (ret != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, ret:%d.", this, ret);
    return ret;
  }

  if (ExitCancel == _exitStatus) {
    LOG_WARN("Node(%p) current ExitCancel, skip sslProcess.", this);
    return -(InvalidExitStatus);
  }

  if (_aiFamily == AF_INET) {
    retCode = connect(_socketFd, (const sockaddr *)&_addrV4, sizeof(_addrV4));
  } else {
    retCode = connect(_socketFd, (const sockaddr *)&_addrV6, sizeof(_addrV6));
  }

  if (retCode == -1) {
    int connectErrCode = utility::getLastErrorCode();
    if (NLS_ERR_CONNECT_RETRIABLE(connectErrCode)) {
      /*
       *  connectErrCode == 115(EINPROGRESS)
       *  means connection is in progress,
       *  normally the socket connecting timeout is 75s.
       *  after the socket fd is ready to read.
       */

      /* 开启用于链接的libevent */
      if (NULL == _connectEvent) {
        LOG_ERROR("Node(%p) event is nullptr.", this);
        return -(EventEmpty);
      }
      time_t timeout_ms = _request->getRequestParam()->getTimeout();
      utility::TextUtils::GetTimevalFromMs(&_connectTv, timeout_ms);
      event_add(_connectEvent, &_connectTv);

      LOG_DEBUG("Node(%p) will connect later, errno:%d. timeout:%ldms.", this,
                connectErrCode, timeout_ms);
      return 1;
    } else {
      LOG_ERROR(
          "Node(%p) connect failed:%s. retCode:%d connectErrCode:%d. retry ...",
          this,
          evutil_socket_error_to_string(evutil_socket_geterror(_socketFd)),
          retCode, connectErrCode);

#ifdef ENABLE_DNS_IP_CACHE
      std::string tmp_ip = _eventThread->getIpFromCache(
          (char *)_request->getRequestParam()->_url.c_str());
      if (tmp_ip.length() == 0) {
        LOG_ERROR("Node(%p) connect failed, clear IpCache, will dns later ...",
                  this);
        _eventThread->setIpCache(NULL, NULL);
      }
#endif

      return -(SocketConnectFailed);
    }
  } else {
    LOG_DEBUG("Node(%p) connected directly. retCode:%d.", this, retCode);
    _workStatus = NodeConnected;
    node_manager->updateNodeStatus(this, NodeStatusConnected);
    _isConnected = true;
  }
  return Success;
}

/**
 * @brief: 进行SSL握手
 * @return: 成功 等待链接则返回1, 正在握手则返回0, 失败则返回负值
 */
int ConnectNode::sslProcess() {
  EXIT_CANCEL_CHECK(_exitStatus, this);
  int ret = 0;
  NlsClientImpl *client = _instance;
  NlsNodeManager *node_manager = client->getNodeManger();
  int status = NodeStatusInvalid;
  ret = node_manager->checkNodeExist(this, &status);
  if (ret != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, ret:%d", this, ret);
    return ret;
  }

  if (_url._isSsl) {
    ret = _sslHandle->sslHandshake(_socketFd, _url._host);
    if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE) {
// current status:NodeConnected
// LOG_DEBUG("Node(%p) status:%s, wait ssl process ...",
//     this, getConnectNodeStatusString().c_str());

// trigger connectEventCallback()
#ifdef ENABLE_HIGH_EFFICIENCY
      // connect回调和定时connect回调交替调用, 降低事件量的同时保证稳定
      if (!_connectTimerFlag) {
        if (NULL == _connectTimerEvent) {
          LOG_ERROR("Node(%p) event is nullptr.", this);
          return -(EventEmpty);
        }

        // LOG_DEBUG("Node(%p) add evtimer _connectTimerEvent.", this);
        evtimer_add(_connectTimerEvent, &_connectTimerTv);
        _connectTimerFlag = true;
      } else {
        if (NULL == _connectEvent) {
          LOG_ERROR("Node(%p) event is nullptr.", this);
          return -(EventEmpty);
        }

        // LOG_DEBUG("Node(%p) add events _connectEvent.", this);
        utility::TextUtils::GetTimevalFromMs(
            &_connectTv, _request->getRequestParam()->getTimeout());
        // set _connectEvent to pending status.
        event_add(_connectEvent, &_connectTv);
        _connectTimerFlag = false;
      }
#else
      if (NULL == _connectEvent) {
        LOG_ERROR("Node(%p) event is nullptr.", this);
        return -(EventEmpty);
      }
      utility::TextUtils::GetTimevalFromMs(
          &_connectTv, _request->getRequestParam()->getTimeout());
      event_add(_connectEvent, &_connectTv);
#endif

      return 1;
    } else if (ret < 0) {
      _nodeErrMsg = _sslHandle->getFailedMsg();
      LOG_ERROR("Node(%p) sslHandshake failed, %s.", this, _nodeErrMsg.c_str());
      return -(SslHandshakeFailed);
    } else {
      _workStatus = NodeHandshaking;
      node_manager->updateNodeStatus(this, NodeStatusHandshaking);
      LOG_DEBUG("Node(%p) sslHandshake done, ret:%d, set node:NodeHandshaking.",
                this, ret);
      return 0;
    }
  } else {
    _workStatus = NodeHandshaking;
    node_manager->updateNodeStatus(this, NodeStatusHandshaking);
    LOG_INFO("Node(%p) it's not ssl process, set node:NodeHandshaking.", this);
  }

  return 0;
}

/**
 * @brief: 初始化音频编码器
 * @return:
 */
void ConnectNode::initNlsEncoder() {
  MUTEX_LOCK(_mtxNode);

  if (NULL == _nlsEncoder) {
    if (NULL == _request) {
      LOG_ERROR("The request of node(%p) is nullptr.", this);
      MUTEX_UNLOCK(_mtxNode);
      return;
    }

    if (_request->getRequestParam()->_format == "opu") {
      _encoderType = ENCODER_OPU;
    } else if (_request->getRequestParam()->_format == "opus") {
      _encoderType = ENCODER_OPUS;
    }

    if (_encoderType != ENCODER_NONE) {
      _nlsEncoder = new NlsEncoder();
      if (NULL == _nlsEncoder) {
        LOG_ERROR("Node(%p) new _nlsEncoder failed.", this);
        MUTEX_UNLOCK(_mtxNode);
        return;
      }

      int errorCode = 0;
      int ret = _nlsEncoder->createNlsEncoder(
          _encoderType, 1, _request->getRequestParam()->_sampleRate,
          &errorCode);
      if (ret < 0) {
        LOG_ERROR("Node(%p) createNlsEncoder failed, errcode:%d.", this,
                  errorCode);
        delete _nlsEncoder;
        _nlsEncoder = NULL;
      }
    }
  }

  MUTEX_UNLOCK(_mtxNode);
}

/**
 * @brief: 更新并生效所有ConnectNode中设置的参数
 * @return:
 */
void ConnectNode::updateParameters() {
  MUTEX_LOCK(_mtxNode);

  if (_request->getRequestParam()->_sampleRate == SampleRate16K) {
    _limitSize = Buffer16kMaxLimit;
  } else {
    _limitSize = Buffer8kMaxLimit;
  }

  time_t timeout_ms = _request->getRequestParam()->getTimeout();
  utility::TextUtils::GetTimevalFromMs(&_connectTv, timeout_ms);
  LOG_INFO("Node(%p) set connect timeout: %ldms.", this, timeout_ms);

  _enableRecvTv = _request->getRequestParam()->getEnableRecvTimeout();
  if (_enableRecvTv) {
    LOG_INFO("Node(%p) enable recv timeout.", this);
  } else {
    LOG_INFO("Node(%p) disable recv timeout.", this);
  }
  timeout_ms = _request->getRequestParam()->getRecvTimeout();
  utility::TextUtils::GetTimevalFromMs(&_recvTv, timeout_ms);
  LOG_INFO("Node(%p) set recv timeout: %ldms.", this, timeout_ms);

  timeout_ms = _request->getRequestParam()->getSendTimeout();
  utility::TextUtils::GetTimevalFromMs(&_sendTv, timeout_ms);
  LOG_INFO("Node(%p) set send timeout: %ldms.", this, timeout_ms);

  _enableOnMessage = _request->getRequestParam()->getEnableOnMessage();
  if (_enableOnMessage) {
    LOG_INFO("Node(%p) enable OnMessage Callback.", this);
  } else {
    LOG_INFO("Node(%p) disable OnMessage Callback.", this);
  }

  MUTEX_UNLOCK(_mtxNode);
}

/**
 * @brief: 等待所有EventCallback退出并删除和停止EventCallback的队列
 * @return:
 */
void ConnectNode::delAllEvents() {
  LOG_DEBUG(
      "Node(%p) delete all events begin, current node status:%s exit "
      "status:%s.",
      this, getConnectNodeStatusString().c_str(),
      getExitStatusString().c_str());

  /* 当Node处于Invoking状态, 即还未进入正式运行,
   * 需要等待其进入Invoked才可进行操作 */
  int try_count = 2000;
  while (_workStatus == NodeInvoking && try_count-- > 0) {
#ifdef _MSC_VER
    Sleep(1);
#else
    usleep(1000);
#endif
  }
  if (try_count <= 0) {
    LOG_WARN("Node(%p) waiting exit NodeInvoking failed.", this);
  } else {
    LOG_WARN("Node(%p) waiting exit NodeInvoking success.", this);
  }

  /* 当Node处于异步dns线程状态, 通知线程退出并等待其退出再进行释放 */
  if (_url._enableSysGetAddr && _dnsThreadRunning) {
    _dnsThreadExit = true;
    try_count = 5000;
    LOG_WARN("Node(%p) waiting exit dnsThread ...", this);
    while (_dnsThreadRunning && try_count-- > 0) {
#ifdef _MSC_VER
      Sleep(1);
#else
      usleep(1000);
#endif
    }
    if (try_count <= 0) {
      LOG_WARN("Node(%p) waiting exit dnsThread failed.", this);
    } else {
      LOG_WARN("Node(%p) waiting exit dnsThread success.", this);
    }
  }

  waitEventCallback();

  bool del_all_events_lock_ret = true;
  MUTEX_TRY_LOCK(_mtxCloseNode, 2000, del_all_events_lock_ret);
  if (!del_all_events_lock_ret) {
    LOG_ERROR("Node(%p) delAllEvents, deadlock has occurred", this);
  }

  if (_dnsEvent) {
    event_del(_dnsEvent);
    event_free(_dnsEvent);
    _dnsEvent = NULL;
  }
  if (_readEvent) {
    event_del(_readEvent);
    event_free(_readEvent);
    _readEvent = NULL;
  }
  if (_writeEvent) {
    event_del(_writeEvent);
    event_free(_writeEvent);
    _writeEvent = NULL;
  }
  if (_connectEvent) {
    event_del(_connectEvent);
    event_free(_connectEvent);
    _connectEvent = NULL;
  }
#ifdef ENABLE_HIGH_EFFICIENCY
  if (_connectTimerEvent != NULL) {
    if (_connectTimerFlag) {
      evtimer_del(_connectTimerEvent);
      _connectTimerFlag = false;
    }
    event_free(_connectTimerEvent);
    _connectTimerEvent = NULL;
  }
#endif

  if (del_all_events_lock_ret) {
    MUTEX_UNLOCK(_mtxCloseNode);
  }

  waitEventCallback();

  LOG_DEBUG("Node(%p) delete all events done.", this);
}

/**
 * @brief: 等待所有EventCallback退出, 默认1s超时.
 * @return:
 */
void ConnectNode::waitEventCallback() {
  // LOG_DEBUG("Node:%p waiting EventCallback, current node status:%s exit
  // status:%s",
  //     this,
  //     getConnectNodeStatusString().c_str(),
  //     getExitStatusString().c_str());

  if (_inEventCallbackNode) {
    LOG_DEBUG("Node(%p) waiting EventCallback ...", this);

#if defined(_MSC_VER)
    MUTEX_LOCK(_mtxEventCallbackNode);
#else
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t time_ms =
        now.tv_sec * 1000 + now.tv_usec / 1000 + 1000;  // plus timeout 1000ms
    utility::TextUtils::GetTimespecFromMs(&outtime, time_ms);
    MUTEX_LOCK(_mtxEventCallbackNode);
    if (ETIMEDOUT == pthread_cond_timedwait(&_cvEventCallbackNode,
                                            &_mtxEventCallbackNode, &outtime)) {
      LOG_WARN("Node(%p) waiting EventCallback timeout.", this);
      _waitEventCallbackAbnormally = true;
    }
    MUTEX_UNLOCK(_mtxEventCallbackNode);
#endif
    LOG_DEBUG("Node(%p) waiting EventCallback done with abnormal flag(%s).",
              this, _waitEventCallbackAbnormally ? "true" : "false");
  }

  // LOG_DEBUG("Node(%p) wait all EventCallback exit done.", this);
}

/**
 * @brief: 等待调用完成, 默认10s超时.
 * @return:
 */
void ConnectNode::waitInvokeFinish() {
  if (_syncCallTimeoutMs > 0) {
    LOG_DEBUG("Node(%p) waiting invoke finish, timeout %dms...", this,
              _syncCallTimeoutMs);

#if defined(_MSC_VER)
    MUTEX_LOCK(_mtxInvokeSyncCallNode);
#else
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&now, NULL);
    uint64_t time_ms =
        now.tv_sec * 1000 + now.tv_usec / 1000 + _syncCallTimeoutMs;
    utility::TextUtils::GetTimespecFromMs(&outtime, time_ms);
    MUTEX_LOCK(_mtxInvokeSyncCallNode);
    if (ETIMEDOUT == pthread_cond_timedwait(&_cvInvokeSyncCallNode,
                                            &_mtxInvokeSyncCallNode,
                                            &outtime)) {
      _nodeErrCode = InvokeTimeout;
      LOG_WARN("Node(%p) waiting invoke timeout.", this);
    }
    MUTEX_UNLOCK(_mtxInvokeSyncCallNode);
#endif

    LOG_DEBUG("Node(%p) waiting invoke done.", this);
  }
}

/**
 * @brief: 发送调用完成的信号.
 * @return:
 */
void ConnectNode::sendFinishCondSignal(NlsEvent::EventType eventType) {
  if (_syncCallTimeoutMs > 0) {
    if (eventType == NlsEvent::Close ||
        eventType == NlsEvent::RecognitionStarted ||
        eventType == NlsEvent::TranscriptionStarted ||
        eventType == NlsEvent::SynthesisStarted) {
      LOG_DEBUG("Node(%p) send finish cond signal, event type:%d.", this,
                eventType);

      bool useless_flag = false;
#ifdef _MSC_VER
      SET_EVENT(useless_flag, _mtxInvokeSyncCallNode);
#else
      SEND_COND_SIGNAL(_mtxInvokeSyncCallNode, _cvInvokeSyncCallNode,
                       useless_flag);
#endif
    }
  }
}

/**
 * @brief: 生成closed事件信息, 填入任务相关信息比如task_id
 * @return:
 */
const char *ConnectNode::genCloseMsg(std::string *buf_str) {
  if (_request == NULL) {
    return NULL;
  }

  Json::Value root;
  Json::FastWriter writer;
  std::string task_id = _request->getRequestParam()->_task_id;

  try {
    root["channelClosed"] = "nls request finished.";
    if (!task_id.empty()) {
      root["task_id"] = task_id;
    }

    *buf_str = writer.write(root);
    if (buf_str->empty()) {
      return CLOSE_JSON_STRING;
    } else {
      return buf_str->c_str();
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return CLOSE_JSON_STRING;
  }
}

/**
 * @brief: 生成SynthesisStarted事件信息, 填入任务相关信息比如task_id
 * @return:
 */
const char *ConnectNode::genSynthesisStartedMsg() {
  if (_request == NULL) {
    return NULL;
  }

  /* fake:
   * {
   *   "header":{
   *     "namespace":"SpeechSynthesizer",
   *     "name":"SynthesisStarted",
   *     "status":20000000,
   *     "message_id":"94b682af3ee549349d25085e76d53610",
   *     "task_id":"0d528c2bdba942689fd291b6b7760fd2",
   *     "status_text":"Gateway:SUCCESS:Success."
   *   }
   * }
   */
  std::string fake_cmd = "";
  Json::Value root, header;
  Json::FastWriter writer;
  std::string task_id = _request->getRequestParam()->getTaskId();
  try {
    header["namespace"] = "SpeechSynthesizer";
    header["name"] = "SynthesisStarted";
    header["status"] = 20000000;
    header["task_id"] = task_id;
    header["status_text"] = "Gateway:SUCCESS:Success.";
    root["header"] = header;
    fake_cmd = writer.write(root);
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  LOG_DEBUG("Node(%p) send %s to user", this, fake_cmd.c_str());
  return fake_cmd.c_str();
}

#ifdef ENABLE_REQUEST_RECORDING
void ConnectNode::updateNodeProcess(std::string api, int status, bool enter,
                                    size_t size) {
  if (api == "start") {
    if (enter) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.start_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
      _nodeProcess.last_status = NodeInvoking;
      _nodeProcess.api_start_run = true;
      _nodeProcess.last_api_timestamp_ms = utility::TextUtils::GetTimestampMs();
    } else {
      _nodeProcess.api_start_run = false;
    }
  } else if (api == "stop") {
    if (enter) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.stop_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
      _nodeProcess.last_status = NodeStop;
      _nodeProcess.api_stop_run = true;
      _nodeProcess.last_api_timestamp_ms = utility::TextUtils::GetTimestampMs();
    } else {
      _nodeProcess.api_stop_run = false;
    }
  } else if (api == "cancel") {
    if (enter) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.cancel_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
      _nodeProcess.last_status = NodeCancel;
      _nodeProcess.api_cancel_run = true;
      _nodeProcess.last_api_timestamp_ms = utility::TextUtils::GetTimestampMs();
    } else {
      _nodeProcess.api_cancel_run = false;
    }
  } else if (api == "sendAudio") {
    if (enter) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.last_send_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
      _nodeProcess.last_status = NodeSendAudio;
      _nodeProcess.recording_bytes += size;
      _nodeProcess.send_count++;
      _nodeProcess.api_send_run = true;
      _nodeProcess.last_api_timestamp_ms = utility::TextUtils::GetTimestampMs();
    } else {
      _nodeProcess.api_send_run = false;
    }
  } else if (api == "ctrl") {
    if (enter) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.last_ctrl_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
      _nodeProcess.last_status = NodeSendControl;
      _nodeProcess.api_ctrl_run = true;
      _nodeProcess.last_api_timestamp_ms = utility::TextUtils::GetTimestampMs();
    } else {
      _nodeProcess.api_ctrl_run = false;
    }
  } else if (api == "send_text") {
    if (enter) {
      _nodeProcess.last_op_timestamp_ms = utility::TextUtils::GetTimestampMs();
      _nodeProcess.last_status = NodeSendText;
      _nodeProcess.api_ctrl_run = true;
      _nodeProcess.last_api_timestamp_ms = utility::TextUtils::GetTimestampMs();
    } else {
      _nodeProcess.api_ctrl_run = false;
    }
  } else if (api == "callback") {
    if (enter) {
      _nodeProcess.last_callback = (NlsEvent::EventType)status;
      switch (_nodeProcess.last_callback) {
        case NlsEvent::RecognitionStarted:
        case NlsEvent::TranscriptionStarted:
          _nodeProcess.last_op_timestamp_ms =
              utility::TextUtils::GetTimestampMs();
          _nodeProcess.started_timestamp_ms = _nodeProcess.last_op_timestamp_ms;
          break;
        case NlsEvent::TranscriptionCompleted:
        case NlsEvent::RecognitionCompleted:
        case NlsEvent::SynthesisCompleted:
          _nodeProcess.last_op_timestamp_ms =
              utility::TextUtils::GetTimestampMs();
          _nodeProcess.completed_timestamp_ms =
              _nodeProcess.last_op_timestamp_ms;
          break;
        case NlsEvent::Binary:
          _nodeProcess.last_op_timestamp_ms =
              utility::TextUtils::GetTimestampMs();
          _nodeProcess.play_bytes += size;
          _nodeProcess.play_count++;
          _nodeProcess.last_status = NodePlayAudio;
          if (_isFirstBinaryFrame) {
            _nodeProcess.first_binary_timestamp_ms =
                _nodeProcess.last_op_timestamp_ms;
          }
          break;
      }
      _nodeProcess.last_cb_start_timestamp_ms =
          utility::TextUtils::GetTimestampMs();
      _nodeProcess.last_cb_end_timestamp_ms = 0;
      _nodeProcess.last_cb_run = true;
    } else {
      _nodeProcess.last_cb_end_timestamp_ms =
          utility::TextUtils::GetTimestampMs();
      _nodeProcess.last_cb_run = false;
    }
  }
}

const char *ConnectNode::dumpAllInfo() {
  try {
    Json::Value root(Json::objectValue);
    Json::Value request_process(Json::objectValue);
    Json::Value timestamp(Json::objectValue);
    Json::Value last(Json::objectValue);
    Json::Value data(Json::objectValue);
    Json::Value callback(Json::objectValue);
    Json::Value block(Json::objectValue);
    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";

    timestamp = updateNodeProcess4Timestamp();
    last = updateNodeProcess4Last();
    data = updateNodeProcess4Data();
    callback = updateNodeProcess4Callback();
    block = updateNodeProcess4Block();

    if (!timestamp.isNull()) {
      root["timestamp"] = timestamp;
    }
    if (!last.isNull()) {
      root["last"] = last;
    }
    if (!data.isNull()) {
      root["data"] = data;
    }
    if (!callback.isNull()) {
      root["callback"] = callback;
    }
    if (!block.isNull() && block.isObject() && !block.empty()) {
      root["block"] = block;
    }
    root["sdkversion"] = NLS_SDK_VERSION_STR;

#ifdef ENABLE_CONTINUED
    Json::Value reconnection(Json::objectValue);
    reconnection = updateNodeReconnection();
    if (!reconnection.isNull() && reconnection.isObject() &&
        !reconnection.empty()) {
      root["reconnection"] = reconnection;
    }
#endif
    std::string info = Json::writeString(writerBuilder, root);
    LOG_INFO("current info: %s", info.c_str());
    return info.c_str();
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return "";
  }
}

/**
 * @brief: 在closed事件或TaskFailed(由SDK引发)事件中填入此请求的记录
 * @return:
 */
std::string ConnectNode::replenishNodeProcess(const char *error) {
  if (error == NULL) {
    return "";
  }

  try {
    Json::Reader reader;
    Json::Value root(Json::objectValue);
    if (!reader.parse(error, root)) {
      return "";
    } else {
      Json::Value request_process(Json::objectValue);
      Json::Value timestamp(Json::objectValue);
      Json::Value last(Json::objectValue);
      Json::Value data(Json::objectValue);
      Json::Value callback(Json::objectValue);
      Json::Value block(Json::objectValue);
      Json::StreamWriterBuilder writerBuilder;
      writerBuilder["indentation"] = "";

      timestamp = updateNodeProcess4Timestamp();
      last = updateNodeProcess4Last();
      data = updateNodeProcess4Data();
      callback = updateNodeProcess4Callback();
      block = updateNodeProcess4Block();

      if (!timestamp.isNull()) {
        root["timestamp"] = timestamp;
      }
      if (!last.isNull()) {
        root["last"] = last;
      }
      if (!data.isNull()) {
        root["data"] = data;
      }
      if (!callback.isNull()) {
        root["callback"] = callback;
      }
      if (!block.isNull() && block.isObject() && !block.empty()) {
        root["block"] = block;
      }
      root["sdkversion"] = NLS_SDK_VERSION_STR;

#ifdef ENABLE_CONTINUED
      Json::Value reconnection(Json::objectValue);
      reconnection = updateNodeReconnection();
      if (!reconnection.isNull() && reconnection.isObject() &&
          !reconnection.empty()) {
        root["reconnection"] = reconnection;
      }
#endif
      return Json::writeString(writerBuilder, root);
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return "";
  }
}

Json::Value ConnectNode::updateNodeProcess4Data() {
  Json::Value data(Json::objectValue);
  try {
    if (_nodeProcess.recording_bytes > 0) {
      data["recording_bytes"] = (Json::UInt64)_nodeProcess.recording_bytes;
    }
    if (_nodeProcess.send_count > 0) {
      data["send_count"] = (Json::UInt64)_nodeProcess.send_count;
    }
    if (_nodeProcess.play_bytes > 0) {
      data["play_bytes"] = (Json::UInt64)_nodeProcess.play_bytes;
    }
    if (_nodeProcess.play_count > 0) {
      data["play_count"] = (Json::UInt64)_nodeProcess.play_count;
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return data;
  }
  return data;
}

Json::Value ConnectNode::updateNodeProcess4Last() {
  Json::Value last(Json::objectValue);
  try {
    last["status"] = getConnectNodeStatusString(_nodeProcess.last_status);
    if (_nodeProcess.last_send_timestamp_ms > 0) {
      last["send"] = utility::TextUtils::GetTimeFromMs(
          _nodeProcess.last_send_timestamp_ms);
    }
    if (_nodeProcess.last_ctrl_timestamp_ms > 0) {
      last["control"] = utility::TextUtils::GetTimeFromMs(
          _nodeProcess.last_ctrl_timestamp_ms);
    }
    if (_nodeProcess.last_op_timestamp_ms > 0) {
      last["action"] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.last_op_timestamp_ms);
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return last;
  }
  return last;
}

Json::Value ConnectNode::updateNodeProcess4Timestamp() {
  Json::Value timestamp(Json::objectValue);
  try {
    timestamp["create"] =
        utility::TextUtils::GetTimeFromMs(_nodeProcess.create_timestamp_ms);
    if (_nodeProcess.start_timestamp_ms > 0) {
      timestamp["start"] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.start_timestamp_ms);
    }
    if (_nodeProcess.started_timestamp_ms > 0) {
      timestamp["started"] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.started_timestamp_ms);
    }
    if (_nodeProcess.stop_timestamp_ms > 0) {
      timestamp["stop"] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.stop_timestamp_ms);
    }
    if (_nodeProcess.cancel_timestamp_ms > 0) {
      timestamp["cancel"] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.cancel_timestamp_ms);
    }
    if (_nodeProcess.failed_timestamp_ms > 0) {
      timestamp["failed"] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.failed_timestamp_ms);
    }
    if (_nodeProcess.completed_timestamp_ms > 0) {
      timestamp["completed"] = utility::TextUtils::GetTimeFromMs(
          _nodeProcess.completed_timestamp_ms);
    }
    if (_nodeProcess.closed_timestamp_ms > 0) {
      timestamp["closed"] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.closed_timestamp_ms);
    }
    if (_nodeProcess.first_binary_timestamp_ms > 0) {
      timestamp["first_binary"] = utility::TextUtils::GetTimeFromMs(
          _nodeProcess.first_binary_timestamp_ms);
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return timestamp;
  }
  return timestamp;
}

Json::Value ConnectNode::updateNodeProcess4Callback() {
  Json::Value callback(Json::objectValue);
  try {
    NlsEvent event;
    std::string cb_name = event.getMsgTypeString(_nodeProcess.last_callback);
    if (!cb_name.empty() && cb_name != "Unknown") {
      callback["name"] = cb_name;
      if (_nodeProcess.last_cb_start_timestamp_ms > 0) {
        callback["start"] = utility::TextUtils::GetTimeFromMs(
            _nodeProcess.last_cb_start_timestamp_ms);
      }
      if (_nodeProcess.last_cb_end_timestamp_ms > 0) {
        callback["end"] = utility::TextUtils::GetTimeFromMs(
            _nodeProcess.last_cb_end_timestamp_ms);
      }
      if (_nodeProcess.last_cb_run) {
        callback["status"] = "running";
      }
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return callback;
  }
  return callback;
}

Json::Value ConnectNode::updateNodeProcess4Block() {
  Json::Value block(Json::objectValue);
  try {
    bool running_flag = false;
    std::string timestamp_name = "";
    std::string duration_name = "";
    if (_nodeProcess.api_start_run) {
      block["start"] = "running";
      timestamp_name = "start_timestamp";
      duration_name = "start_duration_ms";
      running_flag = true;
    }
    if (_nodeProcess.api_stop_run) {
      block["stop"] = "running";
      timestamp_name = "stop_timestamp";
      duration_name = "stop_duration_ms";
      running_flag = true;
    }
    if (_nodeProcess.api_cancel_run) {
      block["cancel"] = "running";
      timestamp_name = "cancel_timestamp";
      duration_name = "cancel_duration_ms";
      running_flag = true;
    }
    if (_nodeProcess.api_send_run) {
      block["send"] = "running";
      timestamp_name = "send_timestamp";
      duration_name = "send_duration_ms";
      running_flag = true;
    }
    if (_nodeProcess.api_ctrl_run) {
      block["ctrl"] = "running";
      timestamp_name = "ctrl_timestamp";
      duration_name = "ctrl_duration_ms";
      running_flag = true;
    }
    if (running_flag && _nodeProcess.last_api_timestamp_ms > 0) {
      block[timestamp_name] =
          utility::TextUtils::GetTimeFromMs(_nodeProcess.last_api_timestamp_ms);
      uint64_t current = utility::TextUtils::GetTimestampMs();
      block[duration_name] =
          Json::UInt64(current - _nodeProcess.last_api_timestamp_ms);
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return block;
  }
  return block;
}
#endif

#ifdef ENABLE_CONTINUED
Json::Value ConnectNode::updateNodeReconnection() {
  Json::Value reconnection(Json::objectValue);
  try {
    if (_reconnection.first_audio_timestamp_ms > 0) {
      reconnection["first_audio_timestamp"] = utility::TextUtils::GetTimeFromMs(
          _reconnection.first_audio_timestamp_ms);
    }
    if (_reconnection.interruption_timestamp_ms > 0) {
      reconnection["interruption_timestamp"] =
          utility::TextUtils::GetTimeFromMs(
              _reconnection.interruption_timestamp_ms);
    }
    if (_reconnection.tw_index_offset > 0) {
      reconnection["tw_index_offset"] =
          (Json::UInt64)_reconnection.tw_index_offset;
    }
    if (_reconnection.state != NodeReconnection::NoReconnection) {
      reconnection["should_reconnecting"] = true;
      reconnection["reconnected_count"] = _reconnection.reconnected_count;
    }
  } catch (const std::exception &e) {
    LOG_ERROR("Json failed: %s", e.what());
    return reconnection;
  }
  return reconnection;
}

void ConnectNode::updateTwIndexOffset(NlsEvent *frameEvent) {
  if (frameEvent) {
    if (frameEvent->getMsgType() == NlsEvent::SentenceBegin) {
      _reconnection.tw_index_offset = frameEvent->getSentenceIndex();
    }
  }
}

bool ConnectNode::nodeReconnecting() {
  if (_reconnection.state == NodeReconnection::WillReconnect) {
    struct timeval _reconnectTv;
    utility::TextUtils::GetTimevalFromMs(
        &_reconnectTv, NodeReconnection::reconnect_interval_ms);
    LOG_INFO("reconnect node(%p) after %dms.", this,
             NodeReconnection::reconnect_interval_ms);
    int event_ret = event_add(getReconnectEvent(), &_reconnectTv);
    if (event_ret == Success) {
      LOG_INFO("reconnect node(%p) event_add success.", this);
      _reconnection.state = NodeReconnection::TriggerReconnection;
    } else {
      LOG_WARN("reconnect node(%p) event_add failed with %d.", this, event_ret);
    }
    return true;
  } else if (_reconnection.state == NodeReconnection::TriggerReconnection ||
             _reconnection.state == NodeReconnection::NewReconnectionStarting) {
    LOG_INFO("Node(%p) reconnection has launched(%d)", this,
             _reconnection.state);
  }
  return false;
}

/**
 * @brief: 获得用于重启当前node的libevent
 * @return: libevent的event指针
 */
struct event *ConnectNode::getReconnectEvent() {
  if (_reconnectEvent != NULL) {
    event_del(_reconnectEvent);
    event_free(_reconnectEvent);
    _reconnectEvent = NULL;
  }
  _reconnectEvent = event_new(_eventThread->_workBase, -1, EV_READ | EV_TIMEOUT,
                              WorkThread::launchEventCallback, this);
  if (NULL == _reconnectEvent) {
    LOG_ERROR("Node(%p) new event(_reconnectEvent) failed.", this);
  } else {
    LOG_DEBUG("Node(%p) new event(_reconnectEvent).", this);
  }
  return _reconnectEvent;
}
#endif

void ConnectNode::sendFakeSynthesisStarted() {
  // send SynthesisStarted if speechSynthesizer
  if (_request->getRequestParam()->getNlsRequestType() == SpeechSynthesizer) {
    NlsEvent *useEvent = new NlsEvent(genSynthesisStartedMsg(), Success,
                                      NlsEvent::SynthesisStarted,
                                      _request->getRequestParam()->_task_id);
    if (useEvent) {
      handlerFrame(useEvent);
      delete useEvent;
    }
  }
}

bool ConnectNode::ignoreCallbackWhenReconnecting(NlsEvent::EventType eventType,
                                                 int code) {
#ifdef ENABLE_CONTINUED
  if (_request) {
    if (_request->getRequestParam()->_enableReconnect) {
      if (eventType == NlsEvent::TaskFailed) {
        if ((code >= 50000000 && code < 60000000) || code == DefaultErrorCode ||
            code == SysErrorCode || code == UnknownWsHeadType ||
            code == HttpConnectFailed || code == SysConnectFailed ||
            code == ClientError || code == ConcurrencyExceed) {
          if (_reconnection.state == NodeReconnection::NoReconnection ||
              _reconnection.state ==
                  NodeReconnection::NewReconnectionStarting) {
            _reconnection.interruption_timestamp_ms =
                utility::TextUtils::GetTimestampMs();
            _reconnection.reconnected_count++;
            _reconnection.state = NodeReconnection::WillReconnect;
          }
        }
      }

      if (_reconnection.state == NodeReconnection::WillReconnect ||
          _reconnection.state == NodeReconnection::TriggerReconnection) {
        if (eventType == NlsEvent::TaskFailed || eventType == NlsEvent::Close) {
          if (_reconnection.reconnected_count <=
              NodeReconnection::max_try_count) {
            NlsEvent tmp;
            LOG_INFO(
                "Node(%p) state(%d) get status code is %d with %s, try %d "
                "times, should reconnecting "
                "and "
                "ignore callback.",
                this, _reconnection.state, code,
                tmp.getMsgTypeString(eventType).c_str(),
                _reconnection.reconnected_count);
            return true;
          } else {
            _reconnection.state = NodeReconnection::NoReconnection;
            LOG_INFO("Node(%p) failed %d times, should boot normally.", this,
                     _reconnection.reconnected_count);
          }
        }
      }
    }
  }
#endif
  return false;
}

}  // namespace AlibabaNls
