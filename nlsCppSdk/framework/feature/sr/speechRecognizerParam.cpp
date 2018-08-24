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
#include "speechRecognizerParam.h"
#include "util/log.h"

using namespace util;
using namespace Json;
using std::string;
using std::map;

#define D_CMD_START_RECOGNITION "StartRecognition"
#define D_CMD_STOP_RECOGNITION "StopRecognition"
#define D_NAMESPACE_RECOGNITION "SpeechRecognizer"

SpeechRecognizerParam::SpeechRecognizerParam() : INlsRequestParam(SR) {

#if defined(_WIN32)
    _outputFormat = D_DEFAULT_VALUE_ENCODE_GBK;
#else
    _outputFormat = D_DEFAULT_VALUE_ENCODE_UTF8;
#endif

	_header[D_NAMESPACE] = D_NAMESPACE_RECOGNITION;

	_payload[D_FORMAT] = D_DEFAULT_VALUE_AUDIO_ENCODE;
	_payload[D_SAMPLE_RATE] = D_DEFAULT_VALUE_SAMPLE_RATE;

}

SpeechRecognizerParam::~SpeechRecognizerParam() {

}

int SpeechRecognizerParam::setIntermediateResult(const char* value) {
    if (strcmp(value, D_DEFAULT_VALUE_BOOL_TRUE) == 0) {
		_payload[D_SR_INTERMEDIATE_RESULT] = true;
    } else if (strcmp(value, D_DEFAULT_VALUE_BOOL_FALSE) == 0) {
		_payload[D_SR_INTERMEDIATE_RESULT] = false;
    } else {
		LOG_ERROR("the parameter is error.");
        return -1;
    }

    return 0;
}

int SpeechRecognizerParam::setPunctuationPrediction(const char* value) {
    if (strcmp(value, D_DEFAULT_VALUE_BOOL_TRUE) == 0) {
		_payload[D_SR_PUNCTUATION_PREDICTION] = true;
    } else if (strcmp(value, D_DEFAULT_VALUE_BOOL_FALSE) == 0) {
		_payload[D_SR_PUNCTUATION_PREDICTION] = false;
    } else {
		LOG_ERROR("the parameter is error.");
        return -1;
    }

    return 0;
}

int SpeechRecognizerParam::setTextNormalization(const char* value) {
    if (strcmp(value, D_DEFAULT_VALUE_BOOL_TRUE) == 0) {
		_payload[D_SR_TEXT_NORMALIZATION] = true;
    } else if (strcmp(value, D_DEFAULT_VALUE_BOOL_FALSE) == 0) {
		_payload[D_SR_TEXT_NORMALIZATION] = false;
    } else {
        return -1;
    }

    return 0;
}

int SpeechRecognizerParam::setContextParam(const char* key, const char* value) {

    return INlsRequestParam::setContextParam(key, value);
}

const string SpeechRecognizerParam::getStartCommand() {
    Json::Value root;
    Json::FastWriter writer;

    _header[D_NAME] = D_CMD_START_RECOGNITION;
    _header[D_TASK_ID] = this->_task_id;
    _header[D_MESSAGE_ID] = random_uuid().c_str();

	if (!_customParam.empty()) {
		_context[D_CUSTOM_PARAM] = _customParam;
	}

    root["context"] = _context;
    root["header"] = _header;
    root["payload"] = _payload;

	string startCommand = writer.write(root);
	LOG_INFO("StartCommand: %s", startCommand.c_str());
	return startCommand;
}

const string SpeechRecognizerParam::getStopCommand() {
    Json::Value root;
    Json::FastWriter writer;

    _header[D_NAME] = D_CMD_STOP_RECOGNITION;
    _header[D_TASK_ID] = this->_task_id;
    _header[D_MESSAGE_ID] = random_uuid().c_str();

    root[D_HEADER] = _header;

	string stopCommand = writer.write(root);
	LOG_INFO("StopCommand: %s", stopCommand.c_str());
	return stopCommand;
}

int SpeechRecognizerParam::speechParam(string key, string value) {

    if (0 == strncasecmp(D_SR_INTERMEDIATE_RESULT, key.c_str(), key.length())) {
        setIntermediateResult(value.c_str());
    } else if (0 == strncasecmp(D_SR_PUNCTUATION_PREDICTION, key.c_str(), key.length())) {
        setPunctuationPrediction(value.c_str());
    } else if (0 == strncasecmp(D_SR_TEXT_NORMALIZATION, key.c_str(), key.length())) {
        setTextNormalization(value.c_str());
    } else {
        LOG_ERROR("%s is invalid.", key.c_str());
        return -1;
    }

    return 0;
}
