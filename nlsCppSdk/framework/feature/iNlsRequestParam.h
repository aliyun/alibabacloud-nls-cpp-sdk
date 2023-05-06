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
#include "json/json.h"

#ifdef _WIN32
#define strncasecmp _strnicmp
#endif

namespace AlibabaNls {

enum NlsRequestType {
    SpeechNormal = 0, /**拥有正常start, sendAudio, stop调用流程的语音功能**/
    SpeechExecuteDialog,
	SpeechWakeWordDialog,
	SpeechTextDialog,
    SpeechSynthesizer
};

//语音类型
enum NlsType {
	TypeAsr = 0,
	TypeRealTime,
	TypeTts,
	TypeDialog,
	TypeNone
};

class ConnectNode;
class INlsRequestParam {
public:
	INlsRequestParam(NlsType mode);
	virtual ~INlsRequestParam() = 0;

    inline void setPayloadParam(const char* key, Json::Value value) {
		_payload[key] = value[key];
	};

	inline void setContextParam(const char* key, Json::Value value) {
		_context[key] = value[key];
	};

	inline void setToken(const char* token) {
		this->_token = token;
	};

	inline void setUrl(const char* url) {
		this->_url = url;
	};

    void setAppKey(const char* appKey);
    void setFormat(const char* format);

    void setSampleRate(int sampleRate);

	inline void setTimeout(int timeout) {
		_timeout = timeout;
	};

	inline void setOutputFormat(const char* outputFormat) {
		_outputFormat = outputFormat;
	};

    void  setIntermediateResult(bool value);
	void setPunctuationPrediction(bool value);
    void  setTextNormalization(bool value);
    void  setSentenceDetection(bool value);

    inline void setNlsRequestType(NlsRequestType requestType) {
		_requestType = requestType;
	};

	virtual int setCustomizationId(const char * value);
	virtual int setVocabularyId(const char * value);

	Json::Value getSdkInfo();
	int setContextParam(const char* value);
	int setPayloadParam(const char* value);

	virtual const char* getStartCommand();
	virtual const char* getStopCommand();
	virtual const char* getControlCommand(const char* message);
	virtual const char* getExecuteDialog();
	virtual const char* getStopWakeWordCommand();

	int setEnableWakeWordVerification(bool value);

	bool _enableWakeWord;

    int _timeout;
	int _sampleRate;
    NlsRequestType _requestType;

	std::string _url;
	std::string _outputFormat;
	std::string _token;
	std::string _format;

	std::string _task_id;

	NlsType _mode;

	std::string _startCommand;
	std::string _controlCommand;
	std::string _stopCommand;

	Json::Value _header;
	Json::Value _payload;
	Json::Value _context;

	std::string getRandomUuid();

//	void* _cbParam;
//	void setCallbackParam(void* param, int size);

	int setSessionId(const char* sessionId);

	Json::Value _httpHeader;
	std::string _httpHeaderString;
	int AppendHttpHeader(const char* key, const char* value);

	std::string GetHttpHeader();
};

}

#endif
