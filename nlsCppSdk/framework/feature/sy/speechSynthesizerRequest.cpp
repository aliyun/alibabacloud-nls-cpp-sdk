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
#include "speechSynthesizerSession.h"
#include "speechSynthesizerRequest.h"
#include "speechSynthesizerParam.h"
#include "speechSynthesizerListener.h"

using std::string;
using namespace util;

SpeechSynthesizerCallback::SpeechSynthesizerCallback() {
    this->_onTaskFailed = NULL;
    this->_onSynthesisStarted = NULL;
    this->_onSynthesisCompleted = NULL;
    this->_onChannelClosed = NULL;
}

SpeechSynthesizerCallback::~SpeechSynthesizerCallback() {
    this->_onTaskFailed = NULL;
    this->_onSynthesisStarted = NULL;
    this->_onSynthesisCompleted = NULL;
    this->_onChannelClosed = NULL;
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

SpeechSynthesizerRequest::SpeechSynthesizerRequest(SpeechSynthesizerCallback* cb,
                                                 const char* configPath) {
    _requestParam = new SpeechSynthesizerParam();
    if (NULL != configPath) {
        _requestParam->generateRequestFromConfig(configPath);
    }

    _listener = new SpeechSynthesizerListener(cb);

    _session = NULL;
}

SpeechSynthesizerRequest::~SpeechSynthesizerRequest() {
    if (_session) {
        delete _session;
        _session = NULL;
    }

    delete _requestParam;
    _requestParam = NULL;

    delete _listener;
    _listener = NULL;
}

int SpeechSynthesizerRequest::start() {

    int ret = -1;
    string errorInfo;
    int errorCode = 0;
    int count = 10;

    do {
        try {
            if (!_session) {
                _session = new SpeechSynthesizerSession(_requestParam);
                if (_session == NULL) {
                    LOG_ERROR("request start failed.");
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

int SpeechSynthesizerRequest::stop() {
    if (!_session) {
        LOG_ERROR("Stop invoke failed. Please check the order of execution.");
        return -1;
    }

    return _session->stop();
}

int SpeechSynthesizerRequest::cancel() {
    if (!_session) {
        LOG_ERROR("Cancel invoke failed. Please check the order of execution.");
        return -1;
    }

    return _session->shutdown();
}

int SpeechSynthesizerRequest::setContextParam(const char *key, const char *value) {
    if (!value || !key) {
        LOG_ERROR("Key or Value is null.");
        return -1;
    }

    return this->_requestParam->setContextParam(key, value);
}

int SpeechSynthesizerRequest::setUrl(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Url.");
        return -1;
    }

    return this->_requestParam->setUrl(value);
}

int SpeechSynthesizerRequest::setAppKey(const char* value) {
    if (!value) {
        LOG_ERROR("It's null AppKey.");
        return -1;
    }

    return this->_requestParam->setAppKey(value);
}

int SpeechSynthesizerRequest::setToken(const char*token) {
    if (!token) {
        LOG_ERROR("It's null token.");
        return -1;
    }

    return this->_requestParam->setToken(token);
}

int SpeechSynthesizerRequest::setFormat(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Format.");
        return -1;
    }

    return this->_requestParam->setFormat(value);
}

int SpeechSynthesizerRequest::setSampleRate(int value) {
    return this->_requestParam->setSampleRate(value);
}

int SpeechSynthesizerRequest::setText(const char* value) {
    if (!value) {
        LOG_ERROR("It's null text.");
        return -1;
    }

    return this->_requestParam->setText(value);
}

int SpeechSynthesizerRequest::setMethod(int value) {
    return this->_requestParam->setMethod(value);
}

int SpeechSynthesizerRequest::setPitchRate(int value) {
    return this->_requestParam->setPitchRate(value);
}

int SpeechSynthesizerRequest::setSpeechRate(int value) {
    return this->_requestParam->setSpeechRate(value);
}

int SpeechSynthesizerRequest::setVolume(int value) {
    return this->_requestParam->setVolume(value);
}

int SpeechSynthesizerRequest::setVoice(const char* value) {
    if (!value) {
        LOG_ERROR("It's null voice.");
        return -1;
    }

    return this->_requestParam->setVoice(value);
}

int SpeechSynthesizerRequest::setOutputFormat(const char* value) {
	if (!value) {
		LOG_ERROR("It's null OutputFormat.");
		return -1;
	}

	return this->_requestParam->setOutputFormat(value);
}
