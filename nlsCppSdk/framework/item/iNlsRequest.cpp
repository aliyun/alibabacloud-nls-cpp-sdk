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

#include <string>
#include "nlsGlobal.h"
#include "iNlsRequest.h"
#include "iNlsRequestParam.h"
#include "iNlsRequestListener.h"
#include "nlsEventNetWork.h"
#include "nlsRequestParamInfo.h"
#include "connectNode.h"
#include "nlog.h"

namespace AlibabaNls {

INlsRequest::INlsRequest(const char* sdkName) {
  _threadNumber = -1;
}

INlsRequest::~INlsRequest() {
  _node = NULL;
}

int INlsRequest::start(INlsRequest *request) {
  if (request == NULL) {
    LOG_ERROR("Input request is empty.");
    return -1;
  } else {
    LOG_DEBUG("request:%p start ->", request);
  }

  int ret = NlsEventNetWork::_eventClient->start(request);
  LOG_DEBUG("request:%p start done", request);
  return ret;
}

int INlsRequest::stop(INlsRequest *request, int type) {
  if (request == NULL) {
    LOG_ERROR("Input request is empty.");
    return -1;
  } else {
    LOG_DEBUG("request:%p stop ->", request);
  }

  int ret = NlsEventNetWork::_eventClient->stop(request, type);
  LOG_DEBUG("request:%p stop done", request);
  return ret;
}

int INlsRequest::stControl(INlsRequest *request, const char* message) {
  if (request == NULL) {
    LOG_ERROR("Input request is empty.");
    return -1;
  }

  return NlsEventNetWork::_eventClient->stControl(request, message);
}

int INlsRequest::sendAudio(INlsRequest *request, const uint8_t* data,
                           size_t dataSize, ENCODER_TYPE type) {
  if (request == NULL || data == NULL || dataSize <= 0) {
    LOG_ERROR("Input arg is empty.");
    return -1;
  }

//  LOG_DEBUG("Node:%p sendAudio type:%d begin, size(%d)",
//      request->getConnectNode(), type, dataSize);

  int ret = NlsEventNetWork::_eventClient->sendAudio(
      request, data, dataSize, type);

//  LOG_DEBUG("Node:%p sendAudio type:%d done, ret(%d).",
//      request->getConnectNode(), type, ret);
  return ret;
}

ConnectNode* INlsRequest::getConnectNode() {
  if (_node == NULL) {
    LOG_WARN("getConnectNode nullptr");
    return NULL;
  }
  return _node;
}

INlsRequestParam* INlsRequest::getRequestParam() {
  return _requestParam;
}

void INlsRequest::setThreadNumber(int num) {
  _threadNumber = num;
}

int INlsRequest::getThreadNumber() {
  return _threadNumber;
}

}  // namespace AlibabaNls
