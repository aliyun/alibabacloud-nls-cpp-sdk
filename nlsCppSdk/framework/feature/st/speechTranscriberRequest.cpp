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
#include "speechTranscriberSession.h"
#include "speechTranscriberRequest.h"
#include "speechTranscriberParam.h"
#include "speechTranscriberListener.h"
#include "nlsRequestParamInfo.h"

using std::string;
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

SpeechTranscriberRequest::SpeechTranscriberRequest(SpeechTranscriberCallback* cb,
                                                 const char* configPath) {
    _requestParam = new SpeechTranscriberParam();
    if (NULL != configPath) {
        _requestParam->generateRequestFromConfig(configPath);
    }

    _listener = new SpeechTranscriberListener(cb);

	_session = NULL;
}

SpeechTranscriberRequest::~SpeechTranscriberRequest() {
	if (_session) {
		delete _session;
		_session = NULL;
	}
	
	delete _requestParam;
    _requestParam = NULL;

    delete _listener;
    _listener = NULL;
}

int SpeechTranscriberRequest::start() {

    int ret = -1;
    string errorInfo;
    int errorCode = 0;
    int count = 10;

    do {
        try {
            if (!_session) {
                _session = new SpeechTranscriberSession(_requestParam);
                if (_session == NULL) {
                    LOG_ERROR("request start failed.")
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
    }while((-1 == ret) && ((count --) >= 0));

    if (-1 == ret) {
        errorInfo += ", retry finised.";
        NlsEvent* nlsevent = new NlsEvent(errorInfo, errorCode, NlsEvent::TaskFailed);
        _listener->handlerFrame(*nlsevent);
        delete nlsevent;
    }

    return ret;
}

int SpeechTranscriberRequest::stop() {
	if (!_session) {
		LOG_ERROR("Stop invoke error. Please check the order of execution.");
        return -1;
	}

    return _session->stop();
}

int SpeechTranscriberRequest::cancel() {
	if (!_session) {
		LOG_ERROR("Cancel invoke error. Please check the order of execution.");
        return -1;
	}
	
	return _session->shutdown();
}

int SpeechTranscriberRequest::sendAudio(char* data, size_t dataSzie, bool encoded ) {
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

    if (format == "pcm" || format == "opu") {
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

//    LOG_INFO("Send audio result: %d", ret);

    return ret ;
}

int SpeechTranscriberRequest::setToken(const char*token) {
    if (!token) {
        LOG_ERROR("It's null token.");
        return -1;
    }

    return this->_requestParam->setToken(token);
}

int SpeechTranscriberRequest::setUrl(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Url.");
        return -1;
    }

    return this->_requestParam->setUrl(value);
}

int SpeechTranscriberRequest::setAppKey(const char* value) {
    if (!value) {
        LOG_ERROR("It's null AppKey.");
        return -1;
    }

    return this->_requestParam->setAppKey(value);
}

int SpeechTranscriberRequest::setFormat(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Format.");
        return -1;
    }

    return this->_requestParam->setFormat(value);
}

int SpeechTranscriberRequest::setSampleRate(int value) {
    return this->_requestParam->setSampleRate(value);
}

int SpeechTranscriberRequest::setIntermediateResult(const char* value) {
    if (!value) {
        LOG_ERROR("It's null IntermediateResult.");
        return -1;
    }

    return this->_requestParam->setIntermediateResult(value);
}

int SpeechTranscriberRequest::setPunctuationPrediction(const char* value) {
    if (!value) {
        LOG_ERROR("It's null PunctuationPrediction.");
        return -1;
    }

    return this->_requestParam->setPunctuationPrediction(value);
}

int SpeechTranscriberRequest::setInverseTextNormalization(const char* value) {
    if (!value) {
        LOG_ERROR("It's null InverseTextNormalization.");
        return -1;
    }

    return this->_requestParam->setTextNormalization(value);
}

int SpeechTranscriberRequest::setOutputFormat(const char* value) {
	if (!value) {
		LOG_ERROR("It's null OutputFormat.");
		return -1;
	}

	return this->_requestParam->setOutputFormat(value);
}

int SpeechTranscriberRequest::setContextParam(const char *key, const char *value) {
    if (!value || !key) {
        LOG_ERROR("Key or Value is null.");
        return -1;
    }

    return this->_requestParam->setContextParam(key, value);
}
