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
#include "nlsClient.h"
#include "iNlsRequest.h"
#include "iNlsRequestParam.h"
#include "iNlsRequestListener.h"
#include "nlsEventNetWork.h"
#include "nlsRequestParamInfo.h"
#include "connectNode.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

INlsRequest::INlsRequest(const char* sdkName) {
  _threadNumber = -1;
}

INlsRequest::~INlsRequest() {
  _node = NULL;
  _requestParam = NULL;
}

int INlsRequest::start(INlsRequest *request) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  LOG_DEBUG("Request(%p) invoke start ...", request);
  int ret = NlsEventNetWork::_eventClient->start(request);
  LOG_DEBUG("Request(%p) invoke start done, ret:%d.", request, ret);
  return ret;
}

int INlsRequest::stop(INlsRequest *request) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  LOG_DEBUG("Request(%p) invoke stop ...", request);
  int ret = NlsEventNetWork::_eventClient->stop(request);
  LOG_DEBUG("Request(%p) invoke stop done, ret:%d.", request, ret);
  return ret;
}

int INlsRequest::cancel(INlsRequest *request) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  LOG_DEBUG("Request(%p) invoke cancel ...", request);
  int ret = NlsEventNetWork::_eventClient->cancel(request);
  LOG_DEBUG("Request(%p) invoke cancel done, ret:%d.", request, ret);
  return ret;
}

int INlsRequest::stControl(INlsRequest *request, const char* message) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  return NlsEventNetWork::_eventClient->stControl(request, message);
}

int INlsRequest::sendAudio(INlsRequest *request, const uint8_t* data,
                           size_t dataSize, ENCODER_TYPE type) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  if (data == NULL || dataSize <= 0) {
    LOG_ERROR("Input data is empty.");
    return -(InvalidRequestParams);
  }

  int ret = NlsEventNetWork::_eventClient->sendAudio(
      request, data, dataSize, type);

  return ret;
}

ConnectNode* INlsRequest::getConnectNode() {
  if (_node == NULL) {
    LOG_WARN("_node is nullptr.");
    return NULL;
  }
  return _node;
}

INlsRequestParam* INlsRequest::getRequestParam() {
  if (_requestParam == NULL) {
    LOG_WARN("_requestParam is nullptr.");
    return NULL;
  }
  return _requestParam;
}

void INlsRequest::setThreadNumber(int num) {
  _threadNumber = num;
}

int INlsRequest::getThreadNumber() {
  return _threadNumber;
}

}  // namespace AlibabaNls
