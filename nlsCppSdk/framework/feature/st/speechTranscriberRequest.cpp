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

#include "speechTranscriberRequest.h"
#include <string>
#include "log.h"
#include "nlsRequestParamInfo.h"
#include "nlsSessionBase.h"
#include "speechTranscriberParam.h"
#include "speechTranscriberListener.h"

using std::string;

namespace AlibabaNls {

using namespace util;

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
}

void SpeechTranscriberCallback::setOnTaskFailed(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnTaskFailed callback");

    this->_onTaskFailed = _event;
    if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
        _paramap[NlsEvent::TaskFailed] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::TaskFailed, para));
    }
}

void SpeechTranscriberCallback::setOnTranscriptionStarted(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnTranscriptionStarted callback");

	this->_onTranscriptionStarted = _event;
    if (this->_paramap.find(NlsEvent::TranscriptionStarted) != _paramap.end()) {
        _paramap[NlsEvent::TranscriptionStarted] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::TranscriptionStarted, para));
    }
}

void SpeechTranscriberCallback::setOnSentenceBegin(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnSentenceBegin callback");

	this->_onSentenceBegin = _event;
    if (this->_paramap.find(NlsEvent::SentenceBegin) != _paramap.end()) {
        _paramap[NlsEvent::SentenceBegin] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::SentenceBegin, para));
    }
}

void SpeechTranscriberCallback::setOnTranscriptionResultChanged(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnTranscriptionResultChanged callback");
	
	this->_onTranscriptionResultChanged = _event;
    if (this->_paramap.find(NlsEvent::TranscriptionResultChanged) != _paramap.end()) {
        _paramap[NlsEvent::TranscriptionResultChanged] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::TranscriptionResultChanged, para));
    }
}

void SpeechTranscriberCallback::setOnSentenceEnd(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnSentenceEnd callback");
	
	this->_onSentenceEnd = _event;
    if (this->_paramap.find(NlsEvent::SentenceEnd) != _paramap.end()) {
        _paramap[NlsEvent::SentenceEnd] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::SentenceEnd, para));
    }
}

void SpeechTranscriberCallback::setOnTranscriptionCompleted(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnTranscriptionCompleted callback");
	
	this->_onTranscriptionCompleted = _event;
    if (this->_paramap.find(NlsEvent::TranscriptionCompleted) != _paramap.end()) {
        _paramap[NlsEvent::TranscriptionCompleted] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::TranscriptionCompleted, para));
    }
}

void SpeechTranscriberCallback::setOnChannelClosed(NlsCallbackMethod _event, void* para) {
	LOG_DEBUG("setOnChannelClosed callback");
	
	this->_onChannelClosed = _event;
    if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
        _paramap[NlsEvent::Close] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::Close, para));
    }
}

SpeechTranscriberRequest::SpeechTranscriberRequest(SpeechTranscriberCallback* cb) {
    _transcriberParam = new SpeechTranscriberParam();
    _requestParam = _transcriberParam;

    _listener = new SpeechTranscriberListener(cb);

	_session = NULL;
}

SpeechTranscriberRequest::~SpeechTranscriberRequest() {
	if (_session) {
		delete _session;
		_session = NULL;
	}

    delete _transcriberParam;
    _transcriberParam = NULL;

    _requestParam = NULL;

    delete _listener;
    _listener = NULL;
}

int SpeechTranscriberRequest::start() {
    return INlsRequest::start();
}

int SpeechTranscriberRequest::stop() {
    return INlsRequest::stop();
}

int SpeechTranscriberRequest::cancel() {
    return INlsRequest::cancel();
}

int SpeechTranscriberRequest::sendAudio(char* data, int dataSzie, bool encoded ) {
    return INlsRequest::sendAudio(data, dataSzie, encoded);
}

int SpeechTranscriberRequest::setToken(const char*token) {
    return INlsRequest::setToken(token);
}

int SpeechTranscriberRequest::setUrl(const char* value) {
    return INlsRequest::setUrl(value);
}

int SpeechTranscriberRequest::setAppKey(const char* value) {
    return INlsRequest::setAppKey(value);
}

int SpeechTranscriberRequest::setFormat(const char* value) {
    return INlsRequest::setFormat(value);
}

int SpeechTranscriberRequest::setSampleRate(int value) {
    return INlsRequest::setSampleRate(value);
}

int SpeechTranscriberRequest::setIntermediateResult(bool value) {
    return _transcriberParam->setIntermediateResult(value);
}

int SpeechTranscriberRequest::setPunctuationPrediction(bool value) {
    return _transcriberParam->setPunctuationPrediction(value);
}

int SpeechTranscriberRequest::setInverseTextNormalization(bool value) {
    return _transcriberParam->setTextNormalization(value);
}

int SpeechTranscriberRequest::setSemanticSentenceDetection(bool value) {
    return _transcriberParam->setSentenceDetection(value);
}

int SpeechTranscriberRequest::setMaxSentenceSilence(int value) {
    return _transcriberParam->setMaxSentenceSilence(value);
}

int SpeechTranscriberRequest::setTimeout(int value) {
    return INlsRequest::setTimeout(value);
}

int SpeechTranscriberRequest::setOutputFormat(const char* value) {
    return INlsRequest::setOutputFormat(value);
}

int SpeechTranscriberRequest::setPayloadParam(const char* value) {
    return INlsRequest::setPayloadParam(value);
}

int SpeechTranscriberRequest::setContextParam(const char *value) {
    return INlsRequest::setContextParam(value);
}

int SpeechTranscriberRequest::getTranscriberResult(std::queue<NlsEvent>* eventQueue) {
    return INlsRequest::getRecognizerResult(eventQueue);
}

}
