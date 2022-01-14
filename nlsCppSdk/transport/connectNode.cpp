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

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <iostream>

#if defined(__ANDROID__) || defined(__linux__)
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#ifndef __ANDRIOD__
#include <iconv.h>
#endif
#endif

#ifdef __GNUC__
#include <netdb.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#include "nlsGlobal.h"
#include "iNlsRequest.h"
#include "iNlsRequestParam.h"
#include "nlog.h"
#include "utility.h"
#include "workThread.h"
#include "connectNode.h"

namespace AlibabaNls {

#define NODE_FRAME_SIZE 2048

ConnectNode::ConnectNode(INlsRequest* request,
                         HandleBaseOneParamWithReturnVoid<NlsEvent>* handler) : _request(request), _handler(handler) {

  _connectErrCode = 0;
  _retryConnectCount = 0;

  _socketFd = INVALID_SOCKET;

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

  //int errorCode = 0;
  _nlsEncoder = NULL; //createNlsEncoder
  _encoder_type = ENCODER_NONE;

  _eventThread = NULL;

  _isDestroy = false;
  _isWakeStop = false;
  _isStop = false;

  _workStatus = NodeInitial;
  _exitStatus = ExitInvalid;

  _sslHandle = new SSLconnect();
  if (_sslHandle == NULL) {
    LOG_ERROR("_sslHandle is nullptr");
  }

  _connectTv.tv_sec = LIMIT_CONNECT_TIMEOUT;
  _connectTv.tv_usec = 0;

#if defined(_MSC_VER)
  _mtxNode = CreateMutex(NULL, FALSE, NULL);
  _mtxCloseNode = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxNode, NULL);
  pthread_mutex_init(&_mtxCloseNode, NULL);
#endif

  LOG_DEBUG("Create ConnectNode done.");
}

ConnectNode::~ConnectNode() {
  LOG_DEBUG("Destroy ConnectNode begin.");

  closeConnectNode();

  if (_sslHandle) {
    delete _sslHandle;
    _sslHandle = NULL;
  }

  evbuffer_free(_cmdEvBuffer);
  evbuffer_free(_readEvBuffer);
  evbuffer_free(_binaryEvBuffer);
  evbuffer_free(_wwvEvBuffer);

  if (_nlsEncoder) {
    _nlsEncoder->destroyNlsEncoder();
    delete _nlsEncoder;
    _nlsEncoder = NULL;
  }

#if defined(_MSC_VER)
  CloseHandle(_mtxNode);
  CloseHandle(_mtxCloseNode);
#else
  pthread_mutex_destroy(&_mtxNode);
  pthread_mutex_destroy(&_mtxCloseNode);
#endif
  LOG_DEBUG("Destroy ConnectNode done.");
}

ConnectStatus ConnectNode::getConnectNodeStatus() {
  ConnectStatus status;

#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  status = _workStatus;

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif
  return status;
}

std::string ConnectNode::getConnectNodeStatusString() {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  std::string ret_str("Unknown");
  switch (_workStatus) {
    case NodeInitial:
      ret_str.assign("NodeInitial");
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
    case NodeInvalid:
      ret_str.assign("NodeInvalid");
      break;
  }
#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif
  return ret_str;
}

void ConnectNode::setConnectNodeStatus(ConnectStatus status) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  _workStatus = status;

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif
}

ExitStatus ConnectNode::getExitStatus() {
  ExitStatus ret = ExitInvalid;

#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  ret = _exitStatus;

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif

  return ret;
}

std::string ConnectNode::getExitStatusString() {
  std::string ret_str = "Unknown";
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  switch (_exitStatus) {
    case ExitInvalid:
      ret_str.assign("ExitInvalid");
      break;
    case ExitStopping:
      ret_str.assign("ExitStopping");
      break;
    case ExitStopped:
      ret_str.assign("ExitStopped");
      break;
    case ExitCancel:
      ret_str.assign("ExitCancel");
      break;
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif

  return ret_str;
}

void ConnectNode::setExitStatus(ExitStatus status) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  if (_exitStatus != ExitCancel) {
    _exitStatus = status;
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif
}

bool ConnectNode::updateDestroyStatus() {
  bool ret = true;
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  if (!_isDestroy) {
    _isDestroy = true;
    ret = false;
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif

  return ret;
}

void ConnectNode::setWakeStatus(bool status) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  _isWakeStop = status;

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif
}

bool ConnectNode::getWakeStatus() {
  bool ret = false;
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  ret = _isWakeStop;

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif

  return ret;
}

bool ConnectNode::checkConnectCount() {
  bool result = false;

#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  if (_retryConnectCount < RETRY_CONNECT_COUNT) {
    _retryConnectCount ++;
    result = true;
  } else {
    _retryConnectCount = 0;
    // return false : restart connect failed
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif

  return result;
}

bool ConnectNode::parseUrlInformation() {
  const char* address = _request->getRequestParam()->_url.c_str();
  const char* token = _request->getRequestParam()->_token.c_str();
  size_t tokenSize = _request->getRequestParam()->_token.size();

  LOG_INFO("Node:%p Address:%s.", this, address);

  memset(&_url, 0x0, sizeof(struct urlAddress));

  if (sscanf(address, "%[^:/]://%[^:/]:%d/%s", _url._type, _url._host, &_url._port, _url._path) == 4) {
    if (strcmp(_url._type, "wss") == 0 || strcmp(_url._type, "https") == 0) {
      _url._isSsl = true;
    }
  } else if (sscanf(address, "%[^:/]://%[^:/]/%s", _url._type, _url._host, _url._path) == 3) {
    if (strcmp(_url._type, "wss") == 0 || strcmp(_url._type, "https") == 0) {
      _url._port = 443;
      _url._isSsl = true;
    } else {
      _url._port = 80;
    }
  } else if (sscanf(address, "%[^:/]://%[^:/]:%d", _url._type, _url._host, &_url._port) == 3) {
    _url._path[0] = '\0';
  } else if (sscanf(address, "%[^:/]://%[^:/]", _url._type, _url._host) == 2) {
    if (strcmp(_url._type, "wss") == 0 || strcmp(_url._type, "https") == 0) {
      _url._port = 443;
      _url._isSsl = true;
    } else {
      _url._port = 80;
    }
    _url._path[0] = '\0';
  } else {
    LOG_ERROR("Node:%p Could not parse WebSocket url: %s", this, address);

    return false;
  }

  memcpy(_url._token, token, tokenSize);

  LOG_INFO("Node:%p Type:%s, Host:%s, Port:%d, Path:%s.",
      this, _url._type, _url._host, _url._port, _url._path);

  return true;
}

void ConnectNode::disconnectProcess() {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxCloseNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxCloseNode);
#endif

  if (_socketFd != INVALID_SOCKET) {
    LOG_DEBUG("Node:%p disconnectProcess Begin.", this);

    if (_url._isSsl) {
      _sslHandle->sslClose();
    }

    evutil_closesocket(_socketFd);
    _socketFd = INVALID_SOCKET;

    event_del(&_readEvent);
    event_del(&_writeEvent);
    event_del(&_connectEvent);

    LOG_INFO("Node:%p disconnectProcess done.", this);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxCloseNode);
#else
  pthread_mutex_unlock(&_mtxCloseNode);
#endif
}

void ConnectNode::closeConnectNode() {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxCloseNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxCloseNode);
#endif

  if (_socketFd != INVALID_SOCKET) {
    LOG_DEBUG("Node:%p closeConnectNode Begin.", this);

    setConnectNodeStatus(NodeInvalid);
    setExitStatus(ExitStopped);

    if (_url._isSsl) {
      _sslHandle->sslClose();
    }

    evutil_closesocket(_socketFd);
    _socketFd = INVALID_SOCKET;

    event_del(&_readEvent);
    event_del(&_writeEvent);
    event_del(&_connectEvent);

    LOG_INFO("Node:%p closeConnectNode done.", this);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxCloseNode);
#else
  pthread_mutex_unlock(&_mtxCloseNode);
#endif
}

int ConnectNode::socketWrite(const uint8_t * buffer, size_t len) {
#if defined(__ANDROID__) || defined(__linux__)
  int wLen = send(_socketFd, (const char *)buffer, len, MSG_NOSIGNAL);
#else
  int wLen = send(_socketFd, (const char*)buffer, len, 0);
#endif

  if (wLen < 0) {
    int errorCode = utility::getLastErrorCode();
    if (NLS_ERR_RW_RETRIABLE(errorCode)) {
      //LOG_DEBUG("Node:%p socketWrite continue.", this);

      return 0;
    } else {
      return -1;
    }
  } else {
    return wLen;
  }
}

int ConnectNode::socketRead(uint8_t * buffer, size_t len) {
  int rLen = recv(_socketFd, (char*)buffer, len, 0);

  if (rLen <= 0) { //rLen == 0, close socket, need do
    int errorCode = utility::getLastErrorCode();
    if (NLS_ERR_RW_RETRIABLE(errorCode)) {
      //LOG_DEBUG("Node:%p socketRead continue.", this);

      return 0;
    } else {
      return -1;
    }
  } else {
    return rLen;
  }
}

int ConnectNode::gatewayRequest() {
  event_add(&_readEvent, NULL);

  char tmp[NODE_FRAME_SIZE] = {0};
  int tmpLen = _webSocket.requestPackage(
      &_url, tmp, _request->getRequestParam()->GetHttpHeader());
  if (tmpLen < 0) {
    LOG_DEBUG("Node:%p WebSocket request string failed.\n", this);
    return -1;
  };

  evbuffer_add(_cmdEvBuffer, (void *)tmp, tmpLen);

  return 0;
}

int ConnectNode::gatewayResponse() {
  int ret;
  int read_len;
  uint8_t *frame = (uint8_t *)calloc(READ_BUFFER_SIZE, sizeof(char));
  if (frame == NULL) {
    LOG_ERROR("%s %d calloc failed", __func__, __LINE__);
    return 0;
  }

  read_len = nlsReceive(frame, READ_BUFFER_SIZE);
  if (read_len < 0) {
    free(frame);
    return -1;
  } else if (read_len == 0) {
    free(frame);
    return 0;
  }

  int frameSize = evbuffer_get_length(_readEvBuffer);
  if (frameSize > READ_BUFFER_SIZE) {
    frame = (uint8_t *)realloc(frame, frameSize + 1);
    if (frame == NULL) {
      LOG_ERROR("%s %d realloc failed", __func__, __LINE__);
      free(frame);
      return 0;
    }
  }

  evbuffer_copyout(_readEvBuffer, frame, frameSize);  //evbuffer_peek

  ret = _webSocket.responsePackage((const char*)frame, frameSize);
  if (ret == 0) {
    evbuffer_drain(_readEvBuffer, frameSize);
  } else if (ret > 0) {
    LOG_DEBUG("Node:%p GateWay Middle response:%d\n %s",
        this, frameSize, frame);
  } else {
    _nodeErrMsg = _webSocket.getFailedMsg();
    LOG_DEBUG("Node:%p webSocket.responsePackage :%s\n",
        this, _nodeErrMsg.c_str());
  }

  if (frame) free(frame);
  frame = NULL;

  return ret;
}

int ConnectNode::addAudioDataBuffer(const uint8_t * frame, size_t frameSize) {
  int ret = 0;
  uint8_t *tmp = NULL;
  size_t tmpSize = 0;
  size_t length = 0;
  struct evbuffer* buff = NULL;

  if (_nlsEncoder && _encoder_type != ENCODER_NONE) {
//    LOG_DEBUG("should encording...");
    int nSize = 0;
    uint8_t *outputBuffer = new uint8_t[frameSize];
    if (outputBuffer == NULL) {
      LOG_ERROR("Node:%p new outputBuffer failed", this);
      return -1;
    } else {
      memset(outputBuffer, 0, frameSize);
      nSize = _nlsEncoder->nlsEncoding(
          frame, (int)frameSize,
          outputBuffer, (int)frameSize);
      if (nSize < 0) {
        LOG_ERROR("Node:%p Opus encoder failed %d.", this, nSize);
        delete [] outputBuffer;
        return -1;
      }
      _webSocket.binaryFrame(outputBuffer, nSize, &tmp, &tmpSize);
      delete [] outputBuffer;
    }
  } else {
    // pack frame data
    _webSocket.binaryFrame(frame, frameSize, &tmp, &tmpSize);
  }

  if (_request->getRequestParam()->_enableWakeWord == true &&
      !getWakeStatus()) {
    //LOG_DEBUG("Node:%p It's _wwvEvBuffer.", this);
    buff = _wwvEvBuffer;
  } else {
    //LOG_DEBUG("Node:%p It's _binaryEvBuffer.", this);
    buff = _binaryEvBuffer;
  }

  evbuffer_lock(buff);
  length = evbuffer_get_length(buff);
  if (length >= _limitSize) {
    LOG_WARN("too many audio data in evbuffer");
    evbuffer_unlock(buff);
    return -1;
  }

  evbuffer_add(buff, (void *)tmp, tmpSize);

  if (tmp) free(tmp);
  tmp = NULL;

  evbuffer_unlock(buff);
  //LOG_DEBUG("Node:%p AudioBuffer add buff:%zu %zu", 
  //    this, length, length + tmpSize);

  if (length == 0 && getConnectNodeStatus() == NodeStarted) {
    #if defined(_MSC_VER)
    WaitForSingleObject(_mtxNode, INFINITE);
    #else
    pthread_mutex_lock(&_mtxNode);
    #endif
    if (!_isStop) {
      ret = nlsSendFrame(buff);
    }
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNode);
    #else
    pthread_mutex_unlock(&_mtxNode);
    #endif
  }

  if (length == 0 && getConnectNodeStatus() == NodeWakeWording) {
    #if defined(_MSC_VER)
    WaitForSingleObject(_mtxNode, INFINITE);
    #else
    pthread_mutex_lock(&_mtxNode);
    #endif
    if (!_isStop) {
      ret = nlsSendFrame(buff);
    }
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNode);
    #else
    pthread_mutex_unlock(&_mtxNode);
    #endif
  }

  if (ret == 0) {
    ret = sendControlDirective();
  }

  if (ret == -1) {
    handlerTaskFailedEvent(getErrorMsg());
    disconnectProcess();
  }

  return ret;
}

int ConnectNode::sendControlDirective() {
  int ret = 0;

#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  if (_workStatus == NodeStarted && _exitStatus == ExitInvalid) {
    size_t length = evbuffer_get_length(getCmdEvBuffer());
    if (length != 0) {
      LOG_DEBUG("Node:%p Cmd buffer is't empty.", this);
      ret = nlsSendFrame(getCmdEvBuffer());
    }
  } else {
    if (_exitStatus == ExitStopping && _isStop == false) {
      LOG_DEBUG("Node:%p Audio is send done. And invoke stop command.", this);
      addCmdDataBuffer(CmdStop);
      ret = nlsSendFrame(getCmdEvBuffer());
      _isStop = true;
    }
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif

  return ret;
}

void ConnectNode::addCmdDataBuffer(CmdType type, const char* message) {
  char *cmd = NULL;

  LOG_DEBUG("Node:%p Get Cmd Type:%d", this, type);

  switch(type) {
    case CmdStart:
      cmd = (char *)_request->getRequestParam()->getStartCommand();
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
      LOG_DEBUG("Node:%p Cancel Cmd.", this);
      return;
    default:
      LOG_DEBUG("Node:%p Unkown Cmd.", this);
      return ;
  }

  if (cmd) {
    LOG_INFO("Node:%p Get Cmd:%s", this, cmd);

    uint8_t *frame = NULL;
    size_t frameSize = 0;
    _webSocket.textFrame((uint8_t *) cmd, strlen(cmd), &frame, &frameSize);

    LOG_DEBUG("Node:%p WebSocket Size:%zu", this, frameSize);

    evbuffer_add(_cmdEvBuffer, (void *) frame, frameSize);

    if (frame) free(frame);
    frame = NULL;
  }
}

int ConnectNode::cmdNotify(CmdType type, const char* message) {
  int ret = 0;

  LOG_DEBUG("Node:%p CmdNotify:%d.", this, type);

  if (type == CmdStop) {
    setExitStatus(ExitStopping);
    if (getConnectNodeStatus() == NodeStarted) {
      size_t length = evbuffer_get_length(_binaryEvBuffer);
      if (length == 0) {
        ret = sendControlDirective();
      } else {
        LOG_DEBUG("Node:%p CmdNotify:%d. Continue Send Audio : %zu .",
            this, type, length);
      }
    }
  } else if (type == CmdStControl) {
    addCmdDataBuffer(CmdStControl, message);
    if (getConnectNodeStatus() == NodeStarted) {
      size_t length = evbuffer_get_length(_binaryEvBuffer);
      if (length == 0) {
        ret = nlsSendFrame(_cmdEvBuffer);
      }
    }
  } else if (type == CmdWarkWord) {
    setWakeStatus(true);
    size_t length = evbuffer_get_length(_wwvEvBuffer);
    if (length == 0) {
      addCmdDataBuffer(CmdWarkWord);
      ret = nlsSendFrame(_cmdEvBuffer);
    }
  } else if (type == CmdCancel) {
    setExitStatus(ExitCancel);
  } else {
    LOG_ERROR("Node:%p CmdNotify Unknown.", this);
  }

  if (ret == -1) {
    handlerTaskFailedEvent(getErrorMsg());
    disconnectProcess();
  }

  return ret;
}

int ConnectNode::nlsSend(const uint8_t * frame, size_t length) {
  int sLen;
  if ((frame == NULL) || (length == 0)) {
    return 0;
  }

  if (_url._isSsl) {
    //LOG_DEBUG("SSL Send data.");
    sLen = _sslHandle->sslWrite(frame, length);
  } else {
    //LOG_DEBUG("Socket Send data.");
    sLen = socketWrite(frame, length);
  }

  //LOG_DEBUG("Send data: %d.", sLen);

  if (sLen < 0) {
    if (_url._isSsl) {
      _nodeErrMsg = _sslHandle->getFailedMsg();
    } else {
      _nodeErrMsg =
          evutil_socket_error_to_string(evutil_socket_geterror(_socketFd));
    }

    LOG_ERROR("Node:%p send failed:%s.", this,  _nodeErrMsg.c_str());
  }

  return sLen;
}

int ConnectNode::nlsSendFrame(struct evbuffer * eventBuffer) {
  int sLen = 0;
  uint8_t buffer[NODE_FRAME_SIZE] = {0};
  size_t bufferSize = 0;

  evbuffer_lock(eventBuffer);
  size_t length = evbuffer_get_length(eventBuffer);
  if (length == 0) {
    LOG_DEBUG("eventBuffer is NULL.");
    evbuffer_unlock(eventBuffer);
    return 0;
  }

  if (length > NODE_FRAME_SIZE) {
    bufferSize = NODE_FRAME_SIZE;
  } else {
    bufferSize =length;
  }
  evbuffer_copyout(eventBuffer, buffer, bufferSize); //evbuffer_peek

  if (bufferSize > 0) {
    sLen = nlsSend(buffer, bufferSize);
  }

  if (sLen < 0) {
    LOG_ERROR("Node:%p nlsSend failed: %d.", this, sLen);
    evbuffer_unlock(eventBuffer);
    return -1;
  } else {
    evbuffer_drain(eventBuffer, sLen);
    length = evbuffer_get_length(eventBuffer);
    if (length > 0) {
      struct timeval tv;
      tv.tv_sec = 3;  //_request->getRequestParam()->_timeout;
      tv.tv_usec = 0;
      event_add(&_writeEvent, &tv);
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
      _nodeErrMsg = evutil_socket_error_to_string(
          evutil_socket_geterror(_socketFd));
    }
    LOG_ERROR("Node:%p Recv Failed: %s.", this, _nodeErrMsg.c_str());
    return -1;
  }

  evbuffer_add(_readEvBuffer, (void *)buffer, rLen);

  return rLen;
}

int ConnectNode::webSocketResponse() {
  int ret = 0;
  bool rLoop = false;
  uint8_t *frame = (uint8_t *)calloc(READ_BUFFER_SIZE, sizeof(char));
  if (frame == NULL) {
    LOG_ERROR("%s %d calloc failed", __func__, __LINE__);
    return 0;
  }

  do {
    ret = nlsReceive(frame, READ_BUFFER_SIZE);
    if (ret < 0) {
      free(frame);
      return -1;
    } else if (ret > 0) {
      rLoop = true;
    }
    //LOG_DEBUG("Node:%p WebSocket Recv:%d", this, ret);

    bool eLoop = false;
    do {
      size_t frameSize = evbuffer_get_length(_readEvBuffer);
      if (frameSize == 0) {
        //LOG_DEBUG("Node:%p evbuffer_get_length:%d", this, frameSize);
        free(frame);
        return 0;
      } else {
        // no new data added in evbuffer
        if (ret == 0) {
          usleep(2 * 1000);
          break;
        }
      }

      if (frameSize > READ_BUFFER_SIZE) {
        frame = (uint8_t *)realloc(frame, frameSize + 1);
        if (frame == NULL) {
          LOG_ERROR("%s %d realloc failed", __func__, __LINE__);
          free(frame);
          return 0;
        }
      }
      evbuffer_copyout(_readEvBuffer, frame, frameSize);

      //LOG_DEBUG("Node:%p WebSocket Middle response:%d", this, frameSize);

      WebSocketFrame wsFrame;
      memset(&wsFrame, 0x0, sizeof(struct WebSocketFrame));
      if (_webSocket.receiveFullWebSocketFrame(
            frame, frameSize, &_wsType, &wsFrame) == 0) {
        LOG_DEBUG("Node:%p Parse Ws frame:%zu | %zu",
            this, wsFrame.length, frameSize);

        parseFrame(&wsFrame);

        evbuffer_drain(_readEvBuffer, wsFrame.length + _wsType.headerSize);

        if (evbuffer_get_length(_readEvBuffer) > 0) {
          eLoop = true;
          LOG_DEBUG("Node:%p Parse continue.", this);
        }
      } else {
        eLoop = false;
      }
    } while (eLoop);

    if (getConnectNodeStatus() == NodeInvalid) {
      rLoop = false;
      break;
    }
  } while (rLoop);

  if (frame) free(frame);
  frame = NULL;

  return 0;
}

#if defined(__ANDROID__) || defined(__linux__)
int ConnectNode::codeConvert(char *from_charset, char *to_charset,
                             char *inbuf, size_t inlen,
                             char *outbuf, size_t outlen) {
#if defined(__ANDRIOD__)
  outbuf = inbuf;
#else
  iconv_t cd;
  char **pin = &inbuf;
  char **pout = &outbuf;

  cd = iconv_open(to_charset, from_charset);
  if (cd == 0) {
    return -1;
  }

  memset(outbuf, 0, outlen);
  if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t)-1) {
    return -1;
  }

  iconv_close(cd);
#endif
  return 0;
}
#endif

std::string ConnectNode::utf8ToGbk(const std::string &strUTF8) {
#if defined(__ANDROID__) || defined(__linux__)
  const char *msg = strUTF8.c_str();
  size_t inputLen = strUTF8.length();
  size_t outputLen = inputLen * 20;

  char *outbuf = new char[outputLen + 1];
  memset(outbuf, 0x0, outputLen + 1);

  char *inbuf = new char[inputLen + 1];
  memset(inbuf, 0x0, inputLen + 1);
  strncpy(inbuf, msg, inputLen);

  int res = codeConvert(
      (char *)"UTF-8", (char *)"GBK", inbuf, inputLen, outbuf, outputLen);
  if (res == -1) {
    LOG_ERROR("ENCODE: convert to utf8 error :%d .",
        utility::getLastErrorCode());
    return NULL;
  }

  std::string strTemp(outbuf);

  delete [] outbuf;
  outbuf = NULL;
  delete [] inbuf;
  inbuf = NULL;

  return strTemp;

#elif defined (_MSC_VER)

  int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
  unsigned short* wszGBK = new unsigned short[len + 1];
  memset(wszGBK, 0, len * 2 + 2);

  MultiByteToWideChar(CP_UTF8, 0, (char*)strUTF8.c_str(), -1, (wchar_t*)wszGBK, len);

  len = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszGBK, -1, NULL, 0, NULL, NULL);

  char* szGBK = new char[len + 1];
  memset(szGBK, 0, len + 1);
  WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszGBK, -1, szGBK, len, NULL, NULL);

  std::string strTemp(szGBK);
  delete[] szGBK;
  delete[] wszGBK;

  return strTemp;

#else

  return strUTF8;

#endif
}

NlsEvent* ConnectNode::convertResult(WebSocketFrame * wsFrame) {
  NlsEvent* wsEvent = NULL;
  if (wsFrame->type == WebSocketHeaderType::BINARY_FRAME) {
    if (wsFrame->length > 0) {
      std::vector<unsigned char> data = std::vector<unsigned char>(
          wsFrame->data, wsFrame->data + wsFrame->length);

      wsEvent = new NlsEvent(data, 0,
                             NlsEvent::Binary,
                             _request->getRequestParam()->_task_id);
    }
  } else if (wsFrame->type == WebSocketHeaderType::TEXT_FRAME) {
    // 打印这个string，可能会因为太长而崩溃
    std::string result((char *)wsFrame->data, wsFrame->length);
    if (wsFrame->length > 1024) {
      std::string part_result((char *)wsFrame->data, 1024);
      LOG_INFO("Node:%p %d too long, Part Response(1024): %s",
          this, wsFrame->length, part_result.c_str());
    } else {
      LOG_INFO("Node:%p Response(%d): %s",
          this, wsFrame->length, result.c_str());
    }

    if ("GBK" == _request->getRequestParam()->_outputFormat) {
      result = utf8ToGbk(result);
    }
    if (result.empty()) {
      handlerEvent(TASKFAILED_UTF8_JSON_STRING,
                   TASK_FAILED_CODE,
                   NlsEvent::TaskFailed);
      return NULL;
    }

    wsEvent = new NlsEvent(result);
    if (wsEvent == NULL) {
      handlerEvent(TASKFAILED_PARSE_JSON_STRING,
                   TASK_FAILED_CODE,
                   NlsEvent::TaskFailed);
    } else {
      int ret = wsEvent->parseJsonMsg();
      if (ret < 0) {
        delete wsEvent;
        wsEvent = NULL;
        handlerEvent(TASKFAILED_PARSE_JSON_STRING,
                     TASK_FAILED_CODE,
                     NlsEvent::TaskFailed);
      }
    }
  } else {
    handlerEvent(TASKFAILED_WS_JSON_STRING,
                 TASK_FAILED_CODE,
                 NlsEvent::TaskFailed);
  }

  return wsEvent;
}

int ConnectNode::parseFrame(WebSocketFrame * wsFrame) {
  NlsEvent* frameEvent = NULL;

  if (wsFrame->type == WebSocketHeaderType::CLOSE) {
    if (wsFrame->closeCode == -1) {
      std::string msg((char *)wsFrame->data);
      char tmp_msg[2048] = {0};
      snprintf(tmp_msg, 2048 - 1, "{\"TaskFailed\":\"%s\"}", msg.c_str());
      std::string closeMsg = tmp_msg;

      LOG_INFO("Node:%p Close msg:%s.", this, closeMsg.c_str());

      frameEvent = new NlsEvent(
          closeMsg.c_str(), wsFrame->closeCode,
          NlsEvent::TaskFailed, _request->getRequestParam()->_task_id);
    }
  } else {
    frameEvent = convertResult(wsFrame);
  }

  if (frameEvent == NULL) {
    LOG_ERROR("Node:%p Result conversion failed.", this);
    handlerEvent(CLOSE_JSON_STRING, CLOSE_CODE, NlsEvent::Close);
    closeConnectNode();
    return -1;
  }

  LOG_DEBUG("Node:%p parseFrame MsgType:%d, %d.",
      this, frameEvent->getMsgType(), getExitStatus());

  //invoke cancel()
  if (getExitStatus() == ExitCancel || getExitStatus() == ExitStopped) {
    LOG_DEBUG("Node:%p is stopped, %d.", this, getExitStatus());
    return -1;
  }

  LOG_DEBUG("Node:%p Begin HandlerFrame:%d.", this, getExitStatus());
  _handler->handlerFrame(*frameEvent);
  LOG_DEBUG("Node:%p End HandlerFrame.", this);

  bool closeFlag = false;
  switch(frameEvent->getMsgType()) {
    case NlsEvent::RecognitionStarted:
    case NlsEvent::TranscriptionStarted:
      if (_request->getRequestParam()->_requestType != SpeechWakeWordDialog) {
        setConnectNodeStatus(NodeStarted);
        LOG_DEBUG("Node:%p Node Status: NodeStarted.", this);
      } else {
        setConnectNodeStatus(NodeWakeWording);
      }
      break;
    case NlsEvent::Close:
      //setConnectNodeStatus(NodeInitial);  // add by fsc 2021.11.1
    case NlsEvent::RecognitionCompleted:
      if (_request->getRequestParam()->_mode == TypeDialog) {
        closeFlag = false;
        break;
      }
    case NlsEvent::TaskFailed:
    case NlsEvent::TranscriptionCompleted:
    case NlsEvent::SynthesisCompleted:
    case NlsEvent::DialogResultGenerated:
      closeFlag = true;
      break;
    case NlsEvent::WakeWordVerificationCompleted:
      setConnectNodeStatus(NodeStarted);
      break;
    default:
      closeFlag = false;
      break;
  }

  if (frameEvent) delete frameEvent;
  frameEvent = NULL;

  if (closeFlag) {
    handlerEvent(CLOSE_JSON_STRING, CLOSE_CODE, NlsEvent::Close);
    closeConnectNode();
    return -1;
  }

  return 0;
}

void ConnectNode::handlerEvent(const char* error,
                               int errorCode,
                               NlsEvent::EventType eventType) {
  //invoke cancel()
  LOG_DEBUG("Node:%p 's Exit Status:%d.", this, getExitStatus());
  if (getExitStatus() == ExitCancel || getExitStatus() == ExitStopped) {
    LOG_WARN("Node:%p Invoke Cancel command, Callback will n't be invoked.", this);
    return;
  }

  NlsEvent* useEvent = NULL;
  useEvent = new NlsEvent(
      error, errorCode, eventType, _request->getRequestParam()->_task_id);
  if (useEvent == NULL) {
    return;
  }

  LOG_INFO("Node:%p Begin HandlerFrame.", this);
  _handler->handlerFrame(*useEvent);
  LOG_INFO("Node:%p End HandlerFrame.", this);

  delete useEvent;
  useEvent = NULL;
}

void ConnectNode::handlerTaskFailedEvent(std::string failedInfo) {
  char tmp_msg[1024] = {0};

  if (failedInfo.empty()) {
    strcpy(tmp_msg, "{\"TaskFailed\":\"Unknown failed.\"}");
  } else {
    snprintf(tmp_msg, 1024 - 1, "{\"TaskFailed\":\"%s\"}", failedInfo.c_str());
  }

  handlerEvent(tmp_msg, TASK_FAILED_CODE, NlsEvent::TaskFailed);
  handlerEvent(CLOSE_JSON_STRING, CLOSE_CODE, NlsEvent::Close);

  return ;
}

int ConnectNode::dnsProcess() {
  struct evutil_addrinfo hints;
  struct evdns_getaddrinfo_request *dnsRequest;

  //invoke cancel()
  if (getExitStatus() == ExitCancel) {
    return -1;
  }

  if (!checkConnectCount()) {
    LOG_ERROR("Node:%p restart connect failed.", this);
    handlerTaskFailedEvent(TASKFAILED_CONNECT_JSON_STRING);
    return -1;
  }

  setConnectNodeStatus(NodeConnecting);

  parseUrlInformation();

  if (_url._isSsl) {
    LOG_DEBUG("Node:%p _url._isSsl is True.", this);
  } else {
    LOG_DEBUG("Node:%p _url._isSsl is false.", this);
  }

  memset(&hints,  0,  sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = EVUTIL_AI_CANONNAME;

  /* Unless we specify a socktype, we'llget at least two entries for
   * each address: one for TCP and onefor UDP. That's not what we
   * want. */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  LOG_INFO("Node:%p Dns URL:%s.", this, _request->getRequestParam()->_url.c_str());

  dnsRequest = evdns_getaddrinfo(_eventThread->_dnsBase,
                                 _url._host,
                                 NULL,
                                 &hints,
                                 WorkThread::dnsEventCallback,
                                 this);
  if (dnsRequest == NULL) {
    //LOG_DEBUG("Node:%p dnsRequest returned immediately.", this);

    /*
     * No need to free user_data ordecrement n_pending_requests; that
     * happened in the callback.
     */
  }

  return 0;
}

int ConnectNode::connectProcess(const char *ip, int aiFamily) {
  //invoke cancel()
  if (getExitStatus() == ExitCancel) {
    return -1;
  }

  evutil_socket_t sockFd = socket(aiFamily, SOCK_STREAM, 0);
  if (sockFd < 0) {
    LOG_ERROR("Node:%p socket failed. aiFamily:%d, sockFd:%d. err mesg:%s",
        this, aiFamily, sockFd,
        evutil_socket_error_to_string(evutil_socket_geterror(sockFd)));
    return -1;
  } else {
    LOG_DEBUG("Node:%p sockFd:%d", this, sockFd);
  }

  struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = 0;
  if (setsockopt(sockFd, SOL_SOCKET, SO_LINGER,
      (char *)&so_linger, sizeof(struct linger)) < 0) {
    LOG_ERROR("Node:%p Set SO_LINGER failed.", this);
    return -1;
  }

  if (evutil_make_socket_nonblocking(sockFd) < 0) {
    LOG_ERROR("Node:%p evutil_make_socket_nonblocking failed.", this);
    return -1;
  }

  LOG_INFO("Node:%p new Socket ip:%s port:%d  Fd:%d.",
      this, ip, _url._port, sockFd);

  event_assign(&_connectEvent, _eventThread->_workBase, sockFd,
               EV_READ | EV_WRITE | EV_TIMEOUT,
               WorkThread::connectEventCallback,
               this);

  event_assign(&_readEvent, _eventThread->_workBase, sockFd,
               EV_READ | EV_TIMEOUT | EV_PERSIST,
               WorkThread::readEventCallBack,
               this);

  event_assign(&_writeEvent, _eventThread->_workBase, sockFd,
               EV_WRITE | EV_TIMEOUT,
               WorkThread::writeEventCallBack,
               this);

  _aiFamily = aiFamily;
  if (aiFamily == AF_INET) {
    memset(&_addrV4, 0, sizeof(_addrV4));
    _addrV4.sin_family = AF_INET;
    _addrV4.sin_port = htons(_url._port);

    if (inet_pton(AF_INET, ip, &_addrV4.sin_addr) <= 0) {
      LOG_ERROR("Node:%p IpV4 inet_pton failed.", this);
      evutil_closesocket(sockFd);
      return -1;
    }
  } else if (aiFamily == AF_INET6) {
    memset(&_addrV6, 0, sizeof(_addrV6));
    _addrV6.sin6_family = AF_INET6;
    _addrV6.sin6_port = htons(_url._port);

    if (inet_pton(AF_INET6, ip, &_addrV6.sin6_addr) <= 0) {
      LOG_ERROR("Node:%p IpV6 inet_pton failed.", this);
      evutil_closesocket(sockFd);
      return -1;
    }
  }

  _socketFd = sockFd;

  return socketConnect();
}

int ConnectNode::socketConnect() {
  int retCode = 0;

  if (_aiFamily == AF_INET) {
    retCode = connect(_socketFd, (const sockaddr *)&_addrV4, sizeof(_addrV4));
  } else {
    retCode = connect(_socketFd, (const sockaddr *)&_addrV6, sizeof(_addrV6));
  }

  if (retCode == -1) {
    _connectErrCode = utility::getLastErrorCode();
    if (NLS_ERR_CONNECT_RETRIABLE(_connectErrCode)) {
      /* _connectErrCode == 115(EINPROGRESS)
       *  means connection is in progress,
       *  normally the socket connecting timeout is 75s.
       *  after the socket fd is ready to read.
       */
#if 0
    // 无法正常工作
    #if 1
      struct pollfd pollfds;
      pollfds.fd = _socketFd;
      pollfds.events = POLLIN | POLLPRI;  // POLLIN|POLLOUT，表示监控是否可读或可写
      int timeout = 2000;
      int retCode = poll(&pollfds, 1, timeout);
      if (retCode > 0) {
        LOG_INFO("Node:%p connected directly0.", this);
        setConnectNodeStatus(NodeConnected);
        return 0;
      } else {
        // "=0" means timeout cause retrun, "<0" means error.
        event_add(&_connectEvent, &_connectTv);
        LOG_DEBUG("Node:%p connect would block:%d.", this, _connectErrCode);
        return 1;
      }
    #else
      // will crash in FD_SET
      fd_set fdr, fdw;
      struct timeval timeout;
      FD_ZERO(&fdr);
      FD_ZERO(&fdw);
      FD_SET(_socketFd, &fdr);
      FD_SET(_socketFd, &fdw);
      timeout.tv_sec = 2;
      timeout.tv_usec = 0;
      int retCode = select(_socketFd + 1, &fdr, &fdw, NULL, &timeout);
      // ">0" means sockfd ready to read
      if (retCode > 0) {
        LOG_INFO("Node:%p connected directly0.", this);
        setConnectNodeStatus(NodeConnected);
        return 0;
      } else {
        // "=0" means timeout cause retrun, "<0" means error.
        event_add(&_connectEvent, &_connectTv);
        LOG_DEBUG("Node:%p connect would block:%d.", this, _connectErrCode);
        return 1;
      }
    #endif
#else
      // origin code
      event_add(&_connectEvent, &_connectTv);
      LOG_DEBUG("Node:%p connect would block:%d.", this, _connectErrCode);
      return 1;
#endif
    } else {
      LOG_ERROR("Node:%p Connect failed:%s. retry...",
          this, evutil_socket_error_to_string(evutil_socket_geterror(_socketFd)));
      return -1;
    }
  } else {
    LOG_INFO("Node:%p connected directly1.", this);
    setConnectNodeStatus(NodeConnected);
  }
  return 0;
}

int ConnectNode::sslProcess() {
  int ret = 0;

  //invoke cancel()
  if (getExitStatus() == ExitCancel) {
    return -1;
  }

  //LOG_DEBUG("begin ssl process.");
  if (_url._isSsl) {
    ret = _sslHandle->sslHandshake(_socketFd, _url._host);
    if (ret == SSL_ERROR_WANT_READ || ret == SSL_ERROR_WANT_WRITE) {
      //LOG_DEBUG("wait ssl process.");
      event_add(&_connectEvent, &_connectTv);
      return 1;
    } else if (ret < 0) {
      _nodeErrMsg = _sslHandle->getFailedMsg();
      LOG_ERROR("Node:%p sslHandshake failed, %s.", this, _nodeErrMsg.c_str());
      return -1;
    } else {
      //LOG_INFO("Node:%p sslHandshake done.", this);
      setConnectNodeStatus(NodeHandshaking);
      return 0;
    }
  } else {
    setConnectNodeStatus(NodeHandshaking);
    LOG_INFO("Node:%p It 's not ssl process.", this);
  }

  return 0;
}

void ConnectNode::resetBufferLimit() {
  if (_request->getRequestParam()->_sampleRate == SAMPLE_RATE_16K) {
    _limitSize = BUFFER_16K_MAX_LIMIT;
  } else {
    _limitSize = BUFFER_8K_MAX_LIMIT;
  }

  return;
}

void ConnectNode::initNlsEncoder() {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNode, INFINITE);
#else
  pthread_mutex_lock(&_mtxNode);
#endif

  if (_nlsEncoder == NULL) {
    if (_request->getRequestParam()->_format == "opu") {
      _encoder_type = ENCODER_OPU;
    } else if (_request->getRequestParam()->_format == "opus") {
      _encoder_type = ENCODER_OPUS;
    }

    if (_encoder_type != ENCODER_NONE) {
      int errorCode = 0;
      _nlsEncoder = new NlsEncoder();
      if (_nlsEncoder == NULL) {
        LOG_ERROR("new _nlsEncoder failed");
        #if defined(_MSC_VER)
        ReleaseMutex(_mtxNode);
        #else
        pthread_mutex_unlock(&_mtxNode);
        #endif
        return;
      }
      int ret = _nlsEncoder->createNlsEncoder(
          _encoder_type, 1,
          _request->getRequestParam()->_sampleRate,
          &errorCode);
      if (ret < 0) {
        LOG_ERROR("createNlsEncoder failed, errcode:%d", errorCode);
        delete _nlsEncoder;
        _nlsEncoder = NULL;
      }
    }
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNode);
#else
  pthread_mutex_unlock(&_mtxNode);
#endif
}

}  // namespace AlibabaNls
