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

#include "flowingSynthesizerRequest.h"

#include "connectNode.h"
#include "flowingSynthesizerListener.h"
#include "flowingSynthesizerParam.h"
#include "iNlsRequestListener.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

FlowingSynthesizerCallback::FlowingSynthesizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onSynthesisStarted = NULL;
  this->_onSynthesisCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onBinaryDataReceived = NULL;
  this->_onSentenceBegin = NULL;
  this->_onSentenceEnd = NULL;
  this->_onSentenceSynthesis = NULL;
  this->_onMessage = NULL;
}

FlowingSynthesizerCallback::~FlowingSynthesizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onSynthesisStarted = NULL;
  this->_onSynthesisCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onBinaryDataReceived = NULL;
  this->_onSentenceBegin = NULL;
  this->_onSentenceEnd = NULL;
  this->_onSentenceSynthesis = NULL;
  this->_onMessage = NULL;

  std::map<NlsEvent::EventType, void*>::iterator iter;
  for (iter = _paramap.begin(); iter != _paramap.end();) {
    _paramap.erase(iter++);
  }
  _paramap.clear();
}

void FlowingSynthesizerCallback::setOnTaskFailed(NlsCallbackMethod _event,
                                                 void* para) {
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void FlowingSynthesizerCallback::setOnSynthesisStarted(NlsCallbackMethod _event,
                                                       void* para) {
  this->_onSynthesisStarted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisStarted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisStarted, para));
  }
}

void FlowingSynthesizerCallback::setOnSynthesisCompleted(
    NlsCallbackMethod _event, void* para) {
  this->_onSynthesisCompleted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisCompleted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisCompleted, para));
  }
}

void FlowingSynthesizerCallback::setOnChannelClosed(NlsCallbackMethod _event,
                                                    void* para) {
  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

void FlowingSynthesizerCallback::setOnBinaryDataReceived(
    NlsCallbackMethod _event, void* para) {
  this->_onBinaryDataReceived = _event;
  if (this->_paramap.find(NlsEvent::Binary) != _paramap.end()) {
    _paramap[NlsEvent::Binary] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Binary, para));
  }
}

void FlowingSynthesizerCallback::setOnSentenceBegin(NlsCallbackMethod _event,
                                                    void* para) {
  this->_onSentenceBegin = _event;
  if (this->_paramap.find(NlsEvent::SentenceBegin) != _paramap.end()) {
    _paramap[NlsEvent::SentenceBegin] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceBegin, para));
  }
}

void FlowingSynthesizerCallback::setOnSentenceEnd(NlsCallbackMethod _event,
                                                  void* para) {
  this->_onSentenceEnd = _event;
  if (this->_paramap.find(NlsEvent::SentenceEnd) != _paramap.end()) {
    _paramap[NlsEvent::SentenceEnd] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceEnd, para));
  }
}

void FlowingSynthesizerCallback::setOnSentenceSynthesis(
    NlsCallbackMethod _event, void* para) {
  this->_onSentenceSynthesis = _event;
  if (this->_paramap.find(NlsEvent::SentenceSynthesis) != _paramap.end()) {
    _paramap[NlsEvent::SentenceSynthesis] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceSynthesis, para));
  }
}

void FlowingSynthesizerCallback::setOnMessage(NlsCallbackMethod _event,
                                              void* para) {
  this->_onMessage = _event;
  if (this->_paramap.find(NlsEvent::Message) != _paramap.end()) {
    _paramap[NlsEvent::Message] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Message, para));
  }
}

FlowingSynthesizerRequest::FlowingSynthesizerRequest(const char* sdkName,
                                                     bool isLongConnection) {
  _callback = new FlowingSynthesizerCallback();

  // init request param
  _flowingSynthesizerParam = new FlowingSynthesizerParam(sdkName);
  _requestParam = _flowingSynthesizerParam;

  // init listener
  _listener = new FlowingSynthesizerListener(_callback);

  // init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_DEBUG(
      "Request(%p) Node(%p) create FlowingSynthesizerRequest with long Connect "
      "flag(%d) "
      "Done.",
      this, _node, isLongConnection);
}

FlowingSynthesizerRequest::~FlowingSynthesizerRequest() {
  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _flowingSynthesizerParam;
  _flowingSynthesizerParam = NULL;
  _requestParam = NULL;

  LOG_INFO("Request(%p) destroy SpeechSynthesizerRequest Done.", this);
}

int FlowingSynthesizerRequest::start() {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setNlsRequestType(FlowingSynthesizer);
  return INlsRequest::start(this);
}

int FlowingSynthesizerRequest::stop() { return INlsRequest::stop(this); }

int FlowingSynthesizerRequest::cancel() { return INlsRequest::cancel(this); }

int FlowingSynthesizerRequest::sendText(const char* text) {
  return INlsRequest::sendText(this, text);
}

int FlowingSynthesizerRequest::sendFlush() {
  return INlsRequest::sendFlush(this);
}

const char* FlowingSynthesizerRequest::dumpAllInfo() {
  return INlsRequest::dumpAllInfo(this);
}

int FlowingSynthesizerRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setPayloadParam(value);
}

int FlowingSynthesizerRequest::setContextParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setContextParam(value);
}

int FlowingSynthesizerRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setUrl(value);
  return Success;
}

int FlowingSynthesizerRequest::setAppKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setAppKey(value);
  return Success;
}

int FlowingSynthesizerRequest::setToken(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setToken(value);
  return 0;
}

int FlowingSynthesizerRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setFormat(value);
  return Success;
}

int FlowingSynthesizerRequest::setSampleRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setSampleRate(value);
  return Success;
}

int FlowingSynthesizerRequest::setPitchRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setPitchRate(value);
}

int FlowingSynthesizerRequest::setEnableSubtitle(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setEnableSubtitle(value);
  return Success;
}

int FlowingSynthesizerRequest::setSpeechRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setSpeechRate(value);
}

int FlowingSynthesizerRequest::setVolume(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setVolume(value);
}

int FlowingSynthesizerRequest::setVoice(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setVoice(value);
}

int FlowingSynthesizerRequest::setTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setTimeout(value);
  return Success;
}

int FlowingSynthesizerRequest::setRecvTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setRecvTimeout(value);
  return Success;
}

int FlowingSynthesizerRequest::setSendTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setSendTimeout(value);
  return Success;
}

int FlowingSynthesizerRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setOutputFormat(value);
  return Success;
}

int FlowingSynthesizerRequest::setEnableOnMessage(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setEnableOnMessage(value);
  return Success;
}

int FlowingSynthesizerRequest::setEnableContinued(bool enable) {
#ifdef ENABLE_CONTINUED
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setEnableContinued(enable);
  return Success;
#else
  return -(InvalidRequest);
#endif
}

const char* FlowingSynthesizerRequest::getOutputFormat() {
  if (_flowingSynthesizerParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _flowingSynthesizerParam->getOutputFormat().c_str();
}

const char* FlowingSynthesizerRequest::getTaskId() {
  if (_flowingSynthesizerParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _flowingSynthesizerParam->getTaskId().c_str();
}

void FlowingSynthesizerRequest::setOnTaskFailed(NlsCallbackMethod _event,
                                                void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void FlowingSynthesizerRequest::setOnSynthesisStarted(NlsCallbackMethod _event,
                                                      void* para) {
  _callback->setOnSynthesisStarted(_event, para);
}

void FlowingSynthesizerRequest::setOnSynthesisCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSynthesisCompleted(_event, para);
}

void FlowingSynthesizerRequest::setOnChannelClosed(NlsCallbackMethod _event,
                                                   void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void FlowingSynthesizerRequest::setOnBinaryDataReceived(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnBinaryDataReceived(_event, para);
}

void FlowingSynthesizerRequest::setOnSentenceBegin(NlsCallbackMethod _event,
                                                   void* para) {
  _callback->setOnSentenceBegin(_event, para);
}

void FlowingSynthesizerRequest::setOnSentenceEnd(NlsCallbackMethod _event,
                                                 void* para) {
  _callback->setOnSentenceEnd(_event, para);
}

void FlowingSynthesizerRequest::setOnSentenceSynthesis(NlsCallbackMethod _event,
                                                       void* para) {
  _callback->setOnSentenceSynthesis(_event, para);
}

void FlowingSynthesizerRequest::setOnMessage(NlsCallbackMethod _event,
                                             void* para) {
  _callback->setOnMessage(_event, para);
}

int FlowingSynthesizerRequest::AppendHttpHeaderParam(const char* key,
                                                     const char* value) {
  return _flowingSynthesizerParam->AppendHttpHeader(key, value);
}

}  // namespace AlibabaNls
