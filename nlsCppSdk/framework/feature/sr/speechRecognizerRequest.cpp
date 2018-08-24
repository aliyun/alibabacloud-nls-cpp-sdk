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
#include "util/log.h"
#include "speechRecognizerSession.h"
#include "speechRecognizerRequest.h"
#include "speechRecognizerParam.h"
#include "speechRecognizerListener.h"
#include "nlsRequestParamInfo.h"

using std::string;
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

SpeechRecognizerRequest::SpeechRecognizerRequest(SpeechRecognizerCallback* cb,
                                                 const char* configPath) {
    _requestParam = new SpeechRecognizerParam();
    if (NULL != configPath) {
        _requestParam->generateRequestFromConfig(configPath);
    }

    _listener = new SpeechRecognizerListener(cb);

	_session = NULL;
}

SpeechRecognizerRequest::~SpeechRecognizerRequest() {
	if (_session) {
		delete _session;
		_session = NULL;
	}
	
	delete _requestParam;
    _requestParam = NULL;

    delete _listener;
    _listener = NULL;
}

int SpeechRecognizerRequest::start() {

    int ret = -1;
    string errorInfo;
    int errorCode = 0;
    int count = 10;

    do {
        try {
            if (!_session) {
                _session = new SpeechRecognizerSession(_requestParam);
                if (_session == NULL) {
                    return -1;
                }
                _session->setHandler(_listener);
            }
            ret = _session->start();
            return ret;
        } catch (ExceptionWithString &e) {
            errorInfo = e.what();
            errorCode = e.getErrorcode();
//			if (NULL != _session) {
//				delete _session;
//				_session = NULL;
//			}
            LOG_ERROR("%s, begining retry...", e.what());
            ret = -1;
        }
    } while((-1 == ret) && ((count --) >= 0));

    if (-1 == ret) {
        errorInfo += ", retry finised.";
        NlsEvent* nlsevent = new NlsEvent(errorInfo, errorCode, NlsEvent::TaskFailed);
        _listener->handlerFrame(*nlsevent);
        delete nlsevent;
    }

    return ret;
}

int SpeechRecognizerRequest::stop() {
	if (!_session) {
		LOG_ERROR("Stop invoke error. Please check the order of execution.");
        return -1;
	}

    return _session->stop();
}

int SpeechRecognizerRequest::cancel() {
	if (!_session) {
		LOG_ERROR("Cancel invoke error. Please check the order of execution.");
        return -1;
	}
	
	return _session->shutdown();
}

int SpeechRecognizerRequest::sendAudio(char* data, size_t dataSzie, bool encoded ) {
    int ret = -1;
    string errorInfo;
    int errorCode = -1; //TODO getErrorCode
	string format = _requestParam->_payload[D_FORMAT].asString();

    if (!data) {
        LOG_ERROR("It's null data.");
        return -1;
    }

	if (!_session) {
		LOG_ERROR("SendAudio invoke error. Please check the order of execution.");
        return -1;
	}

    if (format == "pcm" || format == "opus" || format == "opu") {
        if(encoded) {
            ret = _session->sendOpusVoice((unsigned char *)data, dataSzie);
        } else {
            ret = _session->sendPcmVoice((unsigned char *)data, dataSzie);
        }

        if (-1 == ret) {
            errorInfo = "Send data failed.";
        }

    } else {
        errorInfo = "Format is not supported.";
        LOG_ERROR("Format is not supported.");
    }

    if (-1 == ret) {
		LOG_DEBUG("sendAudio failed, error info : %s", errorInfo.c_str());
        NlsEvent* nlsevent = new NlsEvent(errorInfo, errorCode, NlsEvent::TaskFailed);
        _listener->handlerFrame(*nlsevent);
        delete nlsevent;
        nlsevent = NULL;
    }

//	LOG_INFO("Send audio result: %d", ret);

    return ret ;
}

int SpeechRecognizerRequest::setContextParam(const char *key, const char *value) {
    if (!value || !key) {
        LOG_ERROR("Key or Value is null.");
        return -1;
    }

    return this->_requestParam->setContextParam(key, value);
}

int SpeechRecognizerRequest::setToken(const char*token) {
    if (!token) {
        LOG_ERROR("It's null token.");
        return -1;
    }

    return this->_requestParam->setToken(token);
}

int SpeechRecognizerRequest::setUrl(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Url.");
        return -1;
    }

    return this->_requestParam->setUrl(value);
}

int SpeechRecognizerRequest::setAppKey(const char* value) {
    if (!value) {
        LOG_ERROR("It's null AppKey.");
        return -1;
    }

    return this->_requestParam->setAppKey(value);
}

int SpeechRecognizerRequest::setFormat(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Format.");
        return -1;
    }

    return this->_requestParam->setFormat(value);
}

int SpeechRecognizerRequest::setSampleRate(int value) {
    return this->_requestParam->setSampleRate(value);
}

int SpeechRecognizerRequest::setIntermediateResult(const char* value) {
    if (!value) {
        LOG_ERROR("It's null IntermediateResult.");
        return -1;
    }

    return this->_requestParam->setIntermediateResult(value);
}

int SpeechRecognizerRequest::setPunctuationPrediction(const char* value) {
    if (!value) {
        LOG_ERROR("It's null PunctuationPrediction.");
        return -1;
    }

    return this->_requestParam->setPunctuationPrediction(value);
}

int SpeechRecognizerRequest::setInverseTextNormalization(const char* value) {
    if (!value) {
        LOG_ERROR("It's null InverseTextNormalization.");
        return -1;
    }

    return this->_requestParam->setTextNormalization(value);
}

int SpeechRecognizerRequest::setOutputFormat(const char* value) {
	if (!value) {
		LOG_ERROR("It's null OutputFormat.");
		return -1;
	}

	return this->_requestParam->setOutputFormat(value);
}
