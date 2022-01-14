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
#include "iNlsRequest.h"
#include "nlog.h"
#include "utility.h"
#include "connectNode.h"
#include "workThread.h"
#include "nlsEventNetWork.h"

namespace AlibabaNls {

#define DEFAULT_OPUS_FRAME_SIZE 640

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32")
#endif

WorkThread *NlsEventNetWork::_workThreadArray = NULL;
size_t NlsEventNetWork::_workThreadsNumber = 0;
size_t NlsEventNetWork::_currentCpuNumber = 0;

#if defined(_MSC_VER)
HANDLE NlsEventNetWork::_mtxThread = CreateMutex(NULL, FALSE, NULL);
#else
pthread_mutex_t NlsEventNetWork::_mtxThread = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsEventNetWork * NlsEventNetWork::_eventClient = new NlsEventNetWork();

NlsEventNetWork::NlsEventNetWork() {}
NlsEventNetWork::~NlsEventNetWork() {}

void NlsEventNetWork::DnsLogCb(int w, const char *m) {
  LOG_DEBUG(m);
}

void NlsEventNetWork::initEventNetWork(int count) {
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

#if defined(_MSC_VER)
  SYSTEM_INFO sysInfo;
  GetSystemInfo(&sysInfo);
  WorkThread::_cpuNumber = (int)sysInfo.dwNumberOfProcessors;
#elif defined(__linux__) || defined(__ANDROID__)
  WorkThread::_cpuNumber = (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif

  if (count <= 0) {
    _workThreadsNumber = WorkThread::_cpuNumber;
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

  LOG_DEBUG("Init ClientNetWork done.");
  return;
}

void NlsEventNetWork::destroyEventNetWork() {
  LOG_INFO("destroy NlsEventNetWork begin.");
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  delete [] _workThreadArray;
  _workThreadArray = NULL;

  _workThreadsNumber = 0;
  _currentCpuNumber = 0;

  delete _eventClient;
  _eventClient = NULL;

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif

  LOG_INFO("destroy NlsEventNetWork done.");
  return;
}

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
    LOG_DEBUG("WorkThread is n't startup.");
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

  ConnectNode *node = request->getConnectNode();

  if (node && (node->getConnectNodeStatus() == NodeInitial) &&
      (node->getExitStatus() == ExitInvalid)) {
    int num = selectThreadNumber();
    if (num == -1) {
    #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
    #else
      pthread_mutex_unlock(&_mtxThread);
    #endif
      return -1;
    }

    LOG_DEBUG("Node:%p Select NO.%d thread.", node, num);

    node->_eventThread = &_workThreadArray[num];
    WorkThread::insertQueueNode(node->_eventThread, request);
    node->resetBufferLimit();

    char cmd = 'c';
    int ret = send(node->_eventThread->_notifySendFd,
                   (char *)&cmd, sizeof(char), 0);
    if (ret < 1) {
      LOG_ERROR("Node:%p Start command is failed.", node);
      #if defined(_MSC_VER)
      ReleaseMutex(_mtxThread);
      #else
      pthread_mutex_unlock(&_mtxThread);
      #endif
      return -1;
    }
    node->setConnectNodeStatus(NodeConnecting);
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
    return -1;
  }

  node->initNlsEncoder();

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  return 0;
}

int NlsEventNetWork::sendAudio(INlsRequest *request, const uint8_t * data,
                               size_t dataSize, ENCODER_TYPE type) {
  int ret = -1;
  ConnectNode * node = request->getConnectNode();

  // has no mtx in CppSdk3.0
//  pthread_mutex_lock(&_mtxThread);

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed.", node);
//    pthread_mutex_unlock(&_mtxThread);
    return -1;
  }

//  LOG_DEBUG("Node:%p sendAudio Type:%d, Size %zu.", node, type, dataSize);
//  LOG_DEBUG("Node:%p sendAudio NodeStatus:%s, ExitStatus:%s",
//      node,
//      node->getConnectNodeStatusString(node->getConnectNodeStatus()).c_str(),
//      node->getExitStatusString(node->getExitStatus()).c_str());

  if (type == ENCODER_OPU) {
    if (dataSize != DEFAULT_OPUS_FRAME_SIZE) {
//      pthread_mutex_unlock(&_mtxThread);
      LOG_ERROR("Node:%p The Opus data size is n't 640.", node);
      return -1;
    }
  }

  ret = node->addAudioDataBuffer(data, dataSize);

//  pthread_mutex_unlock(&_mtxThread);
  return ret;
}

int NlsEventNetWork::stop(INlsRequest *request, int type) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  ConnectNode * node = request->getConnectNode();

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed. Status:%s and %s",
        node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
#if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
#else
    pthread_mutex_unlock(&_mtxThread);
#endif
    return -1;
  }

  LOG_INFO("Node:%p call stop %d.", node, type);

  int ret = -1;
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
  return ret;
}

int NlsEventNetWork::stControl(INlsRequest *request, const char* message) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxThread, INFINITE);
#else
  pthread_mutex_lock(&_mtxThread);
#endif

  ConnectNode * node = request->getConnectNode();

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed.", node);
#if defined(_MSC_VER)
    ReleaseMutex(_mtxThread);
#else
    pthread_mutex_unlock(&_mtxThread);
#endif
    return -1;
  }

  int ret = node->cmdNotify(CmdStControl, message);

#if defined(_MSC_VER)
  ReleaseMutex(_mtxThread);
#else
  pthread_mutex_unlock(&_mtxThread);
#endif
  LOG_INFO("Node:%p call stConreol.", node);
  return ret;
}

}  // namespace AlibabaNls
