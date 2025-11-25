/*
 * Copyright 2025 Alibaba Group Holding Limited
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

#include "dashCosyVoiceSynthesizerRequest.h"

#include "connectNode.h"
#include "dashCosyVoiceSynthesizerListener.h"
#include "dashCosyVoiceSynthesizerParam.h"
#include "iNlsRequestListener.h"
#include "nlog.h"
#include "text_utils.h"
#include "utility.h"

namespace AlibabaNls {

DashCosyVoiceSynthesizerCallback::DashCosyVoiceSynthesizerCallback() {
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

DashCosyVoiceSynthesizerCallback::~DashCosyVoiceSynthesizerCallback() {
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

void DashCosyVoiceSynthesizerCallback::setOnTaskFailed(NlsCallbackMethod _event,
                                                       void* para) {
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnSynthesisStarted(
    NlsCallbackMethod _event, void* para) {
  this->_onSynthesisStarted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisStarted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisStarted, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnSynthesisCompleted(
    NlsCallbackMethod _event, void* para) {
  this->_onSynthesisCompleted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisCompleted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisCompleted, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnBinaryDataReceived(
    NlsCallbackMethod _event, void* para) {
  this->_onBinaryDataReceived = _event;
  if (this->_paramap.find(NlsEvent::Binary) != _paramap.end()) {
    _paramap[NlsEvent::Binary] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Binary, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnSentenceBegin(
    NlsCallbackMethod _event, void* para) {
  this->_onSentenceBegin = _event;
  if (this->_paramap.find(NlsEvent::SentenceBegin) != _paramap.end()) {
    _paramap[NlsEvent::SentenceBegin] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceBegin, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnSentenceEnd(
    NlsCallbackMethod _event, void* para) {
  this->_onSentenceEnd = _event;
  if (this->_paramap.find(NlsEvent::SentenceEnd) != _paramap.end()) {
    _paramap[NlsEvent::SentenceEnd] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceEnd, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnSentenceSynthesis(
    NlsCallbackMethod _event, void* para) {
  this->_onSentenceSynthesis = _event;
  if (this->_paramap.find(NlsEvent::SentenceSynthesis) != _paramap.end()) {
    _paramap[NlsEvent::SentenceSynthesis] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceSynthesis, para));
  }
}

void DashCosyVoiceSynthesizerCallback::setOnMessage(NlsCallbackMethod _event,
                                                    void* para) {
  this->_onMessage = _event;
  if (this->_paramap.find(NlsEvent::Message) != _paramap.end()) {
    _paramap[NlsEvent::Message] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Message, para));
  }
}

DashCosyVoiceSynthesizerRequest::DashCosyVoiceSynthesizerRequest(
    const char* sdkName, bool isLongConnection) {
  _callback = new DashCosyVoiceSynthesizerCallback();

  // init request param
  _flowingSynthesizerParam = new DashCosyVoiceSynthesizerParam(sdkName);
  _requestParam = _flowingSynthesizerParam;

  // init listener
  _listener = new DashCosyVoiceSynthesizerListener(_callback);

  // init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_DEBUG(
      "Request(%p) Node(%p) create DashCosyVoiceSynthesizerRequest with long "
      "Connect flag(%d) Done.",
      this, _node, isLongConnection);
}

DashCosyVoiceSynthesizerRequest::~DashCosyVoiceSynthesizerRequest() {
  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _flowingSynthesizerParam;
  _flowingSynthesizerParam = NULL;
  _requestParam = NULL;

  LOG_INFO("Request(%p) destroy DashCosyVoiceSynthesizerRequest Done.", this);
}

int DashCosyVoiceSynthesizerRequest::start() {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setNlsRequestType(FlowingSynthesizer);
  return INlsRequest::start(this);
}

int DashCosyVoiceSynthesizerRequest::stop() { return INlsRequest::stop(this); }

int DashCosyVoiceSynthesizerRequest::cancel() {
  return INlsRequest::cancel(this);
}

int DashCosyVoiceSynthesizerRequest::sendText(const char* text) {
  int wordCount = utility::TextUtils::CharsCalculate(text);
  if (wordCount > _flowingSynthesizerParam->MaximumNumberOfWords) {
    return -(InvalidInputParam);
  }
  return INlsRequest::sendText(this, text);
}

const char* DashCosyVoiceSynthesizerRequest::dumpAllInfo() {
  return INlsRequest::dumpAllInfo(this);
}

NlsRequestStatus DashCosyVoiceSynthesizerRequest::getRequestStatus() {
  return INlsRequest::getRequestStatus(this);
}

int DashCosyVoiceSynthesizerRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setPayloadParam(value);
}

int DashCosyVoiceSynthesizerRequest::setContextParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setContextParam(value);
}

int DashCosyVoiceSynthesizerRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setUrl(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setAPIKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setAPIKey(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setTokenExpirationTime(uint64_t value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setTokenExpirationTime(value);
  return 0;
}

int DashCosyVoiceSynthesizerRequest::setModel(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setModel(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setVoice(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setVoice(value);
}

int DashCosyVoiceSynthesizerRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setFormat(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setSampleRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setSampleRate(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setVolume(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setVolume(value);
}

int DashCosyVoiceSynthesizerRequest::setSpeechRate(float rate) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setSpeechRate(rate);
}

int DashCosyVoiceSynthesizerRequest::setPitchRate(float pitch) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setPitchRate(pitch);
}

void DashCosyVoiceSynthesizerRequest::setSsmlEnabled(bool enable) {
  _flowingSynthesizerParam->setSsmlEnabled(enable);
}

int DashCosyVoiceSynthesizerRequest::setBitRate(int rate) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setBitRate(rate);
}

int DashCosyVoiceSynthesizerRequest::setWordTimestampEnabled(bool enable) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setWordTimestampEnabled(enable);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setSeed(int seed) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setSeed(seed);
}

int DashCosyVoiceSynthesizerRequest::setLanguageHints(
    const char* jsonArrayStr) {
  INPUT_PARAM_STRING_CHECK(jsonArrayStr);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setLanguageHints(jsonArrayStr);
}

int DashCosyVoiceSynthesizerRequest::setInstruction(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setInstruction(value);
}

int DashCosyVoiceSynthesizerRequest::setSingleRoundText(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  return _flowingSynthesizerParam->setSingleRoundText(value);
}

int DashCosyVoiceSynthesizerRequest::setTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setTimeout(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setRecvTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setRecvTimeout(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setSendTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setSendTimeout(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setOutputFormat(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setEnableOnMessage(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setEnableOnMessage(value);
  return Success;
}

int DashCosyVoiceSynthesizerRequest::setEnableContinued(bool enable) {
#ifdef ENABLE_CONTINUED
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setEnableContinued(enable);
  return Success;
#else
  return -(InvalidRequest);
#endif
}

const char* DashCosyVoiceSynthesizerRequest::getOutputFormat() {
  if (_flowingSynthesizerParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _flowingSynthesizerParam->getOutputFormat().c_str();
}

int DashCosyVoiceSynthesizerRequest::setTaskId(const char* taskId) {
  INPUT_REQUEST_PARAM_CHECK(_flowingSynthesizerParam);
  _flowingSynthesizerParam->setTaskId(taskId);
  return Success;
}

const char* DashCosyVoiceSynthesizerRequest::getTaskId() {
  if (_flowingSynthesizerParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _flowingSynthesizerParam->getTaskId().c_str();
}

void DashCosyVoiceSynthesizerRequest::setOnTaskFailed(NlsCallbackMethod _event,
                                                      void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnSynthesisStarted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSynthesisStarted(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnSynthesisCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSynthesisCompleted(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnBinaryDataReceived(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnBinaryDataReceived(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnSentenceBegin(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceBegin(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnSentenceEnd(NlsCallbackMethod _event,
                                                       void* para) {
  _callback->setOnSentenceEnd(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnSentenceSynthesis(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceSynthesis(_event, para);
}

void DashCosyVoiceSynthesizerRequest::setOnMessage(NlsCallbackMethod _event,
                                                   void* para) {
  _callback->setOnMessage(_event, para);
}

int DashCosyVoiceSynthesizerRequest::AppendHttpHeaderParam(const char* key,
                                                           const char* value) {
  return _flowingSynthesizerParam->AppendHttpHeader(key, value);
}

}  // namespace AlibabaNls
