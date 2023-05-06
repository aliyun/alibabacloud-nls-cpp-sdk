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

#include "speechTranscriberSyncRequest.h"
#include <string>
#include "log.h"
#include "nlsRequestParamInfo.h"
#include "speechTranscriberParam.h"
#include "speechTranscriberListener.h"

using std::string;

namespace AlibabaNls {

using namespace utility;

SpeechTranscriberSyncRequest::SpeechTranscriberSyncRequest() {
	_request = new SpeechTranscriberRequest(NULL);
}

SpeechTranscriberSyncRequest::~SpeechTranscriberSyncRequest() {
	if (_request != NULL) {
		delete _request;
		_request = NULL;
	}
}

int SpeechTranscriberSyncRequest::setToken(const char*token) {
    if (NULL == _request) {
        return -1;
    }

    return _request->setToken(token);
}

int SpeechTranscriberSyncRequest::setUrl(const char* value) {
    if (NULL == _request) {
        return -1;
    }

    return _request->setUrl(value);
}

int SpeechTranscriberSyncRequest::setAppKey(const char* value) {
    if (NULL == _request) {
        return -1;
    }

    return _request->setAppKey(value);
}

int SpeechTranscriberSyncRequest::setFormat(const char* value) {
    if (NULL == _request) {
        return -1;
    }

    return _request->setFormat(value);
}

int SpeechTranscriberSyncRequest::setSampleRate(int value) {
	if (NULL == _request) {
		return -1;
	}

    return _request->setSampleRate(value);
}

int SpeechTranscriberSyncRequest::setIntermediateResult(bool value) {
	if (NULL == _request) {
		return -1;
	}

    return _request->setIntermediateResult(value);
}

int SpeechTranscriberSyncRequest::setPunctuationPrediction(bool value) {
	if (NULL == _request) {
		return -1;
	}

    return _request->setPunctuationPrediction(value);
}

int SpeechTranscriberSyncRequest::setInverseTextNormalization(bool value) {
	if (NULL == _request) {
		return -1;
	}

	return _request->setInverseTextNormalization(value);
}

int SpeechTranscriberSyncRequest::setSemanticSentenceDetection(bool value) {
	if (NULL == _request) {
		return -1;
	}

	return _request->setSemanticSentenceDetection(value);
}

int SpeechTranscriberSyncRequest::setMaxSentenceSilence(int value) {
	if (NULL == _request) {
		return -1;
	}

    return _request->setMaxSentenceSilence(value);
}

int SpeechTranscriberSyncRequest::setTimeout(int value) {
	if (NULL == _request) {
		return -1;
	}

	return _request->setTimeout(value);
}

int SpeechTranscriberSyncRequest::setOutputFormat(const char* value) {
	if (NULL == _request || NULL == value) {
		return -1;
	}

	return _request->setOutputFormat(value);
}

int SpeechTranscriberSyncRequest::setContextParam(const char *value) {
	if (NULL == _request || NULL == value) {
		return -1;
	}

    return _request->setContextParam(value);
}

int SpeechTranscriberSyncRequest::sendSyncAudio(const char* data, int dataSize, AudioDataStatus status, bool encoded) {
	if (NULL == _request) {
		return -1;
	}

	int ret = -1;
	if (AUDIO_FIRST == status) {
		ret = _request->start();
		if (ret < 0) {
			return ret;
		}
	}

	int sentSize = 0;
	ret = _request->sendAudio(data, dataSize, encoded);
	sentSize = ret;

	if (ret < 0 || AUDIO_LAST == status) {
		if (_request->isStarted()) {
			ret = _request->stop();
			if (ret < 0) {
				return ret;
			}
		}
	}

	return sentSize;
}


int SpeechTranscriberSyncRequest::getTranscriberResult(std::queue<NlsEvent>* eventQueue) {
	if (NULL == _request) {
		return -1;
	}

	return _request->getTranscriberResult(eventQueue);
}

bool SpeechTranscriberSyncRequest::isStarted() {
	if (NULL == _request) {
		return false;
	}

	return _request->isStarted();
}

int SpeechTranscriberSyncRequest::setPayloadParam(const char* value) {
	if (NULL == _request) {
		return -1;
	}

	return _request->setPayloadParam(value);
}

}
