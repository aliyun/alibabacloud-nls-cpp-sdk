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

#include "iNlsRequestListener.h"
#include "dialogAssistantRequest.h"
#include "dialogAssistantParam.h"
#include "dialogAssistantListener.h"
#include "connectNode.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

DialogAssistantCallback::DialogAssistantCallback() {
  this->_onTaskFailed = NULL;
  this->_onRecognitionStarted = NULL;
  this->_onRecognitionCompleted = NULL;
  this->_onDialogResultGenerated = NULL;
  this->_onRecognitionResultChanged = NULL;
  this->_onChannelClosed = NULL;
}

DialogAssistantCallback::~DialogAssistantCallback() {
  this->_onTaskFailed = NULL;
  this->_onRecognitionStarted = NULL;
  this->_onRecognitionCompleted = NULL;
  this->_onDialogResultGenerated = NULL;
  this->_onRecognitionResultChanged = NULL;
  this->_onChannelClosed = NULL;

  std::map<NlsEvent::EventType, void*>::iterator iter;
  for (iter = _paramap.begin(); iter != _paramap.end();) {
    _paramap.erase(iter++);
  }
  _paramap.clear();
}

void DialogAssistantCallback::setOnTaskFailed(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnTaskFailed callback");
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void DialogAssistantCallback::setOnRecognitionStarted(
    NlsCallbackMethod _event, void* para) {
  LOG_DEBUG("setOnRecognitionStarted callback");

  this->_onRecognitionStarted = _event;
  if (this->_paramap.find(NlsEvent::RecognitionStarted) != _paramap.end()) {
    _paramap[NlsEvent::RecognitionStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::RecognitionStarted, para));
  }
}

void DialogAssistantCallback::setOnRecognitionCompleted(
    NlsCallbackMethod _event, void* para) {
  LOG_DEBUG("setOnRecognitionCompleted callback");
  this->_onRecognitionCompleted = _event;
  if (this->_paramap.find(NlsEvent::RecognitionCompleted) != _paramap.end()) {
    _paramap[NlsEvent::RecognitionCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::RecognitionCompleted, para));
  }
}

void DialogAssistantCallback::setOnDialogResultGenerated(
    NlsCallbackMethod _event, void* para) {
  LOG_DEBUG("setOnDialogResultGenerated callback");
  this->_onDialogResultGenerated = _event;
  if (this->_paramap.find(NlsEvent::DialogResultGenerated) != _paramap.end()) {
    _paramap[NlsEvent::DialogResultGenerated] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::DialogResultGenerated, para));
  }
}

void DialogAssistantCallback::setOnWakeWordVerificationCompleted(
    NlsCallbackMethod _event, void* para) {
  LOG_DEBUG("setOnWakeWordVerificationCompleted callback");

  this->_onWakeWordVerificationCompleted = _event;
  if (this->_paramap.find(
        NlsEvent::WakeWordVerificationCompleted) != _paramap.end()) {
    _paramap[NlsEvent::WakeWordVerificationCompleted] = para;
  } else {
    _paramap.insert(
        std::make_pair(NlsEvent::WakeWordVerificationCompleted, para));
  }
}

void DialogAssistantCallback::setOnRecognitionResultChanged(
    NlsCallbackMethod _event, void* para) {
  LOG_DEBUG("setOnRecognitionResultChanged callback");
  this->_onRecognitionResultChanged = _event;
  if (this->_paramap.find(NlsEvent::RecognitionResultChanged) != _paramap.end()) {
    _paramap[NlsEvent::RecognitionResultChanged] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::RecognitionResultChanged, para));
  }
}

void DialogAssistantCallback::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  LOG_DEBUG("setOnChannelClosed callback");
  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

DialogAssistantRequest::DialogAssistantRequest(
    int version, const char* sdkName, bool isLongConnection) {
  _callback = new DialogAssistantCallback();

  //init request param
  _dialogAssistantParam = new DialogAssistantParam(version, sdkName);
  _requestParam = _dialogAssistantParam;

  //init listener
  _listener = new DialogAssistantListener(_callback);

  //init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_DEBUG("Create DialogAssistantRequest Done.");
}

DialogAssistantRequest::~DialogAssistantRequest() {
//  delete _dialogAssistantParam;
//  _dialogAssistantParam = NULL;

  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _dialogAssistantParam;
  _dialogAssistantParam = NULL;
  LOG_DEBUG("Request:%p Destroy DialogAssistantRequest.", this);
}

int DialogAssistantRequest::start() {
  if (_dialogAssistantParam->_enableWakeWord) {
    _requestParam->setNlsRequestType(SpeechWakeWordDialog);
  } else {
    _requestParam->setNlsRequestType(SpeechExecuteDialog);
  }
  return INlsRequest::start(this);
}

int DialogAssistantRequest::stop() {
  return INlsRequest::stop(this);
}

int DialogAssistantRequest::cancel() {
  return INlsRequest::cancel(this);
}

int DialogAssistantRequest::StopWakeWordVerification() {
  // return INlsRequest::stop(this, 2);
  return Success;
}

int DialogAssistantRequest::queryText() {
  _requestParam->setNlsRequestType(SpeechTextDialog);
  return INlsRequest::start(this);
}

int DialogAssistantRequest::sendAudio(
    const uint8_t * data, size_t dataSize, ENCODER_TYPE type) {
  return INlsRequest::sendAudio(this, data, dataSize, type);
}

int DialogAssistantRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _dialogAssistantParam->setPayloadParam(value);
}

int DialogAssistantRequest::setContextParam(const char *value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _dialogAssistantParam->setContextParam(value);
}

int DialogAssistantRequest::setToken(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _dialogAssistantParam->setToken(value);
  return 0;
}

int DialogAssistantRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _dialogAssistantParam->setUrl(value);
  return 0;
}

int DialogAssistantRequest::setAppKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _dialogAssistantParam->setAppKey(value);
  return 0;
}

int DialogAssistantRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _dialogAssistantParam->setFormat(value);
  return 0;
}

int DialogAssistantRequest::setSampleRate(int value) {
  _dialogAssistantParam->setSampleRate(value);
  return 0;
}

int DialogAssistantRequest::setTimeout(int value) {
  _dialogAssistantParam->setTimeout(value);
  return 0;
}

int DialogAssistantRequest::setRecvTimeout(int value) {
  _dialogAssistantParam->setRecvTimeout(value);
  return 0;
}

int DialogAssistantRequest::setSendTimeout(int value) {
  _dialogAssistantParam->setSendTimeout(value);
  return 0;
}

int DialogAssistantRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _dialogAssistantParam->setOutputFormat(value);
  return 0;
}

int DialogAssistantRequest::setSessionId(const char* sessionId) {
  return _dialogAssistantParam->setSessionId(sessionId);
}

int DialogAssistantRequest::setQueryContext(const char* value) {
  return _dialogAssistantParam->setQueryContext(value);
}

int DialogAssistantRequest::setQuery(const char* value) {
  return _dialogAssistantParam->setQuery(value);
}

int DialogAssistantRequest::setQueryParams(const char* value) {
  return _dialogAssistantParam->setQueryParams(value);
}

int DialogAssistantRequest::AppendHttpHeaderParam(
    const char* key, const char* value) {
  return _dialogAssistantParam->AppendHttpHeader(key, value);
}

int DialogAssistantRequest::setWakeWordModel(const char* value) {
  return _dialogAssistantParam->setWakeWordModel(value);
}

int DialogAssistantRequest::setWakeWord(const char* value) {
  return _dialogAssistantParam->setWakeWord(value);
}

int DialogAssistantRequest::setEnableWakeWordVerification(bool value) {
  return _dialogAssistantParam->setEnableWakeWordVerification(value);
}

void DialogAssistantRequest::setOnTaskFailed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void DialogAssistantRequest::setOnRecognitionStarted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnRecognitionStarted(_event, para);
}

void DialogAssistantRequest::setOnRecognitionCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnRecognitionCompleted(_event, para);
}

void DialogAssistantRequest::setOnDialogResultGenerated(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnDialogResultGenerated(_event, para);
}

void DialogAssistantRequest::setOnWakeWordVerificationCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnWakeWordVerificationCompleted(_event, para);
}

void DialogAssistantRequest::setOnRecognitionResultChanged(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnRecognitionResultChanged(_event, para);
}

void DialogAssistantRequest::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void DialogAssistantRequest::setEnableMultiGroup(bool value) {
  _dialogAssistantParam->setEnableMultiGroup(value);
}

}
