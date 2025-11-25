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

#include "dashFunAsrTranscriberRequest.h"

#include "connectNode.h"
#include "dashFunAsrTranscriberListener.h"
#include "dashFunAsrTranscriberParam.h"
#include "iNlsRequestListener.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

DashFunAsrTranscriberCallback::DashFunAsrTranscriberCallback() {
  this->_onTaskFailed = NULL;
  this->_onTranscriptionStarted = NULL;
  this->_onSentenceBegin = NULL;
  this->_onTranscriptionResultChanged = NULL;
  this->_onSentenceEnd = NULL;
  this->_onTranscriptionCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onMessage = NULL;
}

DashFunAsrTranscriberCallback::~DashFunAsrTranscriberCallback() {
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

void DashFunAsrTranscriberCallback::setOnTaskFailed(NlsCallbackMethod _event,
                                                    void* para) {
  // LOG_DEBUG("setOnTaskFailed callback");
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void DashFunAsrTranscriberCallback::setOnTranscriptionStarted(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnTranscriptionStarted callback");
  this->_onTranscriptionStarted = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionStarted) != _paramap.end()) {
    _paramap[NlsEvent::TranscriptionStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionStarted, para));
  }
}

void DashFunAsrTranscriberCallback::setOnSentenceBegin(NlsCallbackMethod _event,
                                                       void* para) {
  // LOG_DEBUG("setOnSentenceBegin callback");
  this->_onSentenceBegin = _event;
  if (this->_paramap.find(NlsEvent::SentenceBegin) != _paramap.end()) {
    _paramap[NlsEvent::SentenceBegin] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceBegin, para));
  }
}

void DashFunAsrTranscriberCallback::setOnTranscriptionResultChanged(
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

void DashFunAsrTranscriberCallback::setOnSentenceEnd(NlsCallbackMethod _event,
                                                     void* para) {
  // LOG_DEBUG("setOnSentenceEnd callback");
  this->_onSentenceEnd = _event;
  if (this->_paramap.find(NlsEvent::SentenceEnd) != _paramap.end()) {
    _paramap[NlsEvent::SentenceEnd] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceEnd, para));
  }
}

void DashFunAsrTranscriberCallback::setOnSentenceSemantics(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnSentenceSemantics callback");
  this->_onSentenceSemantics = _event;
  if (this->_paramap.find(NlsEvent::SentenceSemantics) != _paramap.end()) {
    _paramap[NlsEvent::SentenceSemantics] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceSemantics, para));
  }
}

void DashFunAsrTranscriberCallback::setOnMessage(NlsCallbackMethod _event,
                                                 void* para) {
  this->_onMessage = _event;
  if (this->_paramap.find(NlsEvent::Message) != _paramap.end()) {
    _paramap[NlsEvent::Message] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Message, para));
  }
}

void DashFunAsrTranscriberCallback::setOnTranscriptionCompleted(
    NlsCallbackMethod _event, void* para) {
  // LOG_DEBUG("setOnTranscriptionCompleted callback");
  this->_onTranscriptionCompleted = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionCompleted) != _paramap.end()) {
    _paramap[NlsEvent::TranscriptionCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionCompleted, para));
  }
}

void DashFunAsrTranscriberCallback::setOnChannelClosed(NlsCallbackMethod _event,
                                                       void* para) {
  // LOG_DEBUG("setOnChannelClosed callback");
  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

DashFunAsrTranscriberRequest::DashFunAsrTranscriberRequest(
    const char* sdkName, bool isLongConnection) {
  _callback = new DashFunAsrTranscriberCallback();

  // init request param
  _transcriberParam = new DashFunAsrTranscriberParam(sdkName);
  _requestParam = _transcriberParam;

  // init listener
  _listener = new DashFunAsrTranscriberListener(_callback);

  // init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_INFO("Request(%p) Node(%p) create DashFunAsrTranscriberRequest Done.",
           this, _node);
}

DashFunAsrTranscriberRequest::~DashFunAsrTranscriberRequest() {
  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _transcriberParam;
  _transcriberParam = NULL;
  _requestParam = NULL;

  LOG_INFO("Request(%p) destroy DashFunAsrTranscriberRequest Done.", this);
}

int DashFunAsrTranscriberRequest::start() { return INlsRequest::start(this); }

int DashFunAsrTranscriberRequest::stop() { return INlsRequest::stop(this); }

int DashFunAsrTranscriberRequest::cancel() { return INlsRequest::cancel(this); }

int DashFunAsrTranscriberRequest::sendAudio(const uint8_t* data,
                                            size_t dataSize,
                                            ENCODER_TYPE type) {
  return INlsRequest::sendAudio(this, data, dataSize, type);
}

const char* DashFunAsrTranscriberRequest::dumpAllInfo() {
  return INlsRequest::dumpAllInfo(this);
}

NlsRequestStatus DashFunAsrTranscriberRequest::getRequestStatus() {
  return INlsRequest::getRequestStatus(this);
}

int DashFunAsrTranscriberRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  return _transcriberParam->setPayloadParam(value);
}

int DashFunAsrTranscriberRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setUrl(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setAPIKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setAPIKey(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setTokenExpirationTime(uint64_t value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setTokenExpirationTime(value);
  return 0;
}

int DashFunAsrTranscriberRequest::setModel(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setModel(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setFormat(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setSampleRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setSampleRate(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setVocabularyId(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  return _transcriberParam->setVocabularyId(value);
}

int DashFunAsrTranscriberRequest::setSemanticPunctuationEnabled(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setSemanticPunctuationEnabled(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setMaxSentenceSilence(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setMaxSentenceSilence(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setMultiThresholdModeEnabled(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setMultiThresholdModeEnabled(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setHeartbeat(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setHeartbeat(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setTimeout(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setEnableRecvTimeout(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setEnableRecvTimeout(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setRecvTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setRecvTimeout(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setSendTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setSendTimeout(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setOutputFormat(value);
  return Success;
}

int DashFunAsrTranscriberRequest::setEnableOnMessage(bool enable) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setEnableOnMessage(enable);
  return Success;
}

int DashFunAsrTranscriberRequest::setEnableContinued(bool enable) {
#ifdef ENABLE_CONTINUED
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setEnableContinued(enable);
  return Success;
#else
  return -(InvalidRequest);
#endif
}

const char* DashFunAsrTranscriberRequest::getOutputFormat() {
  if (_transcriberParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _transcriberParam->getOutputFormat().c_str();
}

int DashFunAsrTranscriberRequest::setTaskId(const char* taskId) {
  INPUT_REQUEST_PARAM_CHECK(_transcriberParam);
  _transcriberParam->setTaskId(taskId);
  return Success;
}

const char* DashFunAsrTranscriberRequest::getTaskId() {
  if (_transcriberParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _transcriberParam->getTaskId().c_str();
}

void DashFunAsrTranscriberRequest::setOnTaskFailed(NlsCallbackMethod _event,
                                                   void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void DashFunAsrTranscriberRequest::setOnTranscriptionStarted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionStarted(_event, para);
}

void DashFunAsrTranscriberRequest::setOnSentenceBegin(NlsCallbackMethod _event,
                                                      void* para) {
  _callback->setOnSentenceBegin(_event, para);
}

void DashFunAsrTranscriberRequest::setOnTranscriptionResultChanged(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionResultChanged(_event, para);
}

void DashFunAsrTranscriberRequest::setOnSentenceEnd(NlsCallbackMethod _event,
                                                    void* para) {
  _callback->setOnSentenceEnd(_event, para);
}

void DashFunAsrTranscriberRequest::setOnTranscriptionCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionCompleted(_event, para);
}

void DashFunAsrTranscriberRequest::setOnChannelClosed(NlsCallbackMethod _event,
                                                      void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void DashFunAsrTranscriberRequest::setOnSentenceSemantics(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceSemantics(_event, para);
}

void DashFunAsrTranscriberRequest::setOnMessage(NlsCallbackMethod _event,
                                                void* para) {
  _callback->setOnMessage(_event, para);
}

}  // namespace AlibabaNls
