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

#ifdef _WIN32
#include <objbase.h>
#include <windows.h>
#else
#include "thirdparty/libuuid/uuid.h"
#endif // _WIN32

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include "nlsRequestParamInfo.h"
#include "iNlsRequestParam.h"
#include "util/log.h"

using std::string;
using std::make_pair;
using std::ifstream;
using namespace Json;
using namespace util;

const char g_sdk_name[] = "nls-sdk-cpp";
const char g_sdk_language[] = "C++";
const char g_sdk_version[] = "2.1.4";

string random_uuid() {
	char str[48] = {0};

#ifdef _WIN32
	GUID guid;
	CoCreateGuid(&guid);
	_snprintf_s(str, sizeof(str), "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
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
#else
	uuid_t uuid;
char tmp[48] = {0};
	uuid_generate(uuid);
	uuid_unparse(uuid, tmp);

    int i = 0, j = 0;
    while(tmp[i]) {
        if (tmp[i] != '-') {
            str[j++] = tmp[i];
        }
        i ++;
    }

#endif

	return str;
}

INlsRequestParam::INlsRequestParam(RequestMode mode) :
	_mode(mode), _payload(Json::objectValue) {
	_customParam.clear();
	_timeout = -1;
	_url = "";
	_token = "";

    _task_id = random_uuid().c_str();

	_context[D_SDK_CLIENT] = getSdkInfo();
}

INlsRequestParam::~INlsRequestParam() {

}

int INlsRequestParam::setContextParam(const char* key, const char* value) {
    if ((NULL == key) || (NULL == value)) {
        LOG_ERROR("key or value is invalid.");

        return -1;
    }

	_customParam[key] = value;

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

int INlsRequestParam::setOutputFormat(const char* outputFormat) {
	_outputFormat = outputFormat;
	return 0;
}

const string INlsRequestParam::getMidStopCommand() {
	return "{}";
}

Json::Value INlsRequestParam::getSdkInfo() {
    Json::Value sdkInfo;

    sdkInfo[D_SDK_NAME] = g_sdk_name;
    sdkInfo[D_SDK_VERSION] = g_sdk_version;
    sdkInfo[D_SDK_LANGUAGE] = g_sdk_language;

    return sdkInfo;
}

int INlsRequestParam::generateRequestFromConfig(const char* config) {

    if (NULL == config) {
        return -1;
    }

    ifstream in(config);
    if (!in) {
        LOG_ERROR(" file: %s is not exist.", config);
        return -1;
    }

    while (!in.eof()) {
        string data = "";
        getline(in, data);
        if (data.size() == 0 || (data.size()> 0 && data[0] == '#')) {
            continue;
        }

        string key = "";
        string value = "";
        int index = data.find(":");
        key = data.substr(0, index);
        if (index < (data.length() -1)) {
            value = data.substr(index + 1, data.length() - index - 1);
        }

        if (key.empty() || value.empty()) {
            LOG_WARN("The content of %s is abnormal.", config);
        } else {

            LOG_INFO("Param [%s:%s].", key.c_str(), value.c_str());

            if (0 == strncasecmp(D_URL, key.c_str(), key.length())) {
                INlsRequestParam::setUrl(value.c_str());
            } else if (0 == strncasecmp(D_FORMAT, key.c_str(), key.length())) {
                INlsRequestParam::setFormat(value.c_str());
            } else if (0 == strncasecmp(D_APP_KEY, key.c_str(), key.length())) {
                INlsRequestParam::setAppKey(value.c_str());
            } else if (0 == strncasecmp(D_TOKEN, key.c_str(), key.length())) {
                INlsRequestParam::setToken(value.c_str());
            } else if (0 == strncasecmp(D_SAMPLE_RATE, key.c_str(), key.length())) {
                INlsRequestParam::setSampleRate(atoi(value.c_str()));
            } else {
                speechParam(key, value);
            }
        }
    }

    in.close();

    return 0;
}
