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

#include "speechRecognizerRequest.h"
#include <string>
#include "log.h"
#include "nlsSessionBase.h"
#include "speechRecognizerParam.h"
#include "speechRecognizerListener.h"
#include "nlsRequestParamInfo.h"

using std::string;

namespace AlibabaNls {

using namespace util;

SpeechRecognizerCallback::SpeechRecognizerCallback() {
    this->_onTaskFailed = NULL;
    this->_onRecognitionStarted = NULL;
    this->_onRecognitionCompleted = NULL;
    this->_onRecognitionResultChanged = NULL;
    this->_onChannelClosed = NULL;
}

SpeechRecognizerCallback::~SpeechRecognizerCallback() {
    this->_onTaskFailed = NULL;
    this->_onRecognitionStarted = NULL;
    this->_onRecognitionCompleted = NULL;
    this->_onRecognitionResultChanged = NULL;
    this->_onChannelClosed = NULL;
}

void SpeechRecognizerCallback::setOnTaskFailed(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnTaskFailed callback");

    this->_onTaskFailed = _event;
    if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
        _paramap[NlsEvent::TaskFailed] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
    }
}

void SpeechRecognizerCallback::setOnRecognitionStarted(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnRecognitionStarted callback");

	this->_onRecognitionStarted = _event;
    if (this->_paramap.find(NlsEvent::RecognitionStarted) != _paramap.end()) {
        _paramap[NlsEvent::RecognitionStarted] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::RecognitionStarted, para));
    }
}

void SpeechRecognizerCallback::setOnRecognitionCompleted(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnRecognitionCompleted callback");
	this->_onRecognitionCompleted = _event;
    if (this->_paramap.find(NlsEvent::RecognitionCompleted) != _paramap.end()) {
        _paramap[NlsEvent::RecognitionCompleted] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::RecognitionCompleted, para));
    }
}

void SpeechRecognizerCallback::setOnRecognitionResultChanged(NlsCallbackMethod _event, void* para) {
    LOG_DEBUG("setOnRecognitionResultChanged callback");
	this->_onRecognitionResultChanged = _event;
    if (this->_paramap.find(NlsEvent::RecognitionResultChanged) != _paramap.end()) {
        _paramap[NlsEvent::RecognitionResultChanged] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::RecognitionResultChanged, para));
    }
}

void SpeechRecognizerCallback::setOnChannelClosed(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnChannelClosed callback");
	this->_onChannelClosed = _event;
    if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
        _paramap[NlsEvent::Close] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::Close, para));
    }
}

SpeechRecognizerRequest::SpeechRecognizerRequest(SpeechRecognizerCallback* cb) {
    _recognizerParam = new SpeechRecognizerParam();
    _requestParam = _recognizerParam;

    _listener = new SpeechRecognizerListener(cb);

	_session = NULL;
}

SpeechRecognizerRequest::~SpeechRecognizerRequest() {
	if (_session) {
		delete _session;
		_session = NULL;
	}

	delete _recognizerParam;
    _recognizerParam = NULL;

    _requestParam = NULL;

    delete _listener;
    _listener = NULL;
}

int SpeechRecognizerRequest::start() {
    return INlsRequest::start();
}

int SpeechRecognizerRequest::stop() {
    return INlsRequest::stop();
}

int SpeechRecognizerRequest::cancel() {
    return INlsRequest::cancel();
}

int SpeechRecognizerRequest::sendAudio(char* data, int dataSzie, bool encoded) {
    return INlsRequest::sendAudio(data, dataSzie, encoded);
}

int SpeechRecognizerRequest::getRecognizerResult(std::queue<NlsEvent>* eventQueue) {
	return INlsRequest::getRecognizerResult(eventQueue);
}

int SpeechRecognizerRequest::setPayloadParam(const char* value) {
    return INlsRequest::setPayloadParam(value);
}

int SpeechRecognizerRequest::setContextParam(const char *value) {
    return INlsRequest::setContextParam(value);
}

int SpeechRecognizerRequest::setToken(const char*token) {
    return INlsRequest::setToken(token);
}

int SpeechRecognizerRequest::setUrl(const char* value) {
    return INlsRequest::setUrl(value);
}

int SpeechRecognizerRequest::setAppKey(const char* value) {
    return INlsRequest::setAppKey(value);
}

int SpeechRecognizerRequest::setFormat(const char* value) {
    return INlsRequest::setFormat(value);
}

int SpeechRecognizerRequest::setSampleRate(int value) {
    return INlsRequest::setSampleRate(value);
}

int SpeechRecognizerRequest::setIntermediateResult(bool value) {
    return _recognizerParam->setIntermediateResult(value);
}

int SpeechRecognizerRequest::setPunctuationPrediction(bool value) {
    return _recognizerParam->setPunctuationPrediction(value);
}

int SpeechRecognizerRequest::setInverseTextNormalization(bool value) {
    return _recognizerParam->setTextNormalization(value);
}

int SpeechRecognizerRequest::setEnableVoiceDetection(bool value) {
    return _recognizerParam->setEnableVoiceDetection(value);
}

int SpeechRecognizerRequest::setMaxStartSilence(int value) {
    return _recognizerParam->setMaxStartSilence(value);
}

int SpeechRecognizerRequest::setMaxEndSilence(int value) {
    return _recognizerParam->setMaxEndSilence(value);
}

int SpeechRecognizerRequest::setTimeout(int value) {
    return INlsRequest::setTimeout(value);
}

int SpeechRecognizerRequest::setOutputFormat(const char* value) {
    return INlsRequest::setOutputFormat(value);
}

}
