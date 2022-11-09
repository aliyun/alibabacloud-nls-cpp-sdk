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

int NlsEventNetWork::_opCount = 0;
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

void NlsEventNetWork::tryCreateMutex() {
#if defined(_MSC_VER)
  _mtxThread = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxThread, NULL);
#endif
}

void NlsEventNetWork::tryDestroyMutex() {
  LOG_DEBUG("destroy _mtxThread enter");
  int cnt = 0;
  const int loop = 5000;
  while (_opCount > 0 && cnt++ < loop) {
  #ifdef _MSC_VER
    Sleep(1);
  #else
    usleep(1000);
  #endif
  }
  if (cnt >= loop) {
    LOG_WARN("wait timeout... opCount(%d)", _opCount);
  }
#if defined(_MSC_VER)
  CloseHandle(_mtxThread);
#else
  pthread_mutex_destroy(&_mtxThread);
#endif
  LOG_DEBUG("destroy _mtxThread done");
}

void NlsEventNetWork::initEventNetWork(
    NlsClient* instance, int count, char *aiFamily, char *directIp, bool sysGetAddr) {
  _opCount++;
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

#if defined(_MSC_VER)
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
  LOG_DEBUG("evthread_use_windows_thread.");
  evthread_use_windows_threads();
#endif
#else
  LOG_DEBUG("evthread_use_pthreads.");
  evthread_use_pthreads();
#endif

#if defined(_MSC_VER)
  WORD wVersionRequested;
  WSADATA wsaData;

  wVersionRequested = MAKEWORD(2, 2);

  (void)WSAStartup(wVersionRequested, &wsaData);
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
    LOG_INFO("Set sockaddr_in type: %s", aiFamily);
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
  LOG_INFO("Work threads number: %d", _workThreadsNumber);

  _workThreadArray = new WorkThread[_workThreadsNumber];

  evdns_set_log_fn(DnsLogCb);

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  _opCount--;

  LOG_INFO("Init ClientNetWork done. opCount(%d)", _opCount);
  return;
}

void NlsEventNetWork::destroyEventNetWork() {
  LOG_INFO("destroy NlsEventNetWork(%p) begin.", _eventClient);
  int cnt = 0;
  const int loop = 5000;
  while (_opCount > 0 && cnt++ < loop) {
  #ifdef _MSC_VER
    Sleep(1);
  #else
    usleep(1000);
  #endif
  }
  if (cnt >= loop) {
    LOG_WARN("wait timeout... opCount(%d)", _opCount);
  }
  _opCount++;
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
  _opCount--;

  LOG_INFO("destroy NlsEventNetWork(%p) done. opCount(%d)", _eventClient, _opCount);
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

    LOG_DEBUG("Select Thread NO.%d", number);

    if (++_currentCpuNumber == _workThreadsNumber) {
      _currentCpuNumber = 0;
    }
    LOG_DEBUG("Next NO.%d , Total:%d.",
        _currentCpuNumber, _workThreadsNumber);
  } else {
    LOG_ERROR("WorkThread is n't startup. Please invoke startWorkThread(-1) first.");
    number = -1;
  }

  return number;
}

int NlsEventNetWork::start(INlsRequest *request) {
  _opCount++;
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  if (_eventClient == NULL) {
    LOG_ERROR("WorkThread has released.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    _opCount--;
    return -(EventClientEmpty);
  }

  ConnectNode *node = request->getConnectNode();
  if (node && (node->getConnectNodeStatus() == NodeInvalid) &&
      (node->getExitStatus() == ExitStopped)) {
    // set NodeInitial and ExitInvalid
    node->initAllStatus();
  }

  if (node && (node->getConnectNodeStatus() == NodeInitial) &&
      (node->getExitStatus() == ExitInvalid)) {
    int num = request->getThreadNumber();
    if (num < 0) {
      num = selectThreadNumber();
    }
    if (num < 0) {
    #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
    #else
      pthread_mutex_unlock(&_mtxThread);
    #endif
      _opCount--;
      return -(SelectThreadFailed);
    } else {
      request->setThreadNumber(num);
    }

    LOG_INFO("Node:%p Select NO.%d thread.", node, num);

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
      LOG_ERROR("Node:%p insertQueueNode failed, ret:%d", node, ret);
    #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
    #else
      pthread_mutex_unlock(&_mtxThread);
    #endif
      _opCount--;
      return ret;
    }
    node->resetBufferLimit();

    char cmd = 'c';
    ret = send(node->_eventThread->_notifySendFd,
                   (char *)&cmd, sizeof(char), 0);
    if (ret < 1) {
      LOG_ERROR("Node:%p Start command is failed.", node);
    #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
    #else
      pthread_mutex_unlock(&_mtxThread);
    #endif
      _opCount--;
      return -(StartCommandFailed);
    }
  } else {
    LOG_ERROR("Node:%p Invoke start failed:%d(%s), %d(%s).",
        node,
        node->getConnectNodeStatus(),
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatus(),
        node->getExitStatusString().c_str());
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
    #else
    pthread_mutex_unlock(&_mtxThread);
    #endif
    _opCount--;
    return -(InvokeStartFailed);
  }

  node->initNlsEncoder();

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  _opCount--;

  return 0;
}

int NlsEventNetWork::sendAudio(INlsRequest *request, const uint8_t * data,
                               size_t dataSize, ENCODER_TYPE type) {
  int ret = -(InvokeSendAudioFailed);
  int status = NodeStatusInvalid;

  // has no mtx in CppSdk3.0
//  pthread_mutex_lock(&_mtxThread);

  if (_eventClient == NULL) {
    LOG_ERROR("WorkThread has released.");
//    pthread_mutex_unlock(&_mtxThread);
    return -(EventClientEmpty);
  }

  NlsNodeManager* node_manager = (NlsNodeManager*)_eventClient->_instance->getNodeManger();
  ret = node_manager->checkRequestExist(request, &status);
  if (ret != Success) {
    LOG_ERROR("Request:%p checkRequestExist failed, ret:%d", request, ret);
    return ret;
  }

  ConnectNode * node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("Node:%p node is nullptr.", node);
    return -(NodeEmpty);
  }
  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed.", node);
//    pthread_mutex_unlock(&_mtxThread);
    return -(InvokeSendAudioFailed);
  }

//  LOG_DEBUG("Node:%p sendAudio Type:%d, Size %zu.", node, type, dataSize);
//  LOG_DEBUG("Node:%p sendAudio NodeStatus:%s, ExitStatus:%s",
//      node,
//      node->getConnectNodeStatusString(node->getConnectNodeStatus()).c_str(),
//      node->getExitStatusString(node->getExitStatus()).c_str());

  if (type == ENCODER_OPU) {
    if (dataSize != DEFAULT_OPUS_FRAME_SIZE) {
//      pthread_mutex_unlock(&_mtxThread);
      LOG_ERROR("Node:%p The Opus data size isn't 640.", node);
      return -(InvalidOpusFrameSize);
    }
  }

  ret = node->addAudioDataBuffer(data, dataSize);

//  pthread_mutex_unlock(&_mtxThread);
  return ret;
}

int NlsEventNetWork::stop(INlsRequest *request, int type) {
  _opCount++;
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  if (_eventClient == NULL) {
    LOG_ERROR("WorkThread has released.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    _opCount--;
    return -(EventClientEmpty);
  }

  ConnectNode * node = request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("Node is nullptr, you have destroyed request or relesed instance!");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    _opCount--;
    return -(NodeEmpty);
  }

  int ret = Success;
  int status = NodeStatusInvalid;
  NlsNodeManager* node_manager = (NlsNodeManager*)node->getInstance()->getNodeManger();
  ret = node_manager->checkRequestExist(request, &status);
  if (ret != Success) {
    LOG_ERROR("Request:%p checkRequestExist failed, ret:%d", request, ret);
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    _opCount--;
    return ret;
  }

  if (type == 1) {
    ret = node_manager->updateNodeStatus(node, NodeStatusCancelling);

    LOG_DEBUG("request:%p node:%p set CallbackCancelled.", request, node);
    node->setCallbackStatus(CallbackCancelled);
  }

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("request:%p node:%p Invoke command failed. Status:%s and %s",
        request, node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    _opCount--;
    return -(InvokeStopFailed);
  }

  LOG_INFO("request:%p node:%p call stop %d. (0:CmdStop, 1:CmdCancel)", request, node, type);

  ret = -1;
  if (type == 0) {
    ret = node->cmdNotify(CmdStop, NULL);
  } else if (type == 1) {
    ret = node->cmdNotify(CmdCancel, NULL);
  } else if (type == 2) {
    ret = node->cmdNotify(CmdWarkWord, NULL);
  } else {
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  _opCount--;
  return ret;
}

int NlsEventNetWork::stControl(INlsRequest *request, const char* message) {
  _opCount++;
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  if (_eventClient == NULL) {
    LOG_ERROR("WorkThread has released.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    _opCount--;
    return -(EventClientEmpty);
  }

  ConnectNode * node = request->getConnectNode();

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed.", node);
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
  #else
    pthread_mutex_unlock(&_mtxThread);
  #endif
    _opCount--;
    return -(InvokeStControlFailed);
  }

  int ret = node->cmdNotify(CmdStControl, message);

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  _opCount--;
  LOG_INFO("Node:%p call stConreol. opCount(%d)", node, _opCount);
  return ret;
}

NlsClient* NlsEventNetWork::getInstance() {
  return _instance;
}

}  // namespace AlibabaNls
