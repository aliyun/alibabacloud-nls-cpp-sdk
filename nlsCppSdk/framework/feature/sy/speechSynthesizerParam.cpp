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

#include <map>
#include <cstdlib>
#include <utility>
#include "nlsRequestParamInfo.h"
#include "speechSynthesizerParam.h"
#include "util/log.h"

using namespace util;
using namespace Json;
using std::string;
using std::map;

#define D_CMD_START_SYNTHESIZER "StartSynthesis"
#define D_NAMESPACE_SYNTHESIZER "SpeechSynthesizer"

SpeechSynthesizerParam::SpeechSynthesizerParam() : INlsRequestParam(SY) {

#if defined(_WIN32)
    _outputFormat = D_DEFAULT_VALUE_ENCODE_GBK;
#else
    _outputFormat = D_DEFAULT_VALUE_ENCODE_UTF8;
#endif

	_header[D_NAMESPACE] = D_NAMESPACE_SYNTHESIZER;

	_payload[D_FORMAT] = D_DEFAULT_VALUE_AUDIO_ENCODE;
	_payload[D_SAMPLE_RATE] = D_DEFAULT_VALUE_SAMPLE_RATE;
}

SpeechSynthesizerParam::~SpeechSynthesizerParam() {

}

int SpeechSynthesizerParam::setText(const char* value) {
    if (value == NULL) {
        LOG_ERROR("text is NULL.");
        return -1;
    }

	_payload[D_SY_TEXT] = value;

    return 0;
}

int SpeechSynthesizerParam::setVoice(const char* value) {
    if (value == NULL) {
        return -1;
    }

	_payload[D_SY_VOICE] = value;

    return 0;
}

int SpeechSynthesizerParam::setVolume(int value) {
	_payload[D_SY_VOLUME] = value;
    return 0;
}

int SpeechSynthesizerParam::setSpeechRate(int value) {
	_payload[D_SY_SPEECH_RATE] = value;
    return 0;
}

int SpeechSynthesizerParam::setPitchRate(int value) {
	_payload[D_SY_PITCH_RATE] = value;
    return 0;
}

int SpeechSynthesizerParam::setMethod(int value) {
	_payload[D_SY_METHOD] = value;
    return 0;
}

const string SpeechSynthesizerParam::getStartCommand() {
    Json::Value root;
    Json::FastWriter writer;

    _header[D_NAME] = D_CMD_START_SYNTHESIZER;
    _header[D_TASK_ID] = this->_task_id;
    _header[D_MESSAGE_ID] = random_uuid().c_str();
	
	if (!_customParam.empty()) {
		_context[D_CUSTOM_PARAM] = _customParam;
	}

    root["header"] = _header;
    root["payload"] = _payload;
    root["context"] = _context;

	string startCommand = writer.write(root);
	LOG_INFO("StartCommand: %s", startCommand.c_str());
	return startCommand;
}

const string SpeechSynthesizerParam::getStopCommand() {
    return "";
//    return "{}";
}

int SpeechSynthesizerParam::speechParam(string key, string value) {

    if (0 == strncasecmp(D_SY_TEXT, key.c_str(), key.length())) {
        setText(value.c_str());
    } else if (0 == strncasecmp(D_SY_VOICE, key.c_str(), key.length())) {
        setVoice(value.c_str());
    } else if (0 == strncasecmp(D_SY_VOLUME, key.c_str(), key.length())) {
        setVolume(atoi(value.c_str()));
    } else if (0 == strncasecmp(D_SY_SPEECH_RATE, key.c_str(), key.length())) {
        setSpeechRate(atoi(value.c_str()));
    } else if (0 == strncasecmp(D_SY_PITCH_RATE, key.c_str(), key.length())) {
        setPitchRate(atoi(value.c_str()));
    } else if (0 == strncasecmp(D_SY_METHOD, key.c_str(), key.length())) {
        setMethod(atoi(value.c_str()));
    }  else {
        LOG_ERROR("%s is invalid.", key.c_str());
        return -1;
    }

    return 0;
}

int SpeechSynthesizerParam::setContextParam(const char* key, const char* value) {

    return INlsRequestParam::setContextParam(key, value);

}
