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
#include "log.h"
#include "utility.h"
#include "connectNode.h"
#include "iNlsRequestListener.h"
#include "speechTranscriberParam.h"
#include "speechTranscriberListener.h"

namespace AlibabaNls {

using std::map;
using std::string;
using namespace utility;

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

void SpeechTranscriberCallback::setOnSentenceSemantics(NlsCallbackMethod _event, void* para) {
    LOG_DEBUG("setOnSentenceSemantics callback");

    this->_onSentenceSemantics = _event;
    if (this->_paramap.find(NlsEvent::SentenceSemantics) != _paramap.end()) {
        _paramap[NlsEvent::SentenceSemantics] = para;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::SentenceSemantics, para));
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

SpeechTranscriberRequest::SpeechTranscriberRequest() {
    _callback = new SpeechTranscriberCallback();

    //init request param
    _transcriberParam = new SpeechTranscriberParam();
    _requestParam = _transcriberParam;

    //init listener
    _listener = new SpeechTranscriberListener(_callback);

    //init connect node
    _node = new ConnectNode(this, _listener);

    LOG_INFO("Create SpeechTranscriberRequest.");
}

SpeechTranscriberRequest::~SpeechTranscriberRequest() {
    delete _transcriberParam;
    _transcriberParam = NULL;

    delete _listener;
    _listener = NULL;

    delete _callback;
    _callback = NULL;

    delete _node;
    _node = NULL;

    LOG_INFO("Destroy SpeechTranscriberRequest.");
}

//const char * SpeechTranscriberRequest::getRequestErrorMsg() {
//    return getConnectNode()->getErrorMessage();
//}
//
//int SpeechTranscriberRequest::getRequestErrorStatus() {
//    return getConnectNode()->getErrorCode();
//}

int SpeechTranscriberRequest::start() {
    return INlsRequest::start(this);
}

int SpeechTranscriberRequest::control(const char* message) {
    return INlsRequest::stControl(this, message);
}

int SpeechTranscriberRequest::stop() {
    return INlsRequest::stop(this, 0);
}

int SpeechTranscriberRequest::cancel() {
    return INlsRequest::stop(this, 1);
//    return INlsRequest::cancel(this);
}

int SpeechTranscriberRequest::sendAudio(const uint8_t * data, size_t dataSize, bool encoded) {
    return INlsRequest::sendAudio(this, data, dataSize, encoded);
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

    return 0;
}

int SpeechTranscriberRequest::setUrl(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    _transcriberParam->setUrl(value);

    return 0;
}

int SpeechTranscriberRequest::setAppKey(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    _transcriberParam->setAppKey(value);

    return 0;
}

int SpeechTranscriberRequest::setFormat(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    _transcriberParam->setFormat(value);

    return 0;
}

int SpeechTranscriberRequest::setSampleRate(int value) {
    _transcriberParam->setSampleRate(value);

    return 0;
}

int SpeechTranscriberRequest::setIntermediateResult(bool value) {
    _transcriberParam->setIntermediateResult(value);
    return 0;
}

int SpeechTranscriberRequest::setPunctuationPrediction(bool value) {
    _transcriberParam->setPunctuationPrediction(value);
    return 0;
}

int SpeechTranscriberRequest::setInverseTextNormalization(bool value) {
    _transcriberParam->setTextNormalization(value);
    return 0;
}

int SpeechTranscriberRequest::AppendHttpHeaderParam(const char* key, const char* value) {
    return _transcriberParam->AppendHttpHeader(key, value);
}

int SpeechTranscriberRequest::setSemanticSentenceDetection(bool value) {
    _transcriberParam->setSentenceDetection(value);
    return 0;
}

int SpeechTranscriberRequest::setMaxSentenceSilence(int value) {
    return _transcriberParam->setMaxSentenceSilence(value);
}

int SpeechTranscriberRequest::setCustomizationId(const char * value) {
    return _transcriberParam->setCustomizationId(value);
}

int SpeechTranscriberRequest::setVocabularyId(const char * value) {
    return _transcriberParam->setVocabularyId(value);
}

int SpeechTranscriberRequest::setTimeout(int value) {
    _transcriberParam->setTimeout(value);

    return 0;
}

int SpeechTranscriberRequest::setOutputFormat(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);
    _transcriberParam->setOutputFormat(value);

    return 0;
}

int SpeechTranscriberRequest::setNlpModel(const char* value) {
    return _transcriberParam->setNlpModel(value);
}

int SpeechTranscriberRequest::setEnableNlp(bool enable) {
    return _transcriberParam->setEnableNlp(enable);
}

int SpeechTranscriberRequest::setSessionId(const char* value) {
    return _transcriberParam->setSessionId(value);
}

void SpeechTranscriberRequest::setOnTaskFailed(NlsCallbackMethod _event, void* para) {
    _callback->setOnTaskFailed(_event, para);
}

void SpeechTranscriberRequest::setOnTranscriptionStarted(NlsCallbackMethod _event, void* para) {
    _callback->setOnTranscriptionStarted(_event, para);
}

void SpeechTranscriberRequest::setOnSentenceBegin(NlsCallbackMethod _event, void* para) {
    _callback->setOnSentenceBegin(_event, para);
}

void SpeechTranscriberRequest::setOnTranscriptionResultChanged(NlsCallbackMethod _event, void* para) {
    _callback->setOnTranscriptionResultChanged(_event, para);
}

void SpeechTranscriberRequest::setOnSentenceEnd(NlsCallbackMethod _event, void* para) {
    _callback->setOnSentenceEnd(_event, para);
}

void SpeechTranscriberRequest::setOnTranscriptionCompleted(NlsCallbackMethod _event, void* para) {
    _callback->setOnTranscriptionCompleted(_event, para);
}

void SpeechTranscriberRequest::setOnChannelClosed(NlsCallbackMethod _event, void* para) {
    _callback->setOnChannelClosed(_event, para);
}

void SpeechTranscriberRequest::setOnSentenceSemantics(NlsCallbackMethod _event, void* para) {
    _callback->setOnSentenceSemantics(_event, para);
}

}
