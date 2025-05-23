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

#include "speechRecognizerRequest.h"

#include "connectNode.h"
#include "iNlsRequestListener.h"
#include "nlog.h"
#include "speechRecognizerListener.h"
#include "speechRecognizerParam.h"
#include "utility.h"

namespace AlibabaNls {

SpeechRecognizerCallback::SpeechRecognizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onRecognitionStarted = NULL;
  this->_onRecognitionCompleted = NULL;
  this->_onRecognitionResultChanged = NULL;
  this->_onChannelClosed = NULL;
  this->_onMessage = NULL;
}

SpeechRecognizerCallback::~SpeechRecognizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onRecognitionStarted = NULL;
  this->_onRecognitionCompleted = NULL;
  this->_onRecognitionResultChanged = NULL;
  this->_onChannelClosed = NULL;
  this->_onMessage = NULL;

  std::map<NlsEvent::EventType, void*>::iterator iter;
  for (iter = _paramap.begin(); iter != _paramap.end();) {
    _paramap.erase(iter++);
  }
  _paramap.clear();
}

void SpeechRecognizerCallback::setOnTaskFailed(NlsCallbackMethod event,
                                               void* param) {
  this->_onTaskFailed = event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = param;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, param));
  }
}

void SpeechRecognizerCallback::setOnRecognitionStarted(NlsCallbackMethod event,
                                                       void* param) {
  this->_onRecognitionStarted = event;
  if (this->_paramap.find(NlsEvent::RecognitionStarted) != _paramap.end()) {
    _paramap[NlsEvent::RecognitionStarted] = param;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::RecognitionStarted, param));
  }
}

void SpeechRecognizerCallback::setOnMessage(NlsCallbackMethod event,
                                            void* param) {
  this->_onMessage = event;
  if (this->_paramap.find(NlsEvent::Message) != _paramap.end()) {
    _paramap[NlsEvent::Message] = param;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Message, param));
  }
}

void SpeechRecognizerCallback::setOnRecognitionCompleted(
    NlsCallbackMethod event, void* param) {
  this->_onRecognitionCompleted = event;
  if (this->_paramap.find(NlsEvent::RecognitionCompleted) != _paramap.end()) {
    _paramap[NlsEvent::RecognitionCompleted] = param;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::RecognitionCompleted, param));
  }
}

void SpeechRecognizerCallback::setOnRecognitionResultChanged(
    NlsCallbackMethod event, void* param) {
  this->_onRecognitionResultChanged = event;
  if (this->_paramap.find(NlsEvent::RecognitionResultChanged) !=
      _paramap.end()) {
    _paramap[NlsEvent::RecognitionResultChanged] = param;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::RecognitionResultChanged, param));
  }
}

void SpeechRecognizerCallback::setOnChannelClosed(NlsCallbackMethod event,
                                                  void* param) {
  this->_onChannelClosed = event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = param;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, param));
  }
}

SpeechRecognizerRequest::SpeechRecognizerRequest(const char* sdkName,
                                                 bool isLongConnection) {
  _callback = new SpeechRecognizerCallback();

  // init request param
  _recognizerParam = new SpeechRecognizerParam(sdkName);
  _requestParam = _recognizerParam;

  // init listener
  _listener = new SpeechRecognizerListener(_callback);

  // init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_INFO("Request(%p) Node(%p) create SpeechRecognizerRequest Done.", this,
           _node);
}

SpeechRecognizerRequest::~SpeechRecognizerRequest() {
  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _recognizerParam;
  _recognizerParam = NULL;
  _requestParam = NULL;

  LOG_INFO("Request(%p) destroy SpeechRecognizerRequest Done.", this);
}

int SpeechRecognizerRequest::start() { return INlsRequest::start(this); }

int SpeechRecognizerRequest::stop() { return INlsRequest::stop(this); }

int SpeechRecognizerRequest::cancel() { return INlsRequest::cancel(this); }

int SpeechRecognizerRequest::sendAudio(const uint8_t* data, size_t dataSize,
                                       ENCODER_TYPE type) {
  return INlsRequest::sendAudio(this, data, dataSize, type);
}

const char* SpeechRecognizerRequest::dumpAllInfo() {
  return INlsRequest::dumpAllInfo(this);
}

int SpeechRecognizerRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  return _recognizerParam->setPayloadParam(value);
}

int SpeechRecognizerRequest::setContextParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  return _recognizerParam->setContextParam(value);
}

int SpeechRecognizerRequest::setToken(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setToken(value);
  return Success;
}

int SpeechRecognizerRequest::setTokenExpirationTime(uint64_t value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setTokenExpirationTime(value);
  return 0;
}

int SpeechRecognizerRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setUrl(value);
  return Success;
}

int SpeechRecognizerRequest::setAppKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setAppKey(value);
  return Success;
}

int SpeechRecognizerRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setFormat(value);
  return Success;
}

int SpeechRecognizerRequest::setSampleRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setSampleRate(value);
  return Success;
}

int SpeechRecognizerRequest::setIntermediateResult(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setIntermediateResult(value);
  return Success;
}

int SpeechRecognizerRequest::setPunctuationPrediction(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setPunctuationPrediction(value);
  return Success;
}

int SpeechRecognizerRequest::setInverseTextNormalization(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setTextNormalization(value);
  return Success;
}

int SpeechRecognizerRequest::setEnableVoiceDetection(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setEnableVoiceDetection(value);
  return Success;
}

int SpeechRecognizerRequest::setMaxStartSilence(int value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setMaxStartSilence(value);
  return Success;
}

int SpeechRecognizerRequest::setMaxEndSilence(int value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setMaxEndSilence(value);
  return Success;
}

int SpeechRecognizerRequest::AppendHttpHeaderParam(const char* key,
                                                   const char* value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  return _recognizerParam->AppendHttpHeader(key, value);
}

int SpeechRecognizerRequest::setCustomizationId(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  return _recognizerParam->setCustomizationId(value);
}

int SpeechRecognizerRequest::setVocabularyId(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  return _recognizerParam->setVocabularyId(value);
}

int SpeechRecognizerRequest::setTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setTimeout(value);
  return Success;
}

int SpeechRecognizerRequest::setEnableRecvTimeout(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setEnableRecvTimeout(value);
  return Success;
}

int SpeechRecognizerRequest::setRecvTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setRecvTimeout(value);
  return Success;
}

int SpeechRecognizerRequest::setSendTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setSendTimeout(value);
  return Success;
}

int SpeechRecognizerRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setOutputFormat(value);
  return Success;
}

int SpeechRecognizerRequest::setEnableOnMessage(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setEnableOnMessage(value);
  return Success;
}

int SpeechRecognizerRequest::setEnableContinued(bool enable) {
#ifdef ENABLE_CONTINUED
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setEnableContinued(enable);
  return Success;
#else
  return -(InvalidRequest);
#endif
}

const char* SpeechRecognizerRequest::getOutputFormat() {
  if (_recognizerParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _recognizerParam->getOutputFormat().c_str();
}

const char* SpeechRecognizerRequest::getTaskId() {
  if (_recognizerParam == NULL) {
    LOG_ERROR("Input request param is empty.");
    return NULL;
  }
  return _recognizerParam->getTaskId().c_str();
}

int SpeechRecognizerRequest::setAudioAddress(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_recognizerParam);
  _recognizerParam->setAudioAddress(value);
  return Success;
}

/* set callback of SpeechRecognizerRequest */
void SpeechRecognizerRequest::setOnTaskFailed(NlsCallbackMethod event,
                                              void* param) {
  _callback->setOnTaskFailed(event, param);
}

void SpeechRecognizerRequest::setOnRecognitionStarted(NlsCallbackMethod event,
                                                      void* param) {
  _callback->setOnRecognitionStarted(event, param);
}

void SpeechRecognizerRequest::setOnRecognitionCompleted(NlsCallbackMethod event,
                                                        void* param) {
  _callback->setOnRecognitionCompleted(event, param);
}

void SpeechRecognizerRequest::setOnRecognitionResultChanged(
    NlsCallbackMethod event, void* param) {
  _callback->setOnRecognitionResultChanged(event, param);
}

void SpeechRecognizerRequest::setOnChannelClosed(NlsCallbackMethod event,
                                                 void* param) {
  _callback->setOnChannelClosed(event, param);
}

void SpeechRecognizerRequest::setOnMessage(NlsCallbackMethod event,
                                           void* param) {
  _callback->setOnMessage(event, param);
}

}  // namespace AlibabaNls
