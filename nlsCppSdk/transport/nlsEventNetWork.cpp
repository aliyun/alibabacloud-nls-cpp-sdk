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

#include <iostream>
#include <stdint.h>
#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#include "event2/thread.h"
#include "event2/dns.h"

#include "nlsGlobal.h"
#include "nlsClient.h"
#include "iNlsRequest.h"
#include "nlog.h"
#include "utility.h"
#include "connectNode.h"
#include "workThread.h"
#include "nlsEventNetWork.h"
#include "nodeManager.h"

namespace AlibabaNls {

#define DEFAULT_OPUS_FRAME_SIZE 640

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32")

HANDLE NlsEventNetWork::_mtxThread = NULL;
#else
pthread_mutex_t NlsEventNetWork::_mtxThread = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsEventNetWork * NlsEventNetWork::_eventClient = NULL;

NlsEventNetWork::NlsEventNetWork() {
  _workThreadArray = NULL;
  _workThreadsNumber = 0;
  _currentCpuNumber = 0;
  _addrInFamily = 0;
  _directIp[64] = {0};
  _enableSysGetAddr = false;
}

NlsEventNetWork::~NlsEventNetWork() {}

void NlsEventNetWork::DnsLogCb(int w, const char *m) {
  LOG_DEBUG(m);
}

void NlsEventNetWork::initEventNetWork(
    NlsClient* instance, int count, char *aiFamily, char *directIp, bool sysGetAddr) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif
  int ret = Success;
#if defined(_MSC_VER)
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
  LOG_DEBUG("evthread_use_windows_thread.");
  ret = evthread_use_windows_threads();
#endif

  WORD wVersionRequested;
  WSADATA wsaData;

  wVersionRequested = MAKEWORD(2, 2);

  (void)WSAStartup(wVersionRequested, &wsaData);
#else
  LOG_DEBUG("evthread_use_pthreads.");
  ret = evthread_use_pthreads();
#endif
  if (ret != Success) {
    LOG_ERROR("Invoke evthread_use_pthreads failed, ret:%d.", ret);
    exit(1);
  }

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

  int cpuNumber = 1;
#if defined(_MSC_VER)
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  cpuNumber = (int)sysInfo.dwNumberOfProcessors;
#elif defined(__linux__) || defined(__ANDROID__)
  cpuNumber = (int)sysconf(_SC_NPROCESSORS_ONLN);
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

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif

  LOG_INFO("Init NlsEventNetWork done.");
  return;
}

void NlsEventNetWork::destroyEventNetWork() {
  LOG_INFO("Destroy NlsEventNetWork(%p) begin ...", _eventClient);
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  delete [] _workThreadArray;
  _workThreadArray = NULL;

 #if defined(_MSC_VER)
  CloseHandle(WorkThread::_mtxCpu);
#else
  pthread_mutex_destroy(&WorkThread::_mtxCpu);
#endif

  _workThreadsNumber = 0;
  _currentCpuNumber = 0;

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif

  LOG_INFO("Destroy NlsEventNetWork(%p) done.", _eventClient);
  return;
}

/*
 * Description: 选择工作进程
 * Return: 成功则返回工程进程号, 失败则返回负值.
 * Others:
 */
int NlsEventNetWork::selectThreadNumber() {
  int number = 0;

  if (_workThreadArray != NULL) {
    number = _currentCpuNumber;

    LOG_DEBUG("Select Thread NO:%d.", number);

    if (++_currentCpuNumber == _workThreadsNumber) {
      _currentCpuNumber = 0;
    }
    LOG_DEBUG("Next NO:%d, Total:%d.",
        _currentCpuNumber, _workThreadsNumber);
  } else {
    LOG_ERROR("WorkThread isn't startup. Please invoke startWorkThread() first.");
    number = -1;
  }

  return number;
}

int NlsEventNetWork::start(INlsRequest *request) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  if (_eventClient == NULL) {
    LOG_ERROR("NlsEventNetWork has destroyed, please invoke startWorkThread() first.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!", request);
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(NodeEmpty);
  }

  /* 长链接模式下, 若为Completed状态, 则等待其进入Closed后, 再重置状态. */
  if (node->_isLongConnection) {
    int try_count = 500;
    while (try_count-- > 0 && node->getConnectNodeStatus() == NodeCompleted) {
    #ifdef _MSC_VER
      Sleep(1);
    #else
      usleep(1000);
    #endif
    }

    if (node->getConnectNodeStatus() == NodeClosed) {
      LOG_DEBUG("Node:%p current is NodeClosed and longConnection mode, reset status.", node);
      node->initAllStatus();
    }
  }

  /* invoke start
   * Node处于刚创建完状态, 且处于非退出状态, 则可进行start操作.
   */
  if (node->getConnectNodeStatus() == NodeCreated && node->getExitStatus() == ExitInvalid) {

    node->setConnectNodeStatus(NodeInvoking);

    int num = request->getThreadNumber();
    if (num < 0) {
      num = selectThreadNumber();
    }
    if (num < 0) {
      node->setConnectNodeStatus(NodeCreated);
    #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
    #else
      pthread_mutex_unlock(&_mtxThread);
    #endif
      return -(SelectThreadFailed);
    } else {
      request->setThreadNumber(num);
    }

    LOG_DEBUG("Request(%p) node(%p) select NO:%d thread.", request, node, num);

    node->_eventThread = &_workThreadArray[num];
    node->_eventThread->setInstance(_instance);
    node->setInstance(_instance);
    node->_eventThread->setAddrInFamily(_addrInFamily);
    if (_directIp != NULL && strnlen(_directIp, 64) > 0) {
      node->_eventThread->setDirectHost(_directIp);
    }
    node->_eventThread->setUseSysGetAddrInfo(_enableSysGetAddr);

    int ret = WorkThread::insertQueueNode(node->_eventThread, request);
    if (ret != Success) {
      LOG_ERROR("Request(%p) node(%p) insertQueueNode failed, ret:%d.",
          request, node, ret);

      node->setConnectNodeStatus(NodeCreated);
    #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
    #else
      pthread_mutex_unlock(&_mtxThread);
    #endif
      return ret;
    }

    LOG_DEBUG("Request(%p) node(%p) send fd:%d.", request, node, node->_eventThread->_notifySendFd);

    char cmd = 'c';
    ret = send(node->_eventThread->_notifySendFd,
                   (char *)&cmd, sizeof(char), 0);
    if (ret < 1) {
      LOG_ERROR("Request(%p) node(%p) send start command failed, errno:%d ret:%d.",
          request, node, utility::getLastErrorCode(), ret);

      node->setConnectNodeStatus(NodeCreated);
    #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
    #else
      pthread_mutex_unlock(&_mtxThread);
    #endif
      return -(StartCommandFailed);
    }
  } else if (node->getExitStatus() == ExitInvalid &&
      node->getConnectNodeStatus() > NodeCreated &&
      node->getConnectNodeStatus() < NodeFailed) {
    LOG_WARN("Request(%p) node(%p) has invoked start, node status:%s, exit status:%s. skip ...",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());

  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return Success;
  } else {
    LOG_ERROR("Request(%p) node(%p) invoke start failed, current status is invalid. node status:%s, exit status:%s.",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());

    node->setConnectNodeStatus(NodeCreated);
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(InvokeStartFailed);
  }

  node->initNlsEncoder();

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif

  return Success;
}

int NlsEventNetWork::sendAudio(INlsRequest *request, const uint8_t * data,
                               size_t dataSize, ENCODER_TYPE type) {
  EVENT_CLIENT_CHECK(_eventClient);
  ConnectNode * node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request:%p is nullptr, you have destroyed request!", request);
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

  if (node->getConnectNodeStatus() != NodeStarted || node->getExitStatus() != ExitInvalid) {
    LOG_ERROR("Request(%p) node(%p) invoke sendAudio command failed, current status is invalid. node status:%s, exit status:%s.",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    return -(InvokeSendAudioFailed);
  }

  if (type == ENCODER_OPU) {
    if (dataSize != DEFAULT_OPUS_FRAME_SIZE) {
      LOG_ERROR("Request(%p) node(%p) the data size of OPU isn't 640bytes.", request, node);
      return -(InvalidOpusFrameSize);
    }
  }

  return node->addAudioDataBuffer(data, dataSize);
}

int NlsEventNetWork::stop(INlsRequest *request) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  if (_eventClient == NULL) {
    LOG_ERROR("NlsEventNetWork has destroyed, please invoke startWorkThread() first.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(EventClientEmpty);
  }

  ConnectNode * node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!", request);
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(NodeEmpty);
  }

  /* invoke stop
   * Node未处于运行状态, 或正处于退出状态, 则当前不可调用stop.
   */
  if (node->getExitStatus() == ExitStopping) {
    LOG_WARN("Request(%p) node(%p) has invoked stop, node status:%s, exit status:%s. skip ...",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return Success;
  } else if (node->getExitStatus() == ExitCancel) {
    LOG_WARN("Request(%p) node(%p) has invoked cancel, node status:%s, exit status:%s. skip ...",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return Success;
  }
  if (node->getConnectNodeStatus() < NodeInvoking ||
      node->getConnectNodeStatus() >= NodeFailed ||
      node->getExitStatus() != ExitInvalid) {
    LOG_ERROR("Request(%p) node(%p) invoke stop command failed, current status is invalid. node status:%s, exit status:%s.",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(InvokeStopFailed);
  }

  int ret = node->cmdNotify(CmdStop, NULL);

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif

  return ret;
}

int NlsEventNetWork::cancel(INlsRequest *request) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  if (_eventClient == NULL) {
    LOG_ERROR("NlsEventNetWork has destroyed, please invoke startWorkThread() first.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(EventClientEmpty);
  }

  ConnectNode * node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!", request);
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(NodeEmpty);
  }

  /* invoke cancel
   * Node未处于运行状态, 或正处于退出状态, 则当前不可调用stop.
   */
  if (node->getExitStatus() == ExitCancel) {
    LOG_WARN("Request(%p) node(%p) has invoked cancel, node status:%s, exit status:%s. skip ...",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return Success;
  }
  if (node->getConnectNodeStatus() < NodeInvoking ||
      node->getConnectNodeStatus() >= NodeClosed) {
        LOG_ERROR("Request(%p) node(%p) invoke cancel command failed, current status is invalid. node status:%s, exit status:%s.",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(InvokeCancelFailed);
  }

  int ret = node->cmdNotify(CmdCancel, NULL);

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  return ret;
}

int NlsEventNetWork::stControl(INlsRequest *request, const char* message) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  if (_eventClient == NULL) {
    LOG_ERROR("NlsEventNetWork has destroyed, please invoke startWorkThread() first.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(EventClientEmpty);
  }

  ConnectNode * node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("The node of request(%p) is nullptr, you have destroyed request!", request);
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(NodeEmpty);
  }

  /* invoke stControl
   * Node未处于started状态, 或处于退出状态, 则当前不可调用stControl.
   */
  if (node->getConnectNodeStatus() != NodeStarted || node->getExitStatus() != ExitInvalid) {
    LOG_ERROR("Request(%p) node(%p) invoke stControl command failed, current status is invalid. node status:%s, exit status:%s.",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    return -(InvokeStControlFailed);
  }

  int ret = node->cmdNotify(CmdStControl, message);

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  return ret;
}

NlsClient* NlsEventNetWork::getInstance() {
  return _instance;
}

}  // namespace AlibabaNls
