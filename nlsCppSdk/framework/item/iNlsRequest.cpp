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

#include "iNlsRequest.h"

#include <string>

#include "connectNode.h"
#include "iNlsRequestListener.h"
#include "iNlsRequestParam.h"
#include "nlog.h"
#include "nlsClient.h"
#include "nlsEventNetWork.h"
#include "nlsGlobal.h"
#include "nlsRequestParamInfo.h"
#include "utility.h"

namespace AlibabaNls {

INlsRequest::INlsRequest(const char* sdkName)
    : _node(NULL), _listener(NULL), _requestParam(NULL), _threadNumber(-1) {}

INlsRequest::~INlsRequest() {
  _node = NULL;
  _requestParam = NULL;
}

int INlsRequest::start(INlsRequest* request) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  LOG_INFO("Request(%p) invoke start ...", request);
  int ret = NlsEventNetWork::_eventClient->start(request);
  LOG_INFO("Request(%p) invoke start done, ret:%d.", request, ret);
  return ret;
}

int INlsRequest::stop(INlsRequest* request) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  LOG_INFO("Request(%p) invoke stop ...", request);
  int ret = NlsEventNetWork::_eventClient->stop(request);
  LOG_INFO("Request(%p) invoke stop done, ret:%d.", request, ret);
  return ret;
}

int INlsRequest::cancel(INlsRequest* request) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  LOG_INFO("Request(%p) invoke cancel ...", request);
  int ret = NlsEventNetWork::_eventClient->cancel(request);
  LOG_INFO("Request(%p) invoke cancel done, ret:%d.", request, ret);
  return ret;
}

int INlsRequest::stControl(INlsRequest* request, const char* message) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  return NlsEventNetWork::_eventClient->stControl(request, message);
}

int INlsRequest::sendAudio(INlsRequest* request, const uint8_t* data,
                           size_t dataSize, ENCODER_TYPE type) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  if (data == NULL || dataSize == 0) {
    LOG_ERROR("Input data is empty.");
    return -(InvalidRequestParams);
  }

  int ret =
      NlsEventNetWork::_eventClient->sendAudio(request, data, dataSize, type);

#ifdef ENABLE_CONTINUED
  if (request->getConnectNode() &&
      request->getConnectNode()->_reconnection.state !=
          NodeReconnection::NoReconnection &&
      ret < 0) {
    LOG_WARN("Request(%p) is reconnecting, ignore(%d) this error(%d) ...",
             request, request->getConnectNode()->_reconnection.state, ret);
    ret = dataSize;
  }
#endif

  return ret;
}

int INlsRequest::sendText(INlsRequest* request, const char* text) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  return NlsEventNetWork::_eventClient->sendText(request, text);
}

int INlsRequest::sendPing(INlsRequest* request) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  return NlsEventNetWork::_eventClient->sendPing(request);
}

int INlsRequest::sendFlush(INlsRequest* request, const char* parameters) {
  INPUT_REQUEST_CHECK(request);
  EVENT_CLIENT_CHECK(NlsEventNetWork::_eventClient);

  return NlsEventNetWork::_eventClient->sendFlush(request, parameters);
}

const char* INlsRequest::dumpAllInfo(INlsRequest* request) {
  if (request == NULL) {
    LOG_ERROR("Input request is empty.");
    return NULL;
  }
  if (NlsEventNetWork::_eventClient == NULL) {
    LOG_ERROR(
        "NlsEventNetWork has destroyed, please invoke startWorkThread() "
        "first.");
    return NULL;
  }

  return NlsEventNetWork::_eventClient->dumpAllInfo(request);
}

ConnectNode* INlsRequest::getConnectNode() {
  if (_node == NULL) {
    LOG_WARN("request(%p) _node is nullptr.", this);
    return NULL;
  }
  return _node;
}

INlsRequestParam* INlsRequest::getRequestParam() {
  if (_requestParam == NULL) {
    LOG_WARN("Request(%p) _requestParam is nullptr.", this);
    return NULL;
  }
  return _requestParam;
}

NlsRequestStatus INlsRequest::getRequestStatus(INlsRequest* request) {
  ConnectStatus node_status = request->getConnectNode()->getConnectNodeStatus();
  NlsRequestStatus status = RequestStatusInvalid;
  switch (node_status) {
    case NodeInvalid:
      status = RequestStatusInvalid;
      break;
    case NodeCreated:
      status = RequestStatusCreated;
      break;
    case NodeInvoking:
      status = RequestStatusInvoking;
      break;
    case NodeInvoked:
      status = RequestStatusInvoked;
      break;
    case NodeConnecting:
      status = RequestStatusConnecting;
      break;
    case NodeConnected:
      status = RequestStatusConnected;
      break;
    case NodeHandshaking:
      status = RequestStatusHandshaking;
      break;
    case NodeHandshaked:
      status = RequestStatusHandshaked;
      break;
    case NodeStarting:
      status = RequestStatusStarting;
      break;
    case NodeStarted:
      status = RequestStatusRunning;
      break;
    case NodeFailed:
      status = RequestStatusFailed;
      break;
    case NodeCompleted:
      status = RequestStatusCompleted;
      break;
    case NodeClosed:
      if (request->getConnectNode()->isLongConnection() &&
          !request->getConnectNode()->isConnected()) {
        status = RequestStatusDisconnected;
      } else {
        status = RequestStatusClosed;
      }
      break;
    case NodeReleased:
      status = RequestStatusReleased;
      break;
    case NodeStop:
      status = RequestStatusStop;
      break;
    case NodeCancel:
      status = RequestStatusCancel;
      break;
    default:
      LOG_ERROR("request(%p) node status is invalid %d.", request, node_status);
      break;
  }
  LOG_DEBUG("request(%p) status is %d and node status is %d", request, status,
            node_status);
  return status;
}

void INlsRequest::setThreadNumber(int num) { _threadNumber = num; }

int INlsRequest::getThreadNumber() { return _threadNumber; }

}  // namespace AlibabaNls
