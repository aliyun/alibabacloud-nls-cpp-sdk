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

#include "dashParaformerTranscriberRequest.h"

#include "connectNode.h"
#include "dashParaformerTranscriberListener.h"
#include "dashParaformerTranscriberParam.h"
#include "iNlsRequestListener.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

DashParaformerTranscriberCallback::DashParaformerTranscriberCallback() {
  this->_onTaskFailed = NULL;
  this->_onTranscriptionStarted = NULL;
  this->_onSentenceBegin = NULL;
  this->_onTranscriptionResultChanged = NULL;
  this->_onSentenceEnd = NULL;
  this->_onTranscriptionCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onMessage = NULL;
}

DashParaformerTranscriberCallback::~DashParaformerTranscriberCallback() {
  this->_onTaskFailed = NULL;
  this->_onTranscriptionStarted = NULL;
  this->_onSentenceBegin = NULL;
  this->_onTranscriptionResultChanged = NULL;
  this->_onSentenceEnd = NULL;
  this->_onTranscriptionCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onMessage = NULL;

  std::map<NlsEvent::EventType, void*>::iterator iter;
  for (iter = _paramap.begin(); iter != _paramap.end();) {
    _paramap.erase(iter++);
  }
  _paramap.clear();
}

void DashParaformerTranscriberCallback::setOnTaskFailed(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnTaskFailed callback");
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void DashParaformerTranscriberCallback::setOnTranscriptionStarted(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnTranscriptionStarted callback");
  this->_onTranscriptionStarted = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionStarted) != _paramap.end()) {
    _paramap[NlsEvent::TranscriptionStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionStarted, para));
  }
}

void DashParaformerTranscriberCallback::setOnSentenceBegin(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnSentenceBegin callback");
  this->_onSentenceBegin = _event;
  if (this->_paramap.find(NlsEvent::SentenceBegin) != _paramap.end()) {
    _paramap[NlsEvent::SentenceBegin] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceBegin, para));
  }
}

void DashParaformerTranscriberCallback::setOnTranscriptionResultChanged(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnTranscriptionResultChanged callback");
  this->_onTranscriptionResultChanged = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionResultChanged) !=
      _paramap.end()) {
    _paramap[NlsEvent::TranscriptionResultChanged] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionResultChanged, para));
  }
}

void DashParaformerTranscriberCallback::setOnSentenceEnd(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnSentenceEnd callback");
  this->_onSentenceEnd = _event;
  if (this->_paramap.find(NlsEvent::SentenceEnd) != _paramap.end()) {
    _paramap[NlsEvent::SentenceEnd] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceEnd, para));
  }
}

void DashParaformerTranscriberCallback::setOnSentenceSemantics(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnSentenceSemantics callback");
  this->_onSentenceSemantics = _event;
  if (this->_paramap.find(NlsEvent::SentenceSemantics) != _paramap.end()) {
    _paramap[NlsEvent::SentenceSemantics] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceSemantics, para));
  }
}

void DashParaformerTranscriberCallback::setOnMessage(NlsCallbackMethod _event,
                                                     void* para) {
  this->_onMessage = _event;
  if (this->_paramap.find(NlsEvent::Message) != _paramap.end()) {
    _paramap[NlsEvent::Message] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Message, para));
  }
}

void DashParaformerTranscriberCallback::setOnTranscriptionCompleted(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnTranscriptionCompleted callback");
  this->_onTranscriptionCompleted = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionCompleted) != _paramap.end()) {
    _paramap[NlsEvent::TranscriptionCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionCompleted, para));
  }
}

void DashParaformerTranscriberCallback::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnChannelClosed callback");
  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

DashParaformerTranscriberRequest::DashParaformerTranscriberRequest(
    const char* sdkName, bool isLongConnection) {
  _callback = new DashParaformerTranscriberCallback();

  // init request param
  _transcriberParam = new DashParaformerTranscriberParam(sdkName);
  _requestParam = _transcriberParam;

  // init listener
  _listener = new DashParaformerTranscriberListener(_callback);

  // init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_INFO("Request(%p) Node(%p) create DashParaformerTranscriberRequest Done.",
           this, _node);
}

DashParaformerTranscriberRequest::~DashParaformerTranscriberRequest() {
  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _transcriberParam;
  _transcriberParam = NULL;
  _requestParam = NULL;

  LOG_INFO("Request(%p) destroy DashParaformerTranscriberRequest Done.", this);
}

int DashParaformerTranscriberRequest::start() {
  return INlsRequest::start(this);
}

int DashParaformerTranscriberRequest::stop() { return INlsRequest::stop(this); }

int DashParaformerTranscriberRequest::cancel() {
  return INlsRequest::cancel(this);
}

int DashParaformerTranscriberRequest::sendAudio(const uint8_t* data,
                                                size_t dataSize,
                                                ENCODER_TYPE type) {
  return INlsRequest::sendAudio(this, data, dataSize, type);
}

const char* DashParaformerTranscriberRequest::dumpAllInfo() {
  return INlsRequest::dumpAllInfo(this);
}

NlsRequestStatus DashParaformerTranscriberRequest::getRequestStatus() {
  return INlsRequest::getRequestStatus(this);
}

int DashParaformerTranscriberRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  return _transcriberParam->setPayloadParam(value);
}

int DashParaformerTranscriberRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setUrl(value);
  return Success;
}

int DashParaformerTranscriberRequest::setAPIKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setAPIKey(value);
  return Success;
}

int DashParaformerTranscriberRequest::setTokenExpirationTime(uint64_t value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setTokenExpirationTime(value);
  return 0;
}

int DashParaformerTranscriberRequest::setModel(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setModel(value);
  return Success;
}

int DashParaformerTranscriberRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setFormat(value);
  return Success;
}

int DashParaformerTranscriberRequest::setSampleRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setSampleRate(value);
  return Success;
}

int DashParaformerTranscriberRequest::setVocabularyId(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  return _transcriberParam->setVocabularyId(value);
}

int DashParaformerTranscriberRequest::setDisfluencyRemovalEnabled(bool enable) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  return _transcriberParam->setDisfluencyRemovalEnabled(enable);
}

int DashParaformerTranscriberRequest::setLanguageHints(
    const char* json_array_str) {
  INPUT_PARAM_STRING_CHECK(json_array_str);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  return _transcriberParam->setLanguageHints(json_array_str);
}

int DashParaformerTranscriberRequest::setSemanticPunctuationEnabled(
    bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setSemanticPunctuationEnabled(value);
  return Success;
}

int DashParaformerTranscriberRequest::setMaxSentenceSilence(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setMaxSentenceSilence(value);
  return Success;
}

int DashParaformerTranscriberRequest::setMultiThresholdModeEnabled(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setMultiThresholdModeEnabled(value);
  return Success;
}

int DashParaformerTranscriberRequest::setPunctuationPrediction(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setPunctuationPrediction(value);
  return Success;
}

int DashParaformerTranscriberRequest::setHeartbeat(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setHeartbeat(value);
  return Success;
}

int DashParaformerTranscriberRequest::setInverseTextNormalization(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setTextNormalization(value);
  return Success;
}

int DashParaformerTranscriberRequest::setResources(const char* json_array_str) {
  INPUT_PARAM_STRING_CHECK(json_array_str);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  return _transcriberParam->setResources(json_array_str);
}

int DashParaformerTranscriberRequest::setTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setTimeout(value);
  return Success;
}

int DashParaformerTranscriberRequest::setEnableRecvTimeout(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setEnableRecvTimeout(value);
  return Success;
}

int DashParaformerTranscriberRequest::setRecvTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setRecvTimeout(value);
  return Success;
}

int DashParaformerTranscriberRequest::setSendTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setSendTimeout(value);
  return Success;
}

int DashParaformerTranscriberRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setOutputFormat(value);
  return Success;
}

int DashParaformerTranscriberRequest::setEnableOnMessage(bool enable) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setEnableOnMessage(enable);
  return Success;
}

int DashParaformerTranscriberRequest::setEnableContinued(bool enable) {
#ifdef ENABLE_CONTINUED
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setEnableContinued(enable);
  return Success;
#else
  return -(InvalidRequest);
#endif
}

const char* DashParaformerTranscriberRequest::getOutputFormat() {
  if (_transcriberParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _transcriberParam->getOutputFormat().c_str();
}

int DashParaformerTranscriberRequest::setTaskId(const char* taskId) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setTaskId(taskId);
  return Success;
}

const char* DashParaformerTranscriberRequest::getTaskId() {
  if (_transcriberParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _transcriberParam->getTaskId().c_str();
}

void DashParaformerTranscriberRequest::setOnTaskFailed(NlsCallbackMethod _event,
                                                       void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void DashParaformerTranscriberRequest::setOnTranscriptionStarted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionStarted(_event, para);
}

void DashParaformerTranscriberRequest::setOnSentenceBegin(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceBegin(_event, para);
}

void DashParaformerTranscriberRequest::setOnTranscriptionResultChanged(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionResultChanged(_event, para);
}

void DashParaformerTranscriberRequest::setOnSentenceEnd(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceEnd(_event, para);
}

void DashParaformerTranscriberRequest::setOnTranscriptionCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionCompleted(_event, para);
}

void DashParaformerTranscriberRequest::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void DashParaformerTranscriberRequest::setOnSentenceSemantics(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceSemantics(_event, para);
}

void DashParaformerTranscriberRequest::setOnMessage(NlsCallbackMethod _event,
                                                    void* para) {
  _callback->setOnMessage(_event, para);
}

}  // namespace AlibabaNls
