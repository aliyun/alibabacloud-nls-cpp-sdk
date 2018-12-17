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

#include "Config.h"
#include "iNlsRequestParam.h"
#if defined(_WIN32)
#include <objbase.h>
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CFUUID.h>
#else
#include "uuid/uuid.h"
#endif // _WIN32

#include "log.h"
#include "nlsRequestParamInfo.h"

using std::string;
using namespace Json;

namespace AlibabaNls {

using namespace util;

#if defined(__ANDROID__)
    const char g_sdk_name[] = "nls-sdk-android";
#elif defined(_WIN32)
    const char g_sdk_name[] = "nls-sdk-windows";
#elif defined(__APPLE__)
    const char g_sdk_name[] = "nls-sdk-ios";
#elif defined(__linux__)
    const char g_sdk_name[] = "nls-sdk-linux";
#endif

const char g_sdk_language[] = "C++";
const char g_sdk_version[] = NLS_SDK_VERSION_STR;

string random_uuid() {
    char uuidBuff[48] = {0};

#if defined(_WIN32)
    GUID guid;
    CoCreateGuid(&guid);
    _snprintf_s(uuidBuff, sizeof(uuidBuff), "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
                guid.Data1,
                guid.Data2,
                guid.Data3,
                guid.Data4[0],
                guid.Data4[1],
                guid.Data4[2],
                guid.Data4[3],
                guid.Data4[4],
                guid.Data4[5],
                guid.Data4[6],
                guid.Data4[7]);
#elif defined(__APPLE__)
    CFUUIDRef newId = CFUUIDCreate(NULL);
    CFUUIDBytes bytes = CFUUIDGetUUIDBytes(newId);
    CFRelease(newId);

    _ssnprintf(uuidBuff, 48, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", bytes.byte0,
               bytes.byte1,
               bytes.byte2,
               bytes.byte3,
               bytes.byte4,
               bytes.byte5,
               bytes.byte6,
               bytes.byte7,
               bytes.byte8,
               bytes.byte9,
               bytes.byte10,
               bytes.byte11,
               bytes.byte12,
               bytes.byte13,
               bytes.byte14,
               bytes.byte15);

#else
    uuid_t uuid;
    char tmp[48] = {0};
    uuid_generate(uuid);
    uuid_unparse(uuid, tmp);

    int i = 0, j = 0;
    while(tmp[i]) {
        if (tmp[i] != '-') {
            uuidBuff[j++] = tmp[i];
        }
        i ++;
    }

#endif

    return uuidBuff;
}

INlsRequestParam::INlsRequestParam(RequestMode mode) : _mode(mode),
                                                       _payload(Json::objectValue) {
	_customParam.clear();
	_timeout = -1;
	_url = "wss://nls-gateway.cn-shanghai.aliyuncs.com/ws/v1";
	_token = "";

    _task_id = random_uuid().c_str();

	_context[D_SDK_CLIENT] = getSdkInfo();

#if defined(_WIN32)
    _outputFormat = D_DEFAULT_VALUE_ENCODE_GBK;
#else
    _outputFormat = D_DEFAULT_VALUE_ENCODE_UTF8;
#endif

    _payload[D_FORMAT] = D_DEFAULT_VALUE_AUDIO_ENCODE;
    _payload[D_SAMPLE_RATE] = D_DEFAULT_VALUE_SAMPLE_RATE;

    _requestType = SpeechNormal;
}

INlsRequestParam::~INlsRequestParam() {

}

int INlsRequestParam::setPayloadParam(const char * key, Json::Value value) {
    _payload[key] = value[key];

    return 0;
}

int INlsRequestParam::setContextParam(const char* key, Json::Value value) {

    _context[key] = value[key];

    return 0;
}

int INlsRequestParam::setToken(const char* token) {
	this->_token = token;
	return 0;
}

int INlsRequestParam::setUrl(const char* url) {
    this->_url = url;
    return 0;
}

int INlsRequestParam::setAppKey(const char* appkey) {
	_header[D_APP_KEY] = appkey;
    return 0;
}

int INlsRequestParam::setFormat(const char* format) {
	_payload[D_FORMAT] = format;
    return 0;
}

int INlsRequestParam::setSampleRate(int sampleRate) {
	_payload[D_SAMPLE_RATE] = sampleRate;
    return 0;
}

int INlsRequestParam::setSentenceDetection(bool value) {

    _payload[D_SR_SENTENCE_DETECTION] = value;

    return 0;
}

int INlsRequestParam::setIntermediateResult(bool value) {
    _payload[D_SR_INTERMEDIATE_RESULT] = value;

    return 0;
}

int INlsRequestParam::setPunctuationPrediction(bool value) {
    _payload[D_SR_PUNCTUATION_PREDICTION] = value;

    return 0;
}

int INlsRequestParam::setTextNormalization(bool value) {
    _payload[D_SR_TEXT_NORMALIZATION] = value;

    return 0;
}

int INlsRequestParam::setTimeout(int timeout) {
	_timeout = timeout;
	return 0;
}

int INlsRequestParam::setOutputFormat(const char* outputFormat) {
	_outputFormat = outputFormat;
	return 0;
}

Json::Value INlsRequestParam::getSdkInfo() {
    Json::Value sdkInfo;

    sdkInfo[D_SDK_NAME] = g_sdk_name;
    sdkInfo[D_SDK_VERSION] = g_sdk_version;
    sdkInfo[D_SDK_LANGUAGE] = g_sdk_language;

    return sdkInfo;
}

const string INlsRequestParam::getStartCommand() {
    Json::Value root;
    Json::FastWriter writer;

    _header[D_TASK_ID] = this->_task_id;
    _header[D_MESSAGE_ID] = random_uuid().c_str();

    root[D_HEADER] = _header;
    root[D_PAYLOAD] = _payload;
    root[D_CONTEXT] = _context;

    string startCommand = writer.write(root);
    LOG_INFO("StartCommand: %s", startCommand.c_str());

    return startCommand;
}

const string INlsRequestParam::getStopCommand() {
    Json::Value root;
    Json::FastWriter writer;

    _header[D_TASK_ID] = this->_task_id;
    _header[D_MESSAGE_ID] = random_uuid().c_str();

    root[D_HEADER] = _header;
    root[D_CONTEXT] = _context;

    string stopCommand = writer.write(root);
    LOG_INFO("StopCommand: %s", stopCommand.c_str());

    return stopCommand;
}

const string INlsRequestParam::getExecuteDialog() {
    return "";
}

void INlsRequestParam::setNlsRequestType(NlsRequestType requestType) {
    _requestType = requestType;
}

}
