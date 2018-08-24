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

#ifndef NLS_SDK_ASYNC_BASE_H
#define NLS_SDK_ASYNC_BASE_H

#include "util/errorHandlingUtility.h"
#include "pthread.h"

namespace transport{
namespace engine{

enum ThreadWorkerStatus {
    Status_Init,
    Status_Stopped,
    Status_Started
};

class AsyncBase {
public:
    AsyncBase(const std::string& name);
    virtual ~AsyncBase(void);
    virtual void workloop() = 0;
    virtual int onErrorCatched(std::string data)=0;

protected:
    void startAsyncBase();
    void stopAsyncBase();
    void cancleAsyncBase();
    ThreadWorkerStatus asyncBaseStatus();
    virtual bool canLoopContinue();

protected:
    pthread_t _workingThread;
    pthread_mutex_t _operationMutex;

public:
    ThreadWorkerStatus _status;
    std::string _ThreadName;
    unsigned long _pid;
};

}

}

#endif //NLS_SDK_ASYNC_BASE_H
