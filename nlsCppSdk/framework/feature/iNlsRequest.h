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

#ifndef NLS_SDK_SPEECH_REQUEST_H
#define NLS_SDK_SPEECH_REQUEST_H

#include <map>
#include <list>
#include <queue>
#include "nlsEvent.h"

#if defined(_WIN32)

#include "pthread.h"

#pragma warning( push )
#pragma warning ( disable : 4251 )

#else

#include <pthread.h>

#endif

namespace AlibabaNls {

class INlsRequestParam;
class NlsSessionBase;
class INlsRequestListener;

class INlsRequest {
public:

INlsRequest();
~INlsRequest();

int setUrl(const char* value);

int setAppKey(const char* value);

int setToken(const char* value);

int setFormat(const char* value);

int setSampleRate(int value);

int setTimeout(int value);

int setOutputFormat(const char* value);

int setPayloadParam(const char* value);

int setContextParam(const char* value);

int start();

int stop();

int cancel();

int sendAudio(char* data, int dataSize, bool encoded = false);

bool isStarted();

int getRecognizerResult(std::queue<NlsEvent>* eventQueue);

protected:
INlsRequestListener* _listener;
INlsRequestParam* _requestParam;
NlsSessionBase* _session;

private:
bool _isStarted;
pthread_mutex_t _mtxStarted;

void setStarted(bool value);

};

}

#if defined (_WIN32)
#pragma warning( pop )
#endif

#endif //NLS_SDK_SPEECH_REQUEST_H
