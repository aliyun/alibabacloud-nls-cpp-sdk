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
#include "log.h"
#include "utility.h"
#include "connectNode.h"
#include "iNlsRequestListener.h"
#include "speechRecognizerParam.h"
#include "speechRecognizerListener.h"

namespace AlibabaNls {

using std::map;
using std::string;
using namespace utility;

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

void SpeechRecognizerCallback::setOnTaskFailed(NlsCallbackMethod event, void* param) {
	LOG_DEBUG("setOnTaskFailed callback");

    if (param == NULL) {
        LOG_DEBUG("setOnTaskFailed NULL");
    }

    this->_onTaskFailed = event;
    if (this->_paramap.find(NlsEvent::TaskFailed) != _paramap.end()) {
        _paramap[NlsEvent::TaskFailed] = param;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::TaskFailed, param));
    }
}

void SpeechRecognizerCallback::setOnRecognitionStarted(NlsCallbackMethod event, void* param) {
    LOG_DEBUG("setOnRecognitionStarted callback");

    if (param == NULL) {
        LOG_DEBUG("setOnRecognitionStarted NULL");
    }

	this->_onRecognitionStarted = event;
    if (this->_paramap.find(NlsEvent::RecognitionStarted) != _paramap.end()) {
        _paramap[NlsEvent::RecognitionStarted] = param;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::RecognitionStarted, param));
    }
}

void SpeechRecognizerCallback::setOnRecognitionCompleted(NlsCallbackMethod event, void* param) {
    LOG_DEBUG("setOnRecognitionCompleted callback");

    if (param == NULL) {
        LOG_DEBUG("setOnRecognitionCompleted NULL");
    }

	this->_onRecognitionCompleted = event;
    if (this->_paramap.find(NlsEvent::RecognitionCompleted) != _paramap.end()) {
        _paramap[NlsEvent::RecognitionCompleted] = param;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::RecognitionCompleted, param));
    }
}

void SpeechRecognizerCallback::setOnRecognitionResultChanged(NlsCallbackMethod event, void* param) {
    LOG_DEBUG("setOnRecognitionResultChanged callback");

    if (param == NULL) {
        LOG_DEBUG("setOnRecognitionResultChanged NULL");
    }

	this->_onRecognitionResultChanged = event;
    if (this->_paramap.find(NlsEvent::RecognitionResultChanged) != _paramap.end()) {
        _paramap[NlsEvent::RecognitionResultChanged] = param;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::RecognitionResultChanged, param));
    }
}

void SpeechRecognizerCallback::setOnChannelClosed(NlsCallbackMethod event, void* param) {
    LOG_DEBUG("setOnChannelClosed callback");

    if (param == NULL) {
        LOG_DEBUG("setOnChannelClosed NULL");
    }

	this->_onChannelClosed = event;
    if (this->_paramap.find(NlsEvent::Close) != _paramap.end()) {
        _paramap[NlsEvent::Close] = param;
    } else {
        _paramap.insert(std::make_pair(NlsEvent::Close, param));
    }
}

SpeechRecognizerRequest::SpeechRecognizerRequest() {
    _callback = new SpeechRecognizerCallback();

    //init request param
    _recognizerParam = new SpeechRecognizerParam();
	_requestParam = _recognizerParam;

    //init listener
    _listener = new SpeechRecognizerListener(_callback);

    //init connect node
    _node = new ConnectNode(this, _listener);

    LOG_INFO("Create SpeechRecognizerRequest.");
}

SpeechRecognizerRequest::~SpeechRecognizerRequest() {
	delete _recognizerParam;
    _recognizerParam = NULL;

    delete _listener;
    _listener = NULL;

    delete _callback;
    _callback = NULL;

    delete _node;
    _node = NULL;

    LOG_INFO("Destroy SpeechRecognizerRequest.");
}

//const char * SpeechRecognizerRequest::getRequestErrorMsg() {
//    return getConnectNode()->getErrorMessage();
//}
//
//int SpeechRecognizerRequest::getRequestErrorStatus() {
//    return getConnectNode()->getErrorCode();
//}

int SpeechRecognizerRequest::start() {
    return INlsRequest::start(this);
}

int SpeechRecognizerRequest::stop() {
    return INlsRequest::stop(this, 0);
}

int SpeechRecognizerRequest::cancel() {
    return INlsRequest::stop(this, 1);
//    return INlsRequest::cancel(this);
}

int SpeechRecognizerRequest::sendAudio(const uint8_t * data, size_t dataSize, bool encoded) {
    return INlsRequest::sendAudio(this, data, dataSize, encoded);
}

int SpeechRecognizerRequest::setPayloadParam(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    return _recognizerParam->setPayloadParam(value);
}

int SpeechRecognizerRequest::setContextParam(const char *value) {
    INPUT_PARAM_STRING_CHECK(value);

    return _recognizerParam->setContextParam(value);
}

int SpeechRecognizerRequest::setToken(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    _recognizerParam->setToken(value);

    return 0;
}

int SpeechRecognizerRequest::setUrl(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    _recognizerParam->setUrl(value);

    return 0;
}

int SpeechRecognizerRequest::setAppKey(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    _recognizerParam->setAppKey(value);

    return 0;
}

int SpeechRecognizerRequest::setFormat(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);

    _recognizerParam->setFormat(value);

    return 0;
}

int SpeechRecognizerRequest::setSampleRate(int value) {

    _recognizerParam->setSampleRate(value);

    return 0;
}

int SpeechRecognizerRequest::setIntermediateResult(bool value) {
    _recognizerParam->setIntermediateResult(value);

    return 0;
}

int SpeechRecognizerRequest::setPunctuationPrediction(bool value) {
    _recognizerParam->setPunctuationPrediction(value);

    return 0;
}

int SpeechRecognizerRequest::setInverseTextNormalization(bool value) {
    _recognizerParam->setTextNormalization(value);

    return 0;
}

int SpeechRecognizerRequest::setEnableVoiceDetection(bool value) {
    _recognizerParam->setEnableVoiceDetection(value);

    return 0;
}

int SpeechRecognizerRequest::setMaxStartSilence(int value) {
    _recognizerParam->setMaxStartSilence(value);

    return 0;
}

int SpeechRecognizerRequest::setMaxEndSilence(int value) {
    _recognizerParam->setMaxEndSilence(value);

    return 0;
}

int SpeechRecognizerRequest::AppendHttpHeaderParam(const char* key, const char* value) {
    return _recognizerParam->AppendHttpHeader(key, value);
}

int SpeechRecognizerRequest::setCustomizationId(const char * value) {
    return _recognizerParam->setCustomizationId(value);
}

int SpeechRecognizerRequest::setVocabularyId(const char * value) {
    return _recognizerParam->setVocabularyId(value);
}

int SpeechRecognizerRequest::setTimeout(int value) {
    _recognizerParam->setTimeout(value);

    return 0;
}

int SpeechRecognizerRequest::setOutputFormat(const char* value) {
    INPUT_PARAM_STRING_CHECK(value);
    _recognizerParam->setOutputFormat(value);

    return 0;
}

void SpeechRecognizerRequest::setOnTaskFailed(NlsCallbackMethod event, void* param) {
    _callback->setOnTaskFailed(event, param);
}

void SpeechRecognizerRequest::setOnRecognitionStarted(NlsCallbackMethod event, void* param) {
    _callback->setOnRecognitionStarted(event, param);
}

void SpeechRecognizerRequest::setOnRecognitionCompleted(NlsCallbackMethod event, void* param) {
    _callback->setOnRecognitionCompleted(event, param);
}

void SpeechRecognizerRequest::setOnRecognitionResultChanged(NlsCallbackMethod event, void* param) {
    _callback->setOnRecognitionResultChanged(event, param);
}

void SpeechRecognizerRequest::setOnChannelClosed(NlsCallbackMethod event, void* param) {
    _callback->setOnChannelClosed(event, param);
}

}
