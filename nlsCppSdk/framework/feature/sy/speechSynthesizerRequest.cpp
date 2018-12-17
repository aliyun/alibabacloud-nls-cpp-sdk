/*
 * Copyright 2015 Alibaba Group Holding Limited
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

#include <string>
#include "log.h"
#include "nlsSessionBase.h"
#include "speechSynthesizerRequest.h"
#include "speechSynthesizerParam.h"
#include "speechSynthesizerListener.h"

using std::string;

namespace AlibabaNls {

using namespace util;

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
}

void SpeechSynthesizerCallback::setOnTaskFailed(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnTaskFailed callback");

	this->_onTaskFailed = _event;
    if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
        _paramap[NlsEvent::TaskFailed] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
    }
}

void SpeechSynthesizerCallback::setOnSynthesisStarted(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnSynthesisStarted callback");
	
	this->_onSynthesisStarted = _event;
    if (this->_paramap.find(NlsEvent::SynthesisStarted) != _paramap.end()) {
        _paramap[NlsEvent::SynthesisStarted] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::SynthesisStarted, para));
    }
}

void SpeechSynthesizerCallback::setOnSynthesisCompleted(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnSynthesisCompleted callback");
	
	this->_onSynthesisCompleted = _event;
    if (this->_paramap.find(NlsEvent::SynthesisCompleted) != _paramap.end()) {
        _paramap[NlsEvent::SynthesisCompleted] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::SynthesisCompleted, para));
    }
}

void SpeechSynthesizerCallback::setOnChannelClosed(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnChannelClosed callback");
	
	this->_onChannelClosed = _event;
    if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
        _paramap[NlsEvent::Close] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::Close, para));
    }
}

void SpeechSynthesizerCallback::setOnBinaryDataReceived(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnBinaryDataReceived callback");
	
	this->_onBinaryDataReceived = _event;
    if (this->_paramap.find(NlsEvent::Binary) != _paramap.end()) {
        _paramap[NlsEvent::Binary] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::Binary, para));
    }
}

SpeechSynthesizerRequest::SpeechSynthesizerRequest(SpeechSynthesizerCallback* cb) {
    _synthesizerParam = new SpeechSynthesizerParam();

    _requestParam = _synthesizerParam;

    _listener = new SpeechSynthesizerListener(cb);

    _session = NULL;
}

SpeechSynthesizerRequest::~SpeechSynthesizerRequest() {
    if (_session) {
        delete _session;
        _session = NULL;
    }

    delete _synthesizerParam;
    _synthesizerParam = NULL;

    _requestParam = NULL;

    delete _listener;
    _listener = NULL;
}

int SpeechSynthesizerRequest::start() {
    _synthesizerParam->setNlsRequestType(SpeechSynthesizer);
    return INlsRequest::start();
}

int SpeechSynthesizerRequest::stop() {
    return INlsRequest::stop();
}

int SpeechSynthesizerRequest::cancel() {
    return INlsRequest::cancel();
}

int SpeechSynthesizerRequest::setPayloadParam(const char* value) {
    return INlsRequest::setPayloadParam(value);
}

int SpeechSynthesizerRequest::setContextParam(const char *value) {
    return INlsRequest::setContextParam(value);
}

int SpeechSynthesizerRequest::setUrl(const char* value) {
    return INlsRequest::setUrl(value);
}

int SpeechSynthesizerRequest::setAppKey(const char* value) {
    return INlsRequest::setAppKey(value);
}

int SpeechSynthesizerRequest::setToken(const char*token) {
    return INlsRequest::setToken(token);
}

int SpeechSynthesizerRequest::setFormat(const char* value) {
    return INlsRequest::setFormat(value);
}

int SpeechSynthesizerRequest::setSampleRate(int value) {
    return INlsRequest::setSampleRate(value);
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
    return INlsRequest::setTimeout(value);
}

int SpeechSynthesizerRequest::setOutputFormat(const char* value) {
    return INlsRequest::setOutputFormat(value);
}

}
