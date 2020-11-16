/*
 * Copyright 2015 Alibaba Group Holding Limited
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

#if defined(__linux__)
#include <unistd.h>
#endif

#include "iNlsRequest.h"
#include "event2/thread.h"
#include "event2/dns.h"
#include "log.h"
#include "utility.h"
#include "connectNode.h"
#include "workThread.h"
#include "eventNetWork.h"

namespace AlibabaNls {

using std::cout;
using std::endl;
using std::vector;

using namespace utility;

#define DEFAULT_OPU_FRAME_SIZE 640

#if defined(_WIN32)
#pragma comment(lib, "ws2_32")
#endif

WorkThread *NlsEventClientNetWork::_workThreadArray = NULL;
size_t NlsEventClientNetWork::_workThreadsNumber = 0;
size_t NlsEventClientNetWork::_currentCpuNumber = 0;
#if defined(_WIN32)
HANDLE NlsEventClientNetWork::_mtxThreadNumber = CreateMutex(NULL, FALSE, NULL);
#else
pthread_mutex_t  NlsEventClientNetWork::_mtxThreadNumber = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsEventClientNetWork * NlsEventClientNetWork::_eventClient = new NlsEventClientNetWork();

/*
 * 1: init libevent thread
 *
 * */
NlsEventClientNetWork::NlsEventClientNetWork() {

}

NlsEventClientNetWork::~NlsEventClientNetWork() {

}

void NlsEventClientNetWork::DnsLogCb(int w, const char *m) {
    LOG_DEBUG(m);
}

void NlsEventClientNetWork::initEventNetWork(int count) {
#if defined(_WIN32)
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
	evthread_use_windows_threads();
	LOG_DEBUG("evthread_use_windows_thread.");
#endif
#else
    LOG_DEBUG("evthread_use_pthreads.");
    evthread_use_pthreads();
#endif

#if defined(_WIN32)
    WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2, 2);

	(void) WSAStartup(wVersionRequested, &wsaData);
#endif

#if defined(__ANDROID__)
    WorkThread::_cpuNumber = 1;
#elif defined(_WIN32)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    WorkThread::_cpuNumber = (int)sysInfo.dwNumberOfProcessors;
#elif defined(__linux__)
    WorkThread::_cpuNumber = (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(__APPLE__)
    WorkThread::_cpuNumber = 1;
#endif

    if (count <= 0) {
        _workThreadsNumber = WorkThread::_cpuNumber;
    } else {
        _workThreadsNumber = count;
    }
    LOG_DEBUG("Work threads number: %d", _workThreadsNumber);

    _workThreadArray = new WorkThread[_workThreadsNumber];

    evdns_set_log_fn(DnsLogCb);

    LOG_INFO("Init ClientNetWork done.");

    return ;
}

void NlsEventClientNetWork::destroyEventNetWork() {

    LOG_INFO("destroy NlsEventClientNetWork begin.");

    delete [] _workThreadArray;

    delete _eventClient;
    _eventClient = NULL;

    LOG_INFO("destroy NlsEventClientNetWork done.");

    return ;
}

int NlsEventClientNetWork::selectThreadNumber() {
    int number = 0;

#if defined(_WIN32)
    WaitForSingleObject(_mtxThreadNumber, INFINITE);
#else
    pthread_mutex_lock(&_mtxThreadNumber);
#endif

    if (_workThreadArray != NULL) {
#if defined(__ANDRIOD__)
        number = 0;
#elif defined(__APPLE__)
        number = 0;
#else
        number = _currentCpuNumber;

        LOG_DEBUG("Select NO.%d", number);

        if (++_currentCpuNumber == _workThreadsNumber) {
            _currentCpuNumber = 0;
        }
#endif
        LOG_DEBUG("Next NO.%d , Total:%d.", _currentCpuNumber, _workThreadsNumber);
    } else {
        LOG_DEBUG("WorkThread is n't startup.");
        number = -1;
    }

#if defined(_WIN32)
    ReleaseMutex(_mtxThreadNumber);
#else
    pthread_mutex_unlock(&_mtxThreadNumber);
#endif

    return number;
}

/*
 * param: request * p;
 * */
int NlsEventClientNetWork::start(INlsRequest *request) {
    ConnectNode *node = request->getConnectNode();

    if ((node->getConnectNodeStatus() == NodeInitial) && (node->getExitStatus() == ExitInvalid)) {
        int num = selectThreadNumber();
        if (num == -1) {
            return -1;
        }

        LOG_INFO("Node:%p Select NO.%d thread.", node, num);

        node->_eventThread = &_workThreadArray[num];
        WorkThread::insertQueueNode(node->_eventThread, request);
        node->resetBufferLimit();

        char cmd = 'c';
        int ret = send(node->_eventThread->_notifySendFd, (char *)&cmd, sizeof(char), 0);
        if (ret < 1) {
            LOG_ERROR("Node:%p Start command is failed.", node);
            return -1;
        }
        node->setConnectNodeStatus(NodeConnecting);
    } else {
        LOG_ERROR("Node:%p Invoke start failed:%d, %d.", node, node->getConnectNodeStatus(), node->getExitStatus());
        return -1;
    }

    return 0;
}

int NlsEventClientNetWork::sendAudio(INlsRequest *request, const uint8_t * data, size_t dataSize, bool encoded) {
    ConnectNode * node = request->getConnectNode();

    if ((node->getConnectNodeStatus() == NodeInitial) || (node->getExitStatus() != ExitInvalid)) {
        LOG_ERROR("Node:%p Invoke command failed.", node);
        return -1;
    }

//    LOG_DEBUG("Node:%p sendAudio Size %zu.", node, dataSize);

    if (encoded) {
        if (dataSize != DEFAULT_OPU_FRAME_SIZE) {
            LOG_ERROR("Node:%p The Opu data size is n't 640.", node);
            return -1;
        }

        uint8_t outputBuffer[DEFAULT_OPU_FRAME_SIZE] = {0};
        
        node->initOpuEncoder();
      
        int nSize = opuEncoder(node->_opuEncoder,
                               data,
                               (int)dataSize,
                               outputBuffer,
                               DEFAULT_FRAME_NORMAL_SIZE);
        if (nSize < 0) {
            LOG_ERROR("Node:%p Opu encoder failed %d.", node, nSize);
            return nSize;
        }

        return node->addAudioDataBuffer(outputBuffer, nSize);
    } else {
        return node->addAudioDataBuffer(data, dataSize);
    }

}

int NlsEventClientNetWork::stop(INlsRequest *request, int type) {
    ConnectNode * node = request->getConnectNode();

    if ((node->getConnectNodeStatus() == NodeInitial) || (node->getExitStatus() != ExitInvalid)) {
        LOG_ERROR("Node:%p Invoke command failed.", node);
        return -1;
    }

    LOG_INFO("Node:%p call stop %d.", node, type);

    if (type == 0) {
        return node->cmdNotify(CmdStop, NULL);
    } else if (type == 1) {
        return node->cmdNotify(CmdCancel, NULL);
    } else if (type == 2) {
        return node->cmdNotify(CmdWarkWord, NULL);
    } else {
        return -1;
    }
}

int NlsEventClientNetWork::stControl(INlsRequest *request, const char* message) {
    ConnectNode * node = request->getConnectNode();

    if ((node->getConnectNodeStatus() == NodeInitial) || (node->getExitStatus() != ExitInvalid)) {
        LOG_ERROR("Node:%p Invoke command failed.", node);
        return -1;
    }

    LOG_INFO("Node:%p call stConreol.", node);

    return node->cmdNotify(CmdStControl, message);
}

}
