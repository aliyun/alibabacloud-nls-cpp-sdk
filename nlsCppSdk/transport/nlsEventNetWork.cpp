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
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "connectNode.h"
#include "event2/dns.h"
#include "event2/thread.h"
#include "iNlsRequest.h"
#include "nlog.h"
#include "nlsClientImpl.h"
#include "nlsEventNetWork.h"
#include "nlsGlobal.h"
#include "nodeManager.h"
#include "utility.h"
#include "workThread.h"
#ifdef ENABLE_REQUEST_RECORDING
#include "text_utils.h"
#endif

namespace AlibabaNls {

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32")

HANDLE NlsEventNetWork::_mtxThread = NULL;
#else
pthread_mutex_t NlsEventNetWork::_mtxThread = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsEventNetWork *NlsEventNetWork::_eventClient = NULL;

NlsEventNetWork::NlsEventNetWork()
    : _workThreadArray(NULL),
      _workThreadsNumber(0),
      _currentCpuNumber(0),
      _addrInFamily(0),
      _directIp(),
      _enableSysGetAddr(false),
      _syncCallTimeoutMs(0),
      _instance(NULL) {}

NlsEventNetWork::~NlsEventNetWork() {}

void NlsEventNetWork::DnsLogCb(int w, const char *m) { LOG_DEBUG(m); }

void NlsEventNetWork::EventLogCb(int w, const char *m) { LOG_DEBUG(m); }

void NlsEventNetWork::initEventNetWork(NlsClientImpl *instance, int count,
                                       char *aiFamily, char *directIp,
                                       bool sysGetAddr,
                                       unsigned int syncCallTimeoutMs) {
  MUTEX_LOCK(_mtxThread);

#if defined(_MSC_VER)
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
  LOG_DEBUG("evthread_use_windows_thread.");
  int ret = evthread_use_windows_threads();
#endif

  WORD wVersionRequested;
  WSADATA wsaData;

  wVersionRequested = MAKEWORD(2, 2);

  (void)WSAStartup(wVersionRequested, &wsaData);
#else
  LOG_DEBUG("evthread_use_pthreads.");
  int ret = evthread_use_pthreads();
#endif
  if (ret != Success) {
    LOG_ERROR("Invoke evthread_use_pthreads failed, ret:%d.", ret);
    MUTEX_UNLOCK(_mtxThread);
    exit(1);
  }

#ifdef ENABLE_NLS_DEBUG
  evthread_enable_lock_debugging();
#endif

  _instance = instance;
  _addrInFamily = AF_INET;
  if (aiFamily != NULL) {
    if (strncmp(aiFamily, "AF_INET", 16) == 0) {
      _addrInFamily = AF_INET;
    } else if (strncmp(aiFamily, "AF_INET6", 16) == 0) {
      _addrInFamily = AF_INET6;
    } else if (strncmp(aiFamily, "AF_UNSPEC", 16) == 0) {
      _addrInFamily = AF_UNSPEC;
    }
    LOG_INFO("Set sockaddr_in type: %s.", aiFamily);
  }

  memset(_directIp, 0, 64);
  if (directIp) {
    strncpy(_directIp, directIp, 64);
  }
  _enableSysGetAddr = sysGetAddr;
  _syncCallTimeoutMs = syncCallTimeoutMs;

#if defined(_MSC_VER)
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  int cpuNumber = (int)sysInfo.dwNumberOfProcessors;
#elif defined(__linux__) || defined(__ANDROID__)
  int cpuNumber = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif

#if defined(_MSC_VER)
  WorkThread::_mtxCpu = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&WorkThread::_mtxCpu, NULL);
#endif

  if (count <= 0) {
    _workThreadsNumber = cpuNumber;
  } else {
    _workThreadsNumber = count;
  }
  LOG_INFO("Work threads number: %d.", _workThreadsNumber);

  _workThreadArray = new WorkThread[_workThreadsNumber];

  evdns_set_log_fn(DnsLogCb);
  event_set_log_callback(EventLogCb);

  MUTEX_UNLOCK(_mtxThread);
  LOG_INFO("Init NlsEventNetWork done.");
  return;
}

void NlsEventNetWork::destroyEventNetWork() {
  LOG_INFO("Destroy NlsEventNetWork(%p) begin ...", _eventClient);
  MUTEX_LOCK(_mtxThread);

  delete[] _workThreadArray;
  _workThreadArray = NULL;

#if defined(_MSC_VER)
  CloseHandle(WorkThread::_mtxCpu);
#else
  pthread_mutex_destroy(&WorkThread::_mtxCpu);
#endif

  _workThreadsNumber = 0;
  _currentCpuNumber = 0;

  MUTEX_UNLOCK(_mtxThread);
  LOG_INFO("Destroy NlsEventNetWork(%p) done.", _eventClient);
  return;
}

/**
 * @brief: 选择工作进程
 * @return: 成功则返回工程进程号, 失败则返回负值.
 */
int NlsEventNetWork::selectThreadNumber() {
  int number = 0;

  if (_workThreadArray != NULL) {
    number = _currentCpuNumber;

    if (++_currentCpuNumber == _workThreadsNumber) {
      _currentCpuNumber = 0;
    }
    LOG_INFO("Select Thread NO:%d, Next NO:%d, Total:%d.", number,
             _currentCpuNumber, _workThreadsNumber);
  } else {
    LOG_ERROR(
        "WorkThread isn't startup. Please invoke startWorkThread() first.");
    number = -1;
  }

  return number;
}

int NlsEventNetWork::start(INlsRequest *request) {
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return -(NodeEmpty);
  }

  /* 长链接模式下, 若为Completed状态, 则等待其进入Closed后, 再重置状态. */
  if (node->isLongConnection()) {
    int try_count = 500;
    while (try_count-- > 0 && node->getConnectNodeStatus() == NodeCompleted) {
#ifdef _MSC_VER
      Sleep(1);
#else
      usleep(1000);
#endif
    }

    if (node->getConnectNodeStatus() == NodeClosed) {
      LOG_DEBUG(
          "Node:%p current is NodeClosed and longConnection mode, reset "
          "status.",
          node);
      node->initAllStatus();
    }
  }

  /*
   * invoke start
   * Node处于刚创建完状态, 且处于非退出状态, 则可进行start操作.
   */
  if (node->getConnectNodeStatus() == NodeCreated &&
      node->getExitStatus() == ExitInvalid) {
    node->setConnectNodeStatus(NodeInvoking);
#ifdef ENABLE_REQUEST_RECORDING
    node->updateNodeProcess("start", NodeInvoking, true, 0);
#endif

    int num = request->getThreadNumber();
    if (num < 0) {
      num = selectThreadNumber();
    }
    if (num < 0) {
      node->setConnectNodeStatus(NodeCreated);
      MUTEX_UNLOCK(_mtxThread);
#ifdef ENABLE_REQUEST_RECORDING
      node->updateNodeProcess("start", NodeCreated, false, 0);
#endif
      return -(SelectThreadFailed);
    } else {
      request->setThreadNumber(num);
    }

    LOG_INFO("Request(%p) node(%p) select NO:%d thread.", request, node, num);

    WorkThread *work_thread = &_workThreadArray[num];
    node->setEventThread(work_thread);
    node->getEventThread()->setInstance(_instance);
    node->setInstance(_instance);
    node->getEventThread()->setAddrInFamily(_addrInFamily);
    if (strnlen(_directIp, 64) > 0) {
      node->getEventThread()->setDirectHost(_directIp);
    }
    node->getEventThread()->setUseSysGetAddrInfo(_enableSysGetAddr);
    node->setSyncCallTimeout(_syncCallTimeoutMs);
    work_thread->updateParameters(node);

    LOG_DEBUG("Request(%p) node(%p) ready to invoke event_add LaunchEvent ...",
              request, node);
    int event_ret = event_add(node->getLaunchEvent(true), NULL);
    if (event_ret != Success) {
      LOG_ERROR("Request(%p) node(%p) invoking event_add failed(%d).", request,
                node, event_ret);
      MUTEX_UNLOCK(_mtxThread);
#ifdef ENABLE_REQUEST_RECORDING
      node->updateNodeProcess("start", NodeCreated, false, 0);
#endif
      return -(InvokeStartFailed);
    } else {
      LOG_DEBUG(
          "Request(%p) node(%p) invoking event_add success, ready to launch "
          "request.",
          request, node);
    }
    event_active(node->getLaunchEvent(), EV_READ, 0);

    node->initNlsEncoder();

    if (node->getSyncCallTimeout() > 0) {
      node->waitInvokeFinish();
      int error_code = node->getErrorCode();
      if (error_code != Success) {
        MUTEX_UNLOCK(_mtxThread);
#ifdef ENABLE_REQUEST_RECORDING
        node->updateNodeProcess("start", NodeCreated, false, 0);
#endif
        return -(error_code);
      }
    }
#ifdef ENABLE_REQUEST_RECORDING
    node->updateNodeProcess("start", NodeCreated, false, 0);
#endif

  } else if (node->getExitStatus() == ExitInvalid &&
             node->getConnectNodeStatus() > NodeCreated &&
             node->getConnectNodeStatus() < NodeFailed) {
    LOG_WARN(
        "Request(%p) node(%p) has invoked start, node status:%s, exit "
        "status:%s. skip ...",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());

    MUTEX_UNLOCK(_mtxThread);
    return Success;
  } else {
    LOG_ERROR(
        "Request(%p) node(%p) invoke start failed, current status is invalid. "
        "node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());

    node->setConnectNodeStatus(NodeCreated);
    MUTEX_UNLOCK(_mtxThread);
    return -(InvokeStartFailed);
  }

  MUTEX_UNLOCK(_mtxThread);
  LOG_DEBUG("Request(%p) node(%p) invoke start success.", request, node);
  return Success;
}

int NlsEventNetWork::sendAudio(INlsRequest *request, const uint8_t *data,
                               size_t dataSize, ENCODER_TYPE type) {
  EVENT_CLIENT_CHECK(_eventClient);
  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request:%p is nullptr, you have destroyed request!",
              request);
    return -(NodeEmpty);
  }

  /* Node也许处于Starting状态还未到Started状态, 可等待一会. */
  int try_count = 500;
  while (try_count-- > 0 && node->getConnectNodeStatus() == NodeStarting) {
#ifdef _MSC_VER
    Sleep(1);
#else
    usleep(1000);
#endif
  }

  if (node->getConnectNodeStatus() != NodeStarted ||
      node->getExitStatus() != ExitInvalid) {
    LOG_ERROR(
        "Request(%p) node(%p) invoke sendAudio command failed, current status "
        "is invalid. node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    return -(InvokeSendAudioFailed);
  }

#ifdef ENABLE_REQUEST_RECORDING
  node->updateNodeProcess("sendAudio", NodeSendAudio, true, dataSize);
#endif

  int ret = 0;
  if (type != ENCODER_NONE) {
    ret = node->addSlicedAudioDataBuffer(data, dataSize);
  } else {
    ret = node->addAudioDataBuffer(data, dataSize);
  }
#ifdef ENABLE_REQUEST_RECORDING
  node->updateNodeProcess("sendAudio", NodeSendAudio, false, 0);
#endif
  return ret;
}

int NlsEventNetWork::stop(INlsRequest *request) {
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return -(NodeEmpty);
  }

  /* invoke stop
   * Node未处于运行状态, 或正处于退出状态, 则当前不可调用stop.
   */
  if (node->getExitStatus() == ExitStopping) {
    LOG_WARN(
        "Request(%p) node(%p) has invoked stop, node status:%s, exit "
        "status:%s. skip ...",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return Success;
  } else if (node->getExitStatus() == ExitCancel) {
    LOG_WARN(
        "Request(%p) node(%p) has invoked cancel, node status:%s, exit "
        "status:%s. skip ...",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return Success;
  }

  if (node->getConnectNodeStatus() < NodeInvoking ||
      node->getConnectNodeStatus() >= NodeFailed ||
      node->getExitStatus() != ExitInvalid) {
    LOG_ERROR(
        "Request(%p) node(%p) invoke stop command failed, current status is "
        "invalid. node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return -(InvokeStopFailed);
  }

  int ret = node->cmdNotify(CmdStop, NULL);

  if (ret == Success && node->getSyncCallTimeout() > 0) {
    node->waitInvokeFinish();
    int error_code = node->getErrorCode();
    if (error_code != Success) {
      MUTEX_UNLOCK(_mtxThread);
      return -(error_code);
    }
  }

  MUTEX_UNLOCK(_mtxThread);
  return ret;
}

int NlsEventNetWork::cancel(INlsRequest *request) {
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return -(NodeEmpty);
  }

  /* invoke cancel
   * Node未处于运行状态, 或正处于退出状态, 则当前不可调用stop.
   */
  if (node->getExitStatus() == ExitCancel) {
    LOG_WARN(
        "Request(%p) node(%p) has invoked cancel, node status:%s, exit "
        "status:%s. skip ...",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return Success;
  }
  if (node->getConnectNodeStatus() < NodeInvoking ||
      node->getConnectNodeStatus() >= NodeClosed) {
    LOG_ERROR(
        "Request(%p) node(%p) invoke cancel command failed, current status is "
        "invalid. node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return -(InvokeCancelFailed);
  }

  int ret = node->cmdNotify(CmdCancel, NULL);

  // NodeConnecting状态尽量不做操作, 500ms
  int try_count = 100;
  while (try_count-- > 0 && node->getConnectNodeStatus() == NodeConnecting) {
#if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
    Sleep(5);
    WaitForSingleObject(_mtxThread, INFINITE);
#else
    pthread_mutex_unlock(&_mtxThread);
    usleep(5 * 1000);
    pthread_mutex_lock(&_mtxThread);
#endif
  }

  // LOG_DUMP_EVENTS(node->getEventThread()->_workBase);
  MUTEX_UNLOCK(_mtxThread);
  return ret;
}

int NlsEventNetWork::stControl(INlsRequest *request, const char *message) {
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return -(NodeEmpty);
  }

  /* invoke stControl
   * Node未处于started状态, 或处于退出状态, 则当前不可调用stControl.
   */
  if (node->getConnectNodeStatus() != NodeStarted ||
      node->getExitStatus() != ExitInvalid) {
    LOG_ERROR(
        "Request(%p) node(%p) invoke stControl command failed, current status "
        "is invalid. node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return -(InvokeStControlFailed);
  }

  int ret = node->cmdNotify(CmdStControl, message);

  MUTEX_UNLOCK(_mtxThread);
  return ret;
}

int NlsEventNetWork::sendText(INlsRequest *request, const char *text) {
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return -(NodeEmpty);
  }

  /* Node也许处于Starting状态还未到Started状态, 可等待一会. */
  int try_count = 500;
  while (try_count-- > 0 && node->getConnectNodeStatus() == NodeStarting) {
#ifdef _MSC_VER
    Sleep(1);
#else
    usleep(1000);
#endif
  }

  /* invoke sendText
   * Node未处于started状态, 或处于退出状态, 则当前不可调用sendText.
   */
  if (node->getConnectNodeStatus() != NodeStarted ||
      node->getExitStatus() != ExitInvalid) {
    LOG_ERROR(
        "Request(%p) node(%p) invoke sendText command failed, current status "
        "is invalid. node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return -(InvokeSendTextFailed);
  }

  int ret = node->cmdNotify(CmdSendText, text);

  MUTEX_UNLOCK(_mtxThread);
  return ret;
}

int NlsEventNetWork::sendPing(INlsRequest *request) {
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return -(NodeEmpty);
  }

  /* Node也许处于Starting状态还未到Started状态, 可等待一会. */
  int try_count = 500;
  while (try_count-- > 0 && node->getConnectNodeStatus() == NodeStarting) {
#ifdef _MSC_VER
    Sleep(1);
#else
    usleep(1000);
#endif
  }

  /* invoke sendPing
   * Node未处于started状态, 或处于退出状态, 则当前不可调用sendPing.
   */
  if (node->getConnectNodeStatus() != NodeStarted ||
      node->getExitStatus() != ExitInvalid) {
    LOG_ERROR(
        "Request(%p) node(%p) invoke sendPing command failed, current status "
        "is invalid. node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return -(InvokeSendTextFailed);
  }

  int ret = node->cmdNotify(CmdSendPing, NULL);

  MUTEX_UNLOCK(_mtxThread);
  return ret;
}

int NlsEventNetWork::sendFlush(INlsRequest *request) {
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return -(NodeEmpty);
  }

  /* Node也许处于Starting状态还未到Started状态, 可等待一会. */
  int try_count = 500;
  while (try_count-- > 0 && node->getConnectNodeStatus() == NodeStarting) {
#ifdef _MSC_VER
    Sleep(1);
#else
    usleep(1000);
#endif
  }

  /* invoke sendFlush
   * Node未处于started状态, 或处于退出状态, 则当前不可调用sendFlush.
   */
  if (node->getConnectNodeStatus() != NodeStarted ||
      node->getExitStatus() != ExitInvalid) {
    LOG_ERROR(
        "Request(%p) node(%p) invoke sendFlush command failed, current status "
        "is invalid. node status:%s, exit status:%s.",
        request, node, node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    MUTEX_UNLOCK(_mtxThread);
    return -(InvokeSendTextFailed);
  }

  int ret = node->cmdNotify(CmdSendFlush, NULL);

  MUTEX_UNLOCK(_mtxThread);
  return ret;
}

const char *NlsEventNetWork::dumpAllInfo(INlsRequest *request) {
#ifdef ENABLE_REQUEST_RECORDING
  MUTEX_LOCK(_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    MUTEX_UNLOCK(_mtxThread);
    return NULL;
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!",
              request);
    MUTEX_UNLOCK(_mtxThread);
    return NULL;
  }

  std::string info(node->dumpAllInfo());

  MUTEX_UNLOCK(_mtxThread);
  return info.c_str();
#else
  return NULL;
#endif
}

}  // namespace AlibabaNls
