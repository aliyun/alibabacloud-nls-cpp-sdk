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

#include <string.h>
#include "asyncBase.h"
#include "thread.h"
#include "util/log.h"

using namespace util;

namespace transport{

namespace engine{

bool AsyncBase::canLoopContinue() {
    pthread_mutex_lock(&_operationMutex);
    bool res = false;
    if (_status == Status_Started) {
        res = true;
    }
    pthread_mutex_unlock(&_operationMutex);

    return res;
}

ThreadWorkerStatus AsyncBase::asyncBaseStatus() {
    return _status;
}

void AsyncBase::stopAsyncBase() {
    pthread_mutex_lock(&_operationMutex);
    _status = Status_Stopped;
    pthread_mutex_unlock(&_operationMutex);
}

void AsyncBase::cancleAsyncBase() {

#ifndef _ANDRIOD_
    pthread_cancel(_workingThread);
#endif

}

void* thread_func(void * arg) {
    AsyncBase* asyc = (AsyncBase*)arg;

    LOG_DEBUG("child id: %lu, parent id: %lu.\n", PthreadSelf(), asyc->_pid);

    SetThreadName(asyc->_ThreadName.c_str());
    asyc->workloop();

    return 0;
}

void AsyncBase::startAsyncBase() {
    pthread_mutex_lock(&_operationMutex);
    if (_status == Status_Init) {
        _status = Status_Started;
        this->_pid = PthreadSelf();
        pthread_create(&_workingThread, NULL, &thread_func, (void*)this);
    }
    pthread_mutex_unlock(&_operationMutex);
}

AsyncBase::~AsyncBase() {
    MuteAllExceptions(&AsyncBase::stopAsyncBase, this);
    pthread_join(_workingThread, NULL);
    pthread_mutex_destroy(&_operationMutex);
}

AsyncBase::AsyncBase(const std::string& name) : _status(Status_Init),
                                                _ThreadName(name) {
    pthread_mutex_init(&_operationMutex, NULL);
    memset(&_pid, 0, sizeof(_pid));
}

}

}
