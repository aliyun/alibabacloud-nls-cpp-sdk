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

#include "speechSynthesizerRequest.h"

#include "connectNode.h"
#include "iNlsRequestListener.h"
#include "nlog.h"
#include "speechSynthesizerListener.h"
#include "speechSynthesizerParam.h"
#include "utility.h"

namespace AlibabaNls {

SpeechSynthesizerCallback::SpeechSynthesizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onSynthesisStarted = NULL;
  this->_onSynthesisCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onBinaryDataReceived = NULL;
  this->_onMessage = NULL;
}

SpeechSynthesizerCallback::~SpeechSynthesizerCallback() {
  this->_onTaskFailed = NULL;
  this->_onSynthesisStarted = NULL;
  this->_onSynthesisCompleted = NULL;
  this->_onChannelClosed = NULL;
  this->_onBinaryDataReceived = NULL;
  this->_onMessage = NULL;

  std::map<NlsEvent::EventType, void*>::iterator iter;
  for (iter = _paramap.begin(); iter != _paramap.end();) {
    _paramap.erase(iter++);
  }
  _paramap.clear();
}

void SpeechSynthesizerCallback::setOnTaskFailed(NlsCallbackMethod _event,
                                                void* para) {
  this->_onTaskFailed = _event;
  if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
    _paramap[NlsEvent::TaskFailed] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
  }
}

void SpeechSynthesizerCallback::setOnSynthesisStarted(NlsCallbackMethod _event,
                                                      void* para) {
  this->_onSynthesisStarted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisStarted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisStarted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisStarted, para));
  }
}

void SpeechSynthesizerCallback::setOnSynthesisCompleted(
    NlsCallbackMethod _event, void* para) {
  this->_onSynthesisCompleted = _event;
  if (this->_paramap.find(NlsEvent::SynthesisCompleted) != _paramap.end()) {
    _paramap[NlsEvent::SynthesisCompleted] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::SynthesisCompleted, para));
  }
}

void SpeechSynthesizerCallback::setOnChannelClosed(NlsCallbackMethod _event,
                                                   void* para) {
  this->_onChannelClosed = _event;
  if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
    _paramap[NlsEvent::Close] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Close, para));
  }
}

void SpeechSynthesizerCallback::setOnBinaryDataReceived(
    NlsCallbackMethod _event, void* para) {
  this->_onBinaryDataReceived = _event;
  if (this->_paramap.find(NlsEvent::Binary) != _paramap.end()) {
    _paramap[NlsEvent::Binary] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Binary, para));
  }
}

void SpeechSynthesizerCallback::setOnMetaInfo(NlsCallbackMethod _event,
                                              void* para) {
  this->_onMetaInfo = _event;
  if (this->_paramap.find(NlsEvent::MetaInfo) != _paramap.end()) {
    _paramap[NlsEvent::MetaInfo] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::MetaInfo, para));
  }
}

void SpeechSynthesizerCallback::setOnMessage(NlsCallbackMethod _event,
                                             void* para) {
  this->_onMessage = _event;
  if (this->_paramap.find(NlsEvent::Message) != _paramap.end()) {
    _paramap[NlsEvent::Message] = para;
  } else {
    _paramap.insert(std::make_pair(NlsEvent::Message, para));
  }
}

SpeechSynthesizerRequest::SpeechSynthesizerRequest(int version,
                                                   const char* sdkName,
                                                   bool isLongConnection) {
  _callback = new SpeechSynthesizerCallback();

  // init request param
  _synthesizerParam = new SpeechSynthesizerParam(version, sdkName);
  _requestParam = _synthesizerParam;

  // init listener
  _listener = new SpeechSynthesizerListener(_callback);

  // init connect node
  _node = new ConnectNode(this, _listener, isLongConnection);

  LOG_DEBUG(
      "Request(%p) Node(%p) create SpeechSynthesizerRequest with long Connect "
      "flag(%d) "
      "Done.",
      this, _node, isLongConnection);
}

SpeechSynthesizerRequest::~SpeechSynthesizerRequest() {
  delete _listener;
  _listener = NULL;

  delete _callback;
  _callback = NULL;

  delete _node;
  _node = NULL;

  delete _synthesizerParam;
  _synthesizerParam = NULL;
  _requestParam = NULL;

  LOG_INFO("Request(%p) destroy SpeechSynthesizerRequest Done.", this);
}

int SpeechSynthesizerRequest::start() {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setNlsRequestType(SpeechSynthesizer);
  return INlsRequest::start(this);
}

int SpeechSynthesizerRequest::stop() { return Success; }

int SpeechSynthesizerRequest::cancel() { return INlsRequest::cancel(this); }

const char* SpeechSynthesizerRequest::dumpAllInfo() {
  return INlsRequest::dumpAllInfo(this);
}

int SpeechSynthesizerRequest::setPayloadParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->setPayloadParam(value);
}

int SpeechSynthesizerRequest::setContextParam(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->setContextParam(value);
}

int SpeechSynthesizerRequest::setUrl(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setUrl(value);
  return Success;
}

int SpeechSynthesizerRequest::setAppKey(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setAppKey(value);
  return Success;
}

int SpeechSynthesizerRequest::setToken(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setToken(value);
  return 0;
}

int SpeechSynthesizerRequest::setFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setFormat(value);
  return Success;
}

int SpeechSynthesizerRequest::setSampleRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setSampleRate(value);
  return Success;
}

int SpeechSynthesizerRequest::setText(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  LOG_DEBUG("Request(%p) setText(%dbytes): %s", this, strlen(value), value);
  return _synthesizerParam->setText(value);
}

int SpeechSynthesizerRequest::setMethod(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->setMethod(value);
}

int SpeechSynthesizerRequest::setPitchRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->setPitchRate(value);
}

int SpeechSynthesizerRequest::setEnableSubtitle(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setEnableSubtitle(value);
  return Success;
}

int SpeechSynthesizerRequest::setSpeechRate(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->setSpeechRate(value);
}

int SpeechSynthesizerRequest::setVolume(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->setVolume(value);
}

int SpeechSynthesizerRequest::setVoice(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->setVoice(value);
}

int SpeechSynthesizerRequest::setTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setTimeout(value);
  return Success;
}

int SpeechSynthesizerRequest::setRecvTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setRecvTimeout(value);
  return Success;
}

int SpeechSynthesizerRequest::setSendTimeout(int value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setSendTimeout(value);
  return Success;
}

int SpeechSynthesizerRequest::setOutputFormat(const char* value) {
  INPUT_PARAM_STRING_CHECK(value);
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setOutputFormat(value);
  return Success;
}

int SpeechSynthesizerRequest::setEnableOnMessage(bool value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setEnableOnMessage(value);
  return Success;
}

int SpeechSynthesizerRequest::setEnableContinued(bool enable) {
#ifdef ENABLE_CONTINUED
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  _synthesizerParam->setEnableContinued(enable);
  return Success;
#else
  return -(InvalidRequest);
#endif
}

const char* SpeechSynthesizerRequest::getOutputFormat() {
  return _synthesizerParam->getOutputFormat().c_str();
}

const char* SpeechSynthesizerRequest::getTaskId() {
  return _synthesizerParam->getTaskId().c_str();
}

void SpeechSynthesizerRequest::setOnTaskFailed(NlsCallbackMethod _event,
                                               void* para) {
  _callback->setOnTaskFailed(_event, para);
}

void SpeechSynthesizerRequest::setOnSynthesisStarted(NlsCallbackMethod _event,
                                                     void* para) {
  _callback->setOnSynthesisStarted(_event, para);
}

void SpeechSynthesizerRequest::setOnSynthesisCompleted(NlsCallbackMethod _event,
                                                       void* para) {
  _callback->setOnSynthesisCompleted(_event, para);
}

void SpeechSynthesizerRequest::setOnChannelClosed(NlsCallbackMethod _event,
                                                  void* para) {
  _callback->setOnChannelClosed(_event, para);
}

void SpeechSynthesizerRequest::setOnBinaryDataReceived(NlsCallbackMethod _event,
                                                       void* para) {
  _callback->setOnBinaryDataReceived(_event, para);
}

void SpeechSynthesizerRequest::setOnMetaInfo(NlsCallbackMethod _event,
                                             void* para) {
  _callback->setOnMetaInfo(_event, para);
}

void SpeechSynthesizerRequest::setOnMessage(NlsCallbackMethod _event,
                                            void* para) {
  _callback->setOnMessage(_event, para);
}

int SpeechSynthesizerRequest::AppendHttpHeaderParam(const char* key,
                                                    const char* value) {
  INPUT_REQUEST_PARAM_CHECK(_synthesizerParam);
  return _synthesizerParam->AppendHttpHeader(key, value);
}

}  // namespace AlibabaNls
