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
#include <unistd.h>

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

WorkThread *NlsEventNetWork::_workThreadArray = NULL;
size_t NlsEventNetWork::_workThreadsNumber = 0;
size_t NlsEventNetWork::_currentCpuNumber = 0;
pthread_mutex_t NlsEventNetWork::_mtxThreadNumber = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t NlsEventNetWork::_mtxThread = PTHREAD_MUTEX_INITIALIZER;
NlsEventNetWork * NlsEventNetWork::_eventClient = new NlsEventNetWork();

NlsEventNetWork::NlsEventNetWork() {}
NlsEventNetWork::~NlsEventNetWork() {}

void NlsEventNetWork::DnsLogCb(int w, const char *m) {
  LOG_DEBUG(m);
}

void NlsEventNetWork::initEventNetWork(int count) {
  LOG_DEBUG("evthread_use_pthreads.");
  pthread_mutex_lock(&_mtxThread);

  evthread_use_pthreads();

  WorkThread::_cpuNumber = (int)sysconf(_SC_NPROCESSORS_ONLN);

  if (count <= 0) {
    _workThreadsNumber = WorkThread::_cpuNumber;
  } else {
    _workThreadsNumber = count;
  }
  LOG_DEBUG("Work threads number: %d", _workThreadsNumber);

  _workThreadArray = new WorkThread[_workThreadsNumber];

  evdns_set_log_fn(DnsLogCb);

  pthread_mutex_unlock(&_mtxThread);

  LOG_INFO("Init ClientNetWork done.");
  return;
}

void NlsEventNetWork::destroyEventNetWork() {
  LOG_INFO("destroy NlsEventNetWork begin.");
  pthread_mutex_lock(&_mtxThread);

  delete [] _workThreadArray;
  _workThreadArray = NULL;

  pthread_mutex_unlock(&_mtxThread);

  delete _eventClient;
  _eventClient = NULL;

  LOG_INFO("destroy NlsEventNetWork done.");
  return;
}

int NlsEventNetWork::selectThreadNumber() {
  int number = 0;
  pthread_mutex_lock(&_mtxThreadNumber);

  if (_workThreadArray != NULL) {
    number = _currentCpuNumber;

    LOG_DEBUG("Select NO.%d", number);

    if (++_currentCpuNumber == _workThreadsNumber) {
      _currentCpuNumber = 0;
    }
    LOG_DEBUG("Next NO.%d , Total:%d.",
        _currentCpuNumber, _workThreadsNumber);
  } else {
    LOG_DEBUG("WorkThread is n't startup.");
    number = -1;
  }

  pthread_mutex_unlock(&_mtxThreadNumber);
  return number;
}

int NlsEventNetWork::start(INlsRequest *request) {
  pthread_mutex_lock(&_mtxThread);

  ConnectNode *node = request->getConnectNode();

  if (node && (node->getConnectNodeStatus() == NodeInitial) &&
      (node->getExitStatus() == ExitInvalid)) {
    int num = selectThreadNumber();
    if (num == -1) {
      pthread_mutex_unlock(&_mtxThread);
      return -1;
    }

    LOG_INFO("Node:%p Select NO.%d thread.", node, num);

    node->_eventThread = &_workThreadArray[num];
    WorkThread::insertQueueNode(node->_eventThread, request);
    node->resetBufferLimit();

    char cmd = 'c';
    int ret = send(node->_eventThread->_notifySendFd,
                   (char *)&cmd, sizeof(char), 0);
    if (ret < 1) {
      LOG_ERROR("Node:%p Start command is failed.", node);
      pthread_mutex_unlock(&_mtxThread);
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
    pthread_mutex_unlock(&_mtxThread);
    return -1;
  }

  node->initNlsEncoder();

  pthread_mutex_unlock(&_mtxThread);
  return 0;
}

int NlsEventNetWork::sendAudio(INlsRequest *request, const uint8_t * data,
                               size_t dataSize, ENCODER_TYPE type) {
  int ret = -1;
  ConnectNode * node = request->getConnectNode();
//  LOG_DEBUG("Node:%p sendAudio -1", node);

  pthread_mutex_lock(&_mtxThread);

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed.", node);
    pthread_mutex_unlock(&_mtxThread);
    return -1;
  }

  //LOG_DEBUG("Node:%p sendAudio Type:%d, Size %zu.", node, type, dataSize);
//  LOG_DEBUG("Node:%p sendAudio NodeStatus:%s, ExitStatus:%s",
//      node,
//      node->getConnectNodeStatusString(node->getConnectNodeStatus()).c_str(),
//      node->getExitStatusString(node->getExitStatus()).c_str());

  if (type == ENCODER_OPUS || type == ENCODER_OPU) {
//    if (dataSize != DEFAULT_OPUS_FRAME_SIZE) {
//      LOG_ERROR("Node:%p The Opus data size is n't 640.", node);
//      return -1;
//    }

//    node->initNlsEncoder();

//    uint8_t outputBuffer[DEFAULT_OPUS_FRAME_SIZE] = {0};
    uint8_t *outputBuffer = new uint8_t[dataSize];
    if (outputBuffer == NULL) {
      LOG_ERROR("Node:%p new outputBuffer failed", node);
      pthread_mutex_unlock(&_mtxThread);
      return -1;
    }
    memset(outputBuffer, 0, dataSize);
    int nSize = nlsEncoding(node->_nlsEncoder, type,
                            data, (int)dataSize,
                            outputBuffer, (int)dataSize);
//                            outputBuffer, DEFAULT_FRAME_NORMAL_SIZE);
    if (nSize < 0) {
      LOG_ERROR("Node:%p Opus encoder failed %d.", node, nSize);
      delete[] outputBuffer;
      pthread_mutex_unlock(&_mtxThread);
      return nSize;
    }

    ret = node->addAudioDataBuffer(outputBuffer, nSize);

    if (outputBuffer) delete[] outputBuffer;
  } else {
    // type == ENCODER_NONE
//    LOG_DEBUG("Node:%p sendAudio 0", node);
    ret = node->addAudioDataBuffer(data, dataSize);
//    LOG_DEBUG("Node:%p sendAudio 1", node);
  }
  pthread_mutex_unlock(&_mtxThread);
//  LOG_DEBUG("Node:%p sendAudio 2", node);
  return ret;
}

int NlsEventNetWork::stop(INlsRequest *request, int type) {
  pthread_mutex_lock(&_mtxThread);

  ConnectNode * node = request->getConnectNode();

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed. Status:%s and %s",
        node,
        node->getConnectNodeStatusString().c_str(),
        node->getExitStatusString().c_str());
    pthread_mutex_unlock(&_mtxThread);
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

  pthread_mutex_unlock(&_mtxThread);
  return ret;
}

int NlsEventNetWork::stControl(INlsRequest *request, const char* message) {
  pthread_mutex_lock(&_mtxThread);

  ConnectNode * node = request->getConnectNode();

  if ((node->getConnectNodeStatus() == NodeInitial) ||
      (node->getExitStatus() != ExitInvalid)) {
    LOG_ERROR("Node:%p Invoke command failed.", node);
    pthread_mutex_unlock(&_mtxThread);
    return -1;
  }

  int ret = node->cmdNotify(CmdStControl, message);

  pthread_mutex_unlock(&_mtxThread);
  LOG_INFO("Node:%p call stConreol.", node);
  return ret;
}

}  // namespace AlibabaNls
