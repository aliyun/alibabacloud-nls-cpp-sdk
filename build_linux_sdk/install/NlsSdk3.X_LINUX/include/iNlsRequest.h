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
#include <string>
#include <stdint.h>
#include "nlsEvent.h"

#if defined(_WIN32)
#pragma warning( push )
#pragma warning ( disable : 4251 )
#endif

namespace AlibabaNls {

class ConnectNode;
class INlsRequestParam;
class INlsRequestListener;

class NLS_SDK_CLIENT_EXPORT INlsRequest {
public:

INlsRequest();
virtual~INlsRequest();

int start(INlsRequest*);
int stop(INlsRequest*, int);
int stControl(INlsRequest*, const char*);
int sendAudio(INlsRequest*, const uint8_t *, size_t, bool encoded = false);

ConnectNode* getConnectNode();
INlsRequestParam* getRequestParam();

protected:
ConnectNode* _node;
INlsRequestListener* _listener;
INlsRequestParam* _requestParam;

private:
};

}

#if defined (_WIN32)
#pragma warning( pop )
#endif

#endif //NLS_SDK_SPEECH_REQUEST_H
