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

#include "log.h"
#include "iNlsRequestListener.h"

using std::string;
using std::queue;

namespace AlibabaNls {

using namespace util;

INlsRequestListener::INlsRequestListener() {
    pthread_mutex_init(&_queueMutex, NULL);
}

INlsRequestListener::~INlsRequestListener() {
    pthread_mutex_destroy(&_queueMutex);
}

void INlsRequestListener::handlerFrame(string errorInfo,
                                  int errorCode,
                                  NlsEvent::EventType type,
                                  string taskId) {
    NlsEvent* nlsevent = new NlsEvent(errorInfo, errorCode, type, taskId);
    handlerFrame(*nlsevent);
    delete nlsevent;

    if (NlsEvent::TaskFailed == type) {
        LOG_ERROR(errorInfo.c_str());
    } else {
        LOG_DEBUG(errorInfo.c_str());
    }
}

int INlsRequestListener::getRecognizerResult(queue<NlsEvent>* eventQueue) {

    pthread_mutex_lock(&_queueMutex);
    while (!_eventQueue.empty()) {
        eventQueue->push(_eventQueue.front());
        _eventQueue.pop();
    }
    pthread_mutex_unlock(&_queueMutex);

    return 0;
}

}
