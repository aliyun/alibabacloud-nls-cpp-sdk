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
#include "speechSynthesizerRequest.h"
#include "speechSynthesizerParam.h"
#include "speechSynthesizerListener.h"
#include "connectNode.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

SpeechSynthesizerCallback::SpeechSynthesizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onSynthesisStarted = NULL;
  this->_onSynthesisCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onBinaryDataReceived = NULL;
}

SpeechSynthesizerCallback::~SpeechSynthesizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onSynthesisStarted = NULL;
  this->_onSynthesisCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onBinaryDataReceived = NULL;

  std::map<NlsEvent::EventType, void*>::iterator iter;
  for (iter = _paramap.begin(); iter != _paramap.end();) {
    _paramap.erase(iter++);
  }
  _paramap.clear();
}

void SpeechSynthesizerCallback::setOnTaskFailed(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnTaskFailed callback");
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void SpeechSynthesizerCallback::setOnSynthesisStarted(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnSynthesisStarted callback");

  this->_onSynthesisStarted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisStarted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisStarted, para));
  }
}

void SpeechSynthesizerCallback::setOnSynthesisCompleted(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnSynthesisCompleted callback");

  this->_onSynthesisCompleted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisCompleted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisCompleted, para));
  }
}

void SpeechSynthesizerCallback::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnChannelClosed callback");

  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

void SpeechSynthesizerCallback::setOnBinaryDataReceived(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnBinaryDataReceived callback");

  this->_onBinaryDataReceived = _event;
  if (this->_paramap.find(NlsEvent::Binary) != _paramap.end()) {
    _paramap[NlsEvent::Binary] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Binary, para));
  }
}

void SpeechSynthesizerCallback::setOnMetaInfo(
    NlsCallbackMethod _event, void* para) {
  //LOG_DEBUG("setOnMetaInfo callback");

  this->_onMetaInfo = _event;
  if (this->_paramap.find(NlsEvent::MetaInfo) != _paramap.end()) {
    _paramap[NlsEvent::MetaInfo] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::MetaInfo, para));
  }
}

SpeechSynthesizerRequest::SpeechSynthesizerRequest(int version) {
  _callback = new SpeechSynthesizerCallback();

  //init request param
  _synthesizerParam = new SpeechSynthesizerParam(version);
  _requestParam = _synthesizerParam;

  //init listener
  _listener = new SpeechSynthesizerListener(_callback);

  //init connect node
  _node = new ConnectNode(this, _listener);

  LOG_DEBUG("Create SpeechSynthesizerRequest.");
}

SpeechSynthesizerRequest::~SpeechSynthesizerRequest() {
//  delete _synthesizerParam;
//  _synthesizerParam = NULL;

  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _synthesizerParam;
  _synthesizerParam = NULL;

  LOG_DEBUG("Destroy SpeechSynthesizerRequest.");
}

int SpeechSynthesizerRequest::start() {
  _synthesizerParam->setNlsRequestType(SpeechSynthesizer);
  return INlsRequest::start(this);
}

int SpeechSynthesizerRequest::stop() {
  //    return INlsRequest::stop(this);
  return 0;
}

int SpeechSynthesizerRequest::cancel() {
  return INlsRequest::stop(this, 1);
  //    return INlsRequest::cancel(this);
}

int SpeechSynthesizerRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _synthesizerParam->setPayloadParam(value);
}

int SpeechSynthesizerRequest::setContextParam(const char *value) {
  INPUT_PARAM_STRING_CHECK(value);
  return _synthesizerParam->setContextParam(value);
}

int SpeechSynthesizerRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _synthesizerParam->setUrl(value);
  return 0;
}

int SpeechSynthesizerRequest::setAppKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _synthesizerParam->setAppKey(value);
  return 0;
}

int SpeechSynthesizerRequest::setToken(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _synthesizerParam->setToken(value);
  return 0;
}

int SpeechSynthesizerRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _synthesizerParam->setFormat(value);
  return 0;
}

int SpeechSynthesizerRequest::setSampleRate(int value) {
  _synthesizerParam->setSampleRate(value);
  return 0;
}

int SpeechSynthesizerRequest::setText(const char* value) {
  return _synthesizerParam->setText(value);
}

int SpeechSynthesizerRequest::setMethod(int value) {
  return _synthesizerParam->setMethod(value);
}

int SpeechSynthesizerRequest::setPitchRate(int value) {
  return _synthesizerParam->setPitchRate(value);
}

int SpeechSynthesizerRequest::setEnableSubtitle(bool value) {
  _synthesizerParam->setEnableSubtitle(value);
  return 0;
}

int SpeechSynthesizerRequest::setSpeechRate(int value) {
  return _synthesizerParam->setSpeechRate(value);
}

int SpeechSynthesizerRequest::setVolume(int value) {
  return _synthesizerParam->setVolume(value);
}

int SpeechSynthesizerRequest::setVoice(const char* value) {
  return _synthesizerParam->setVoice(value);
}

int SpeechSynthesizerRequest::setTimeout(int value) {
  _synthesizerParam->setTimeout(value);
  return 0;
}

int SpeechSynthesizerRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  _synthesizerParam->setOutputFormat(value);
  return 0;
}

void SpeechSynthesizerRequest::setOnTaskFailed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void SpeechSynthesizerRequest::setOnSynthesisCompleted(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnSynthesisCompleted(_event, para);
}

void SpeechSynthesizerRequest::setOnChannelClosed(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void SpeechSynthesizerRequest::setOnBinaryDataReceived(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnBinaryDataReceived(_event, para);
}

void SpeechSynthesizerRequest::setOnMetaInfo(
    NlsCallbackMethod _event, void* para) {
  _callback->setOnMetaInfo(_event, para);
}

int SpeechSynthesizerRequest::AppendHttpHeaderParam(
    const char* key, const char* value) {
  return _synthesizerParam->AppendHttpHeader(key, value);
}

}
