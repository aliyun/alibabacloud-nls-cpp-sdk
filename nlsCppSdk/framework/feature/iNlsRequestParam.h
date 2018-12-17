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

#ifndef NLS_SDK_REQUEST_PARAM_H
#define NLS_SDK_REQUEST_PARAM_H

#include <string>
#include "dataStruct.h"
#include "json/json.h"

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

namespace AlibabaNls {

enum NlsRequestType {
    SpeechNormal = 0, /**拥有正常start, sendAudio, stop调用流程的语音功能**/
    SpeechExecuteDialog,
    SpeechSynthesizer
};

std::string random_uuid();

class INlsRequestParam {
public:
	INlsRequestParam(util::RequestMode mode);
	virtual ~INlsRequestParam() = 0;

    virtual const std::string getStartCommand();
    virtual const std::string getStopCommand();
    virtual const std::string getExecuteDialog();

    virtual int setPayloadParam(const char* key, Json::Value value);
	virtual int setContextParam(const char* key, Json::Value value);
	virtual int setToken(const char* token);
	virtual int setUrl(const char* url);
    virtual int setAppKey(const char* appKey);
    virtual int setFormat(const char* format);
    virtual int setSampleRate(int sampleRate);
	virtual int setTimeout(int timeout);
	virtual int setOutputFormat(const char* outputFormat);

    virtual int setIntermediateResult(bool value);
    virtual int setPunctuationPrediction(bool value);
    virtual int setTextNormalization(bool value);
    virtual int setSentenceDetection(bool value);

    virtual Json::Value getSdkInfo();

    virtual void setNlsRequestType(NlsRequestType requestType);

    int _timeout;
    NlsRequestType _requestType;

	std::string _url;
	std::string _outputFormat;
	std::string _token;

	util::RequestMode _mode;

	std::string _task_id;

	Json::Value _header;
	Json::Value _payload;
	Json::Value _context;
	Json::Value _customParam;
};

}

#endif
