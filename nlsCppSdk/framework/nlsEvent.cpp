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
#include "util/log.h"

using std::string;
using std::vector;
using std::istringstream;
using std::ostringstream;
using namespace util;

NlsEvent::NlsEvent(NlsEvent& ne) {
	this->_errorcode = ne.getStausCode();

	if (ne.getMsgType() == TaskFailed)
	{
		this->_msg = ne.getErrorMessage();
	}
	else if (ne.getMsgType() == Binary)
	{
		this->_msg = "";
		this->_binaryData = ne.getBinaryData();
	}
	else {
		this->_msg = ne.getResponse();
	}
	this->_msgtype = ne.getMsgType();
}

NlsEvent::NlsEvent(string msg, int code, EventType type) :_msg(msg),
                                                          _errorcode(code),
                                                          _msgtype(type) {

}

NlsEvent::~NlsEvent() {

}

int NlsEvent::getStausCode() {
	return _errorcode;
}

const char* NlsEvent::getResponse() {
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

NlsEvent::NlsEvent(vector<unsigned char> data, int code, EventType type) : _binaryData(data),
                                                                           _errorcode(code),
                                                                           _msgtype(type) {
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
