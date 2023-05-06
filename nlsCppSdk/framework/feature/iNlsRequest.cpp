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

#include <string>
#include "log.h"
#include "eventNetWork.h"
#include "iNlsRequestParam.h"
#include "iNlsRequestListener.h"
#include "nlsRequestParamInfo.h"
#include "iNlsRequest.h"

namespace AlibabaNls {
using std::string;
using namespace utility;

INlsRequest::INlsRequest(){
}

INlsRequest::~INlsRequest() {
}

int INlsRequest::start(INlsRequest *request) {
    if (request == NULL) {
        LOG_ERROR("Input request is empty.");
        return -1;
    }

    return NlsEventClientNetWork::_eventClient->start(request);
}

int INlsRequest::stop(INlsRequest *request, int type) {
    if (request == NULL) {
        LOG_ERROR("Input request is empty.");
        return -1;
    }

    return NlsEventClientNetWork::_eventClient->stop(request, type);
}

int INlsRequest::stControl(INlsRequest *request, const char* message) {
    if (request == NULL) {
        LOG_ERROR("Input request is empty.");
        return -1;
    }

    return NlsEventClientNetWork::_eventClient->stControl(request, message);
}

int INlsRequest::sendAudio(INlsRequest *request, const uint8_t* data, size_t dataSize, bool encoded ) {

    if (request == NULL || data == NULL || dataSize <= 0) {
        LOG_ERROR("Input arg is empty.");
        return -1;
    }

#if defined(__ANDROID__)
    if (_requestParam->_format == "opu") {
        return NlsEventClientNetWork::_eventClient->sendAudio(request, data, dataSize, true);
    } else {
        return NlsEventClientNetWork::_eventClient->sendAudio(request, data, dataSize, false);
    }
#else
    if (_requestParam->_format == "opu" && encoded) {
        return NlsEventClientNetWork::_eventClient->sendAudio(request, data, dataSize, true);
    } else {
        return NlsEventClientNetWork::_eventClient->sendAudio(request, data, dataSize, false);
    }
#endif
}

ConnectNode* INlsRequest::getConnectNode() {
    return _node;
}

INlsRequestParam* INlsRequest::getRequestParam() {
    return _requestParam;
}

}
