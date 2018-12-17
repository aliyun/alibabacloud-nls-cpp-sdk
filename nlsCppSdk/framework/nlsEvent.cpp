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

#include "nlsEvent.h"
#include <sstream>
#include "log.h"
#include "exception.h"
#include "json/json.h"

using std::string;
using std::vector;
using std::istringstream;
using std::ostringstream;

namespace AlibabaNls {

using namespace util;

NlsEvent::NlsEvent(const NlsEvent& ne) {
	this->_statusCode = ne._statusCode;
	this->_taskId = ne._taskId;

	this->_result = ne._result;

	this->_sentenceIndex = ne._sentenceIndex;
	this->_sentenceTime = ne._sentenceTime;

	this->_msg = ne._msg;
	this->_msgtype = ne._msgtype;
	this->_binaryData = ne._binaryData;
	this->_sentenceBeginTime = ne._sentenceBeginTime;
	this->_sentenceConfidence = ne._sentenceConfidence;
}

NlsEvent::NlsEvent(string msg, int code, EventType type, string taskId)
	: _statusCode(code), _msg(msg), _msgtype(type), _taskId(taskId) {

}

NlsEvent::NlsEvent(string msg) : _msg(msg) {

}

NlsEvent::~NlsEvent() {

}

int NlsEvent::parseJsonMsg() {
	if (_msg.empty()) {
		return -1;
	}

	Json::Reader reader;
	Json::Value head, payload, root;

	if (!reader.parse(_msg, root)) {
		LOG_ERROR("_msg:%s", _msg.c_str());
		return -1;
	}

	// parse head
	if (!root["header"].isNull()) {
		head = root["header"];

		// name
		if (head["name"].isNull()) {
			return -1;
		}

		string name = head["name"].asCString();
		if (parseMsgType(name) == -1) {
			return -1;
		}

		// status
		if (!head["status"].isNull()) {
			_statusCode = head["status"].asInt();
		} else {
			return -1;
		}

		// task_id
		if (!head["task_id"].isNull()) {
			_taskId = head["task_id"].asCString();
		}
	} else {
		return -1;
	}

	// parse payload
	if (!root["payload"].isNull()) {
		payload = root["payload"];

		// result
		if (!payload["result"].isNull()) {
			_result = payload["result"].asCString();
		}

		// index
		if (!payload["index"].isNull()) {
			_sentenceIndex = payload["index"].asInt();
		}

		// time
		if (!payload["time"].isNull()) {
			_sentenceTime = payload["time"].asInt();
		}

		// begin_time
		if (!payload["begin_time"].isNull()) {
			_sentenceBeginTime = payload["begin_time"].asInt();
		}

		// confidence
		if (!payload["confidence"].isNull()) {
			_sentenceConfidence = payload["confidence"].asDouble();
		}

        // display_text
        if (!payload["display_text"].isNull()) {
            _displayText = payload["display_text"].asCString();
        }

        // spoken_text
        if (!payload["spoken_text"].isNull()) {
            _spokenText = payload["spoken_text"].asCString();
        }
	}

	return 0;
}

int NlsEvent::parseMsgType(std::string name) {
	if (name == "TaskFailed") {
		_msgtype = NlsEvent::TaskFailed;
	} else if (name == "RecognitionStarted") {
		_msgtype = NlsEvent::RecognitionStarted;
	} else if (name == "RecognitionCompleted") {
		_msgtype = NlsEvent::RecognitionCompleted;
	} else if (name == "RecognitionResultChanged") {
		_msgtype = NlsEvent::RecognitionResultChanged;
	} else if (name == "TranscriptionStarted") {
		_msgtype = NlsEvent::TranscriptionStarted;
	} else if (name == "SentenceBegin") {
		_msgtype = NlsEvent::SentenceBegin;
	} else if (name == "TranscriptionResultChanged") {
		_msgtype = NlsEvent::TranscriptionResultChanged;
	} else if (name == "SentenceEnd") {
		_msgtype = NlsEvent::SentenceEnd;
	} else if (name == "TranscriptionCompleted") {
		_msgtype = NlsEvent::TranscriptionCompleted;
	} else if (name == "SynthesisStarted") {
		_msgtype = NlsEvent::SynthesisStarted;
	} else if (name == "SynthesisCompleted") {
		_msgtype = NlsEvent::SynthesisCompleted;
	} else if (name == "DialogResultGenerated") {
        _msgtype = NlsEvent::DialogResultGenerated;
    } else {
		LOG_ERROR("EVENT: type is invalid. [%s].", _msg.c_str());
		return -1;
	}

	return 0;
}

int NlsEvent::getStausCode() {
	return _statusCode;
}

const char* NlsEvent::getAllResponse() {
	if (this->getMsgType() == Binary) {
		LOG_WARN("this is Binary data.");
	}
	return this->_msg.c_str();
}

const char* NlsEvent::getErrorMessage() {
	if (_msgtype != TaskFailed) {
		LOG_WARN("this msg is not error msg.");
		return string("").c_str();
	}
	return this->_msg.c_str();
}	  

NlsEvent::EventType NlsEvent::getMsgType() {
	return _msgtype;
}

const char* NlsEvent::getTaskId() {
	return _taskId.c_str();
}

const char* NlsEvent::getDisplayText() {
    return _displayText.c_str();
}

const char* NlsEvent::getSpokenText() {
    return _spokenText.c_str();
}

const char* NlsEvent::getResult() {
	if (_msgtype != RecognitionResultChanged &&
		_msgtype != RecognitionCompleted &&
		_msgtype != TranscriptionResultChanged &&
		_msgtype != SentenceEnd) {
		return string("").c_str();
	}

	return _result.c_str();
}

int NlsEvent::getSentenceIndex() {
	if (_msgtype != SentenceBegin &&
		_msgtype != SentenceEnd &&
		_msgtype != TranscriptionResultChanged) {
		return -1;
	}

	return _sentenceIndex;
}

int NlsEvent::getSentenceTime() {
	if (_msgtype != SentenceBegin &&
		_msgtype != SentenceEnd &&
		_msgtype != TranscriptionCompleted) {
		return -1;
	}
	return _sentenceTime;
}

int NlsEvent::getSentenceBeginTime() {
	if (_msgtype != SentenceEnd ) {
		return -1;
	}
	return _sentenceBeginTime;
}

double NlsEvent::getSentenceConfidence() {
	if (_msgtype != SentenceEnd ) {
		return -1;
	}
	return _sentenceConfidence;
}

NlsEvent::NlsEvent(vector<unsigned char> data, int code, EventType type, string taskId)
	: _statusCode(code), _msgtype(type), _taskId(taskId), _binaryData(data) {
	this->_msg = "";
}

vector<unsigned char> NlsEvent::getBinaryData() {
	if (this->getMsgType() == Binary) {
		return this->_binaryData;
	} else {
		LOG_WARN("this hasn't Binary data.");
		return _binaryData;
	}
}

}
