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

#include "nlsGlobal.h"
#include "nlog.h"
#include "utility.h"
#include "connectNode.h"
#include "iNlsRequestListener.h"
#include "speechTranscriberRequest.h"
#include "speechTranscriberParam.h"
#include "speechTranscriberListener.h"

namespace AlibabaNls {

SpeechTranscriberCallback::SpeechTranscriberCallback() {
  this->_onTaskFailed = NULL;
  this->_onTranscriptionStarted = NULL;
  this->_onSentenceBegin = NULL;
  this->_onTranscriptionResultChanged = NULL;
  this->_onSentenceEnd = NULL;
  this->_onTranscriptionCompleted = NULL;
  this->_onChannelClosed = NULL;
}

SpeechTranscriberCallback::~SpeechTranscriberCallback() {
  this->_onTaskFailed = NULL;
  this->_onTranscriptionStarted = NULL;
  this->_onSentenceBegin = NULL;
  this->_onTranscriptionResultChanged = NULL;
  this->_onSentenceEnd = NULL;
  this->_onTranscriptionCompleted = NULL;
  this->_onChannelClosed = NULL;

  std::map<NlsEvent::EventType, void*>::iterator iter;
  for (iter = _paramap.begin(); iter != _paramap.end();) {
    _paramap.erase(iter++);
  }
  _paramap.clear();
}

void SpeechTranscriberCallback::setOnTaskFailed(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnTaskFailed callback");
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void SpeechTranscriberCallback::setOnTranscriptionStarted(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnTranscriptionStarted callback");
  this->_onTranscriptionStarted = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionStarted) != _paramap.end()) {
    _paramap[NlsEvent::TranscriptionStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionStarted, para));
  }
}

void SpeechTranscriberCallback::setOnSentenceBegin(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnSentenceBegin callback");
  this->_onSentenceBegin = _event;
  if (this->_paramap.find(NlsEvent::SentenceBegin) != _paramap.end()) {
    _paramap[NlsEvent::SentenceBegin] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceBegin, para));
  }
}

void SpeechTranscriberCallback::setOnTranscriptionResultChanged(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnTranscriptionResultChanged callback");
  this->_onTranscriptionResultChanged = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionResultChanged) != _paramap.end()) {
    _paramap[NlsEvent::TranscriptionResultChanged] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionResultChanged, para));
  }
}

void SpeechTranscriberCallback::setOnSentenceEnd(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnSentenceEnd callback");
  this->_onSentenceEnd = _event;
  if (this->_paramap.find(NlsEvent::SentenceEnd) != _paramap.end()) {
    _paramap[NlsEvent::SentenceEnd] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceEnd, para));
  }
}

void SpeechTranscriberCallback::setOnSentenceSemantics(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnSentenceSemantics callback");
  this->_onSentenceSemantics = _event;
  if (this->_paramap.find(NlsEvent::SentenceSemantics) != _paramap.end()) {
    _paramap[NlsEvent::SentenceSemantics] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SentenceSemantics, para));
  }
}

void SpeechTranscriberCallback::setOnMessage(
    NlsCallbackMethod _event, void* para) {
  this->_onMessage = _event;
  if (this->_paramap.find(NlsEvent::Message) != _paramap.end()) {
    _paramap[NlsEvent::Message] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Message, para));
  }
}

void SpeechTranscriberCallback::setOnTranscriptionCompleted(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnTranscriptionCompleted callback");
  this->_onTranscriptionCompleted = _event;
  if (this->_paramap.find(NlsEvent::TranscriptionCompleted) != _paramap.end()) {
    _paramap[NlsEvent::TranscriptionCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TranscriptionCompleted, para));
  }
}

void SpeechTranscriberCallback::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnChannelClosed callback");
  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

SpeechTranscriberRequest::SpeechTranscriberRequest(
    const char* sdkName, bool isLongConnection) {
  _callback = new SpeechTranscriberCallback();

  //init request param
  _transcriberParam = new SpeechTranscriberParam(sdkName);
  _requestParam = _transcriberParam;

  //init listener
  _listener = new SpeechTranscriberListener(_callback);

  //init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_DEBUG("Request(%p) create SpeechTranscriberRequest Done.", this);
}

SpeechTranscriberRequest::~SpeechTranscriberRequest() {
  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _transcriberParam;
  _transcriberParam = NULL;

  LOG_DEBUG("Request(%p) destroy SpeechTranscriberRequest Done.", this);
}

int SpeechTranscriberRequest::start() {
  return INlsRequest::start(this);
}

int SpeechTranscriberRequest::control(const char* message) {
  return INlsRequest::stControl(this, message);
}

int SpeechTranscriberRequest::stop() {
  return INlsRequest::stop(this);
}

int SpeechTranscriberRequest::cancel() {
  return INlsRequest::cancel(this);
}

int SpeechTranscriberRequest::sendAudio(
    const uint8_t * data, size_t dataSize, ENCODER_TYPE type) {
  return INlsRequest::sendAudio(this, data, dataSize, type);
}

int SpeechTranscriberRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _transcriberParam->setPayloadParam(value);
}

int SpeechTranscriberRequest::setContextParam(const char *value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _transcriberParam->setContextParam(value);
}

int SpeechTranscriberRequest::setToken(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _transcriberParam->setToken(value);
  return Success;
}

int SpeechTranscriberRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _transcriberParam->setUrl(value);
  return Success;
}

int SpeechTranscriberRequest::setAppKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _transcriberParam->setAppKey(value);
  return Success;
}

int SpeechTranscriberRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _transcriberParam->setFormat(value);
  return Success;
}

int SpeechTranscriberRequest::setSampleRate(int value) {
  _transcriberParam->setSampleRate(value);
  return Success;
}

int SpeechTranscriberRequest::setIntermediateResult(bool value) {
  _transcriberParam->setIntermediateResult(value);
  return Success;
}

int SpeechTranscriberRequest::setPunctuationPrediction(bool value) {
  _transcriberParam->setPunctuationPrediction(value);
  return Success;
}

int SpeechTranscriberRequest::setInverseTextNormalization(bool value) {
  _transcriberParam->setTextNormalization(value);
  return Success;
}

int SpeechTranscriberRequest::AppendHttpHeaderParam(
    const char* key, const char* value) {
  return _transcriberParam->AppendHttpHeader(key, value);
}

int SpeechTranscriberRequest::setSemanticSentenceDetection(bool value) {
  _transcriberParam->setSentenceDetection(value);
  return Success;
}

int SpeechTranscriberRequest::setMaxSentenceSilence(int value) {
  return _transcriberParam->setMaxSentenceSilence(value);
}

int SpeechTranscriberRequest::setCustomizationId(const char * value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _transcriberParam->setCustomizationId(value);
}

int SpeechTranscriberRequest::setVocabularyId(const char * value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _transcriberParam->setVocabularyId(value);
}

int SpeechTranscriberRequest::setTimeout(int value) {
  _transcriberParam->setTimeout(value);
  return Success;
}

int SpeechTranscriberRequest::setEnableRecvTimeout(bool value) {
  _transcriberParam->setEnableRecvTimeout(value);
  return Success;
}

int SpeechTranscriberRequest::setRecvTimeout(int value) {
  _transcriberParam->setRecvTimeout(value);
  return Success;
}

int SpeechTranscriberRequest::setSendTimeout(int value) {
  _transcriberParam->setSendTimeout(value);
  return Success;
}

int SpeechTranscriberRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _transcriberParam->setOutputFormat(value);
  return Success;
}

int SpeechTranscriberRequest::setEnableOnMessage(bool value) {
  _transcriberParam->setEnableOnMessage(value);
  return Success;
}

const char* SpeechTranscriberRequest::getOutputFormat() {
  return _transcriberParam->getOutputFormat().c_str();
}

int SpeechTranscriberRequest::setNlpModel(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _transcriberParam->setNlpModel(value);
}

int SpeechTranscriberRequest::setEnableNlp(bool enable) {
  return _transcriberParam->setEnableNlp(enable);
}

int SpeechTranscriberRequest::setSessionId(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _transcriberParam->setSessionId(value);
}

int SpeechTranscriberRequest::setEnableWords(bool enable) {
  return _transcriberParam->setEnableWords(enable);
}

int SpeechTranscriberRequest::setEnableIgnoreSentenceTimeout(bool enable) {
  return _transcriberParam->setEnableIgnoreSentenceTimeout(enable);
}

int SpeechTranscriberRequest::setDisfluency(bool enable) {
  return _transcriberParam->setDisfluency(enable);
}

int SpeechTranscriberRequest::setSpeechNoiseThreshold(float value) {
  return _transcriberParam->setSpeechNoiseThreshold(value);
}

void SpeechTranscriberRequest::setOnTaskFailed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void SpeechTranscriberRequest::setOnTranscriptionStarted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionStarted(_event, para);
}

void SpeechTranscriberRequest::setOnSentenceBegin(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceBegin(_event, para);
}

void SpeechTranscriberRequest::setOnTranscriptionResultChanged(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionResultChanged(_event, para);
}

void SpeechTranscriberRequest::setOnSentenceEnd(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceEnd(_event, para);
}

void SpeechTranscriberRequest::setOnTranscriptionCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTranscriptionCompleted(_event, para);
}

void SpeechTranscriberRequest::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void SpeechTranscriberRequest::setOnSentenceSemantics(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSentenceSemantics(_event, para);
}

void SpeechTranscriberRequest::setOnMessage(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnMessage(_event, para);
}

}
