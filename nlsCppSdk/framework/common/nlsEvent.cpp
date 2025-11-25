/*
 * Copyright 2021 Alibaba Group Holding Limited
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
#include "nlsEventInner.h"

#include <stdlib.h>
#include <string.h>

#include <sstream>

#include "json/json.h"
#include "nlog.h"

namespace AlibabaNls {

NlsEvent::NlsEvent()
    : _statusCode(0),
      _msg(""),
      _msgType(TaskFailed),
      _taskId(""),
      _result(""),
      _displayText(""),
      _spokenText(""),
      _sentenceTimeOutStatus(0),
      _sentenceIndex(0),
      _sentenceTime(0),
      _sentenceBeginTime(0),
      _sentenceConfidence(0.0),
      _wakeWordAccepted(false),
      _wakeWordKnown(false),
      _wakeWordUserId(""),
      _wakeWordGender(0),
      _binaryDataInChar(NULL),
      _binaryDataSize(0),
      _stashResultSentenceId(0),
      _stashResultBeginTime(0),
      _stashResultText(""),
      _stashResultCurrentTime(0),
      _usage(0),
      _nlsType(TypeNone),
      _serviceProtocol(WsServiceProtocolNls) {}

NlsEvent::NlsEvent(NlsType nlsType, NlsServiceProtocol serviceProtocol)
    : _statusCode(0),
      _msg(""),
      _msgType(TaskFailed),
      _taskId(""),
      _result(""),
      _displayText(""),
      _spokenText(""),
      _sentenceTimeOutStatus(0),
      _sentenceIndex(0),
      _sentenceTime(0),
      _sentenceBeginTime(0),
      _sentenceConfidence(0.0),
      _wakeWordAccepted(false),
      _wakeWordKnown(false),
      _wakeWordUserId(""),
      _wakeWordGender(0),
      _binaryDataInChar(NULL),
      _binaryDataSize(0),
      _stashResultSentenceId(0),
      _stashResultBeginTime(0),
      _stashResultText(""),
      _stashResultCurrentTime(0),
      _usage(0),
      _nlsType(nlsType),
      _serviceProtocol(serviceProtocol) {}

NlsEvent::NlsEvent(const NlsEvent& ne) {
  this->_statusCode = ne._statusCode;
  this->_taskId = ne._taskId;

  this->_result = ne._result;

  this->_sentenceIndex = ne._sentenceIndex;
  this->_sentenceTime = ne._sentenceTime;
  this->_sentenceTimeOutStatus = ne._sentenceTimeOutStatus;

  this->_msg = ne._msg;
  this->_msgType = ne._msgType;
  this->_binaryData = ne._binaryData;
  this->_binaryDataInChar = ne._binaryDataInChar;
  this->_binaryDataSize = ne._binaryDataSize;

  this->_sentenceBeginTime = ne._sentenceBeginTime;
  this->_sentenceConfidence = ne._sentenceConfidence;
  this->_sentenceWordsList = ne._sentenceWordsList;

  this->_stashResultSentenceId = ne._stashResultSentenceId;
  this->_stashResultBeginTime = ne._stashResultBeginTime;
  this->_stashResultCurrentTime = ne._stashResultCurrentTime;
  this->_stashResultText = ne._stashResultText;

  this->_wakeWordAccepted = false;
  this->_wakeWordKnown = false;
  this->_wakeWordGender = 0;

  this->_usage = ne._usage;

  this->_nlsType = ne._nlsType;
  this->_serviceProtocol = ne._serviceProtocol;
}

NlsEvent::NlsEvent(const char* msg, int code, EventType type,
                   const std::string& taskId, NlsType nlsType,
                   NlsServiceProtocol serviceProtocol)
    : _statusCode(code),
      _msg(msg),
      _msgType(type),
      _taskId(taskId),
      _result(""),
      _displayText(""),
      _spokenText(""),
      _sentenceTimeOutStatus(0),
      _sentenceIndex(0),
      _sentenceTime(0),
      _sentenceBeginTime(0),
      _sentenceConfidence(0.0),
      _wakeWordAccepted(false),
      _wakeWordKnown(false),
      _wakeWordUserId(""),
      _wakeWordGender(0),
      _binaryDataInChar(NULL),
      _binaryDataSize(0),
      _stashResultSentenceId(0),
      _stashResultBeginTime(0),
      _stashResultText(""),
      _stashResultCurrentTime(0),
      _usage(0),
      _nlsType(nlsType),
      _serviceProtocol(serviceProtocol) {}

NlsEvent::NlsEvent(const std::string& msg, NlsType nlsType,
                   NlsServiceProtocol serviceProtocol)
    : _msg(msg),
      _statusCode(Success),
      _msgType(Message),
      _taskId(""),
      _result(""),
      _displayText(""),
      _spokenText(""),
      _sentenceTimeOutStatus(0),
      _sentenceIndex(0),
      _sentenceTime(0),
      _sentenceBeginTime(0),
      _sentenceConfidence(0.0),
      _wakeWordAccepted(false),
      _wakeWordKnown(false),
      _wakeWordUserId(""),
      _wakeWordGender(0),
      _binaryDataInChar(NULL),
      _binaryDataSize(0),
      _stashResultSentenceId(0),
      _stashResultBeginTime(0),
      _stashResultText(""),
      _stashResultCurrentTime(0),
      _usage(0),
      _nlsType(nlsType),
      _serviceProtocol(serviceProtocol) {}

NlsEvent::NlsEvent(const std::vector<unsigned char>& data, int code,
                   EventType type, const std::string& taskId, NlsType nlsType,
                   NlsServiceProtocol serviceProtocol)
    : _statusCode(code),
      _msgType(type),
      _taskId(taskId),
      _binaryData(data),
      _msg(""),
      _result(""),
      _displayText(""),
      _spokenText(""),
      _sentenceTimeOutStatus(0),
      _sentenceIndex(0),
      _sentenceTime(0),
      _sentenceBeginTime(0),
      _sentenceConfidence(0.0),
      _wakeWordAccepted(false),
      _wakeWordKnown(false),
      _wakeWordUserId(""),
      _wakeWordGender(0),
      _binaryDataInChar(NULL),
      _binaryDataSize(0),
      _stashResultSentenceId(0),
      _stashResultBeginTime(0),
      _stashResultText(""),
      _stashResultCurrentTime(0),
      _usage(0),
      _nlsType(nlsType),
      _serviceProtocol(serviceProtocol) {
  // LOG_DEBUG("Binary data event:%d.", data.size());
}

NlsEvent::NlsEvent(unsigned char* data, int dataBytes, int code, EventType type,
                   const std::string& taskId, NlsType nlsType,
                   NlsServiceProtocol serviceProtocol)
    : _statusCode(code),
      _msgType(type),
      _taskId(taskId),
      _binaryData(data, data + dataBytes),
      _msg(""),
      _result(""),
      _displayText(""),
      _spokenText(""),
      _sentenceTimeOutStatus(0),
      _sentenceIndex(0),
      _sentenceTime(0),
      _sentenceBeginTime(0),
      _sentenceConfidence(0.0),
      _wakeWordAccepted(false),
      _wakeWordKnown(false),
      _wakeWordUserId(""),
      _wakeWordGender(0),
      _binaryDataInChar(NULL),
      _binaryDataSize(0),
      _stashResultSentenceId(0),
      _stashResultBeginTime(0),
      _stashResultText(""),
      _stashResultCurrentTime(0),
      _usage(0),
      _nlsType(nlsType),
      _serviceProtocol(serviceProtocol) {}

NlsEvent::~NlsEvent() {
  if (_binaryDataInChar) {
    free(_binaryDataInChar);
    _binaryDataInChar = NULL;
    _binaryDataSize = 0;
  }
}

int NlsEvent::parseJsonMsg(bool ignore) {
  if (_msg.empty()) {
    return -(NlsEventMsgEmpty);
  }

  NlsEventInner eventInner;
  int ret = eventInner.parseJsonMsg(ignore);
  if (ret == Success) {
    ret = eventInner.transferEvent(this);
  }
  return ret;
}

int NlsEvent::getStatusCode() { return _statusCode; }

const char* NlsEvent::getAllResponse() {
  if (this->getMsgType() == Binary) {
    // LOG_DEBUG("this is Binary data.");
  }
  return this->_msg.c_str();
}

const char* NlsEvent::getErrorMessage() {
  if (_msgType != TaskFailed) {
    LOG_DEBUG("this msg is not error msg.");
    return "";
  }
  return this->_msg.c_str();
}

NlsEvent::EventType NlsEvent::getMsgType() { return _msgType; }

std::string NlsEvent::getMsgTypeString(int type) {
  std::string ret_str = "Unknown";
  NlsEvent::EventType msg_type =
      (type >= 0) ? (NlsEvent::EventType)type : _msgType;

  switch (msg_type) {
    case TaskFailed:
      ret_str.assign("TaskFailed");
      break;
    case RecognitionStarted:
      ret_str.assign("RecognitionStarted");
      break;
    case RecognitionCompleted:
      ret_str.assign("RecognitionCompleted");
      break;
    case RecognitionResultChanged:
      ret_str.assign("RecognitionResultChanged");
      break;
    case WakeWordVerificationCompleted:
      ret_str.assign("WakeWordVerificationCompleted");
      break;
    case TranscriptionStarted:
      ret_str.assign("TranscriptionStarted");
      break;
    case SentenceBegin:
      ret_str.assign("SentenceBegin");
      break;
    case TranscriptionResultChanged:
      ret_str.assign("TranscriptionResultChanged");
      break;
    case SentenceEnd:
      ret_str.assign("SentenceEnd");
      break;
    case SentenceSemantics:
      ret_str.assign("SentenceSemantics");
      break;
    case TranscriptionCompleted:
      ret_str.assign("TranscriptionCompleted");
      break;
    case SynthesisStarted:
      ret_str.assign("SynthesisStarted");
      break;
    case SynthesisCompleted:
      ret_str.assign("SynthesisCompleted");
      break;
    case Binary:
      ret_str.assign("Binary");
      break;
    case MetaInfo:
      ret_str.assign("MetaInfo");
      break;
    case DialogResultGenerated:
      ret_str.assign("DialogResultGenerated");
      break;
    case Close:
      ret_str.assign("Close");
      break;
    case Message:
      ret_str.assign("Message");
      break;
    case SentenceSynthesis:
      ret_str.assign("SentenceSynthesis");
      break;
  }

  return ret_str;
}

const char* NlsEvent::getTaskId() { return _taskId.c_str(); }

const char* NlsEvent::getDisplayText() { return _displayText.c_str(); }

const char* NlsEvent::getSpokenText() { return _spokenText.c_str(); }

const char* NlsEvent::getResult() {
  if (_msgType != RecognitionResultChanged &&
      _msgType != RecognitionCompleted &&
      _msgType != TranscriptionResultChanged && _msgType != SentenceEnd) {
    return NULL;
  }
  return _result.c_str();
}

int NlsEvent::getSentenceIndex() {
  if (_msgType != SentenceBegin && _msgType != SentenceEnd &&
      _msgType != TranscriptionResultChanged) {
    return -(InvalidNlsEventMsgType);
  }
  return _sentenceIndex;
}

int NlsEvent::getSentenceTime() {
  if (_msgType != SentenceBegin && _msgType != SentenceEnd &&
      _msgType != TranscriptionResultChanged) {
    return -(InvalidNlsEventMsgType);
  }
  return _sentenceTime;
}

int NlsEvent::getSentenceBeginTime() {
  if (_msgType != SentenceEnd) {
    return -(InvalidNlsEventMsgType);
  }
  return _sentenceBeginTime;
}

double NlsEvent::getSentenceConfidence() {
  if (_msgType != SentenceEnd) {
    return -(InvalidNlsEventMsgType);
  }
  return _sentenceConfidence;
}

int NlsEvent::getSentenceTimeOutStatus() {
  if (_msgType != SentenceEnd) {
    return -(InvalidNlsEventMsgType);
  }
  return _sentenceTimeOutStatus;
}

std::list<WordInfomation> NlsEvent::getSentenceWordsList() {
  std::list<WordInfomation> tmpList;
  if (_msgType != SentenceEnd) {
    return tmpList;
  }
  return _sentenceWordsList;
}

std::vector<unsigned char> NlsEvent::getBinaryData() {
  if (getMsgType() == Binary) {
    return _binaryData;
  } else {
    LOG_WARN("this hasn't Binary data.");
    return _binaryData;
  }
}

unsigned char* NlsEvent::getBinaryDataInChar() {
  if (getMsgType() == Binary) {
    if (_binaryDataInChar) {
      free(_binaryDataInChar);
    }
    _binaryDataSize = _binaryData.size();
    _binaryDataInChar = (unsigned char*)malloc(_binaryDataSize);
    memcpy(_binaryDataInChar, _binaryData.data(), _binaryDataSize);
    return _binaryDataInChar;
  } else {
    LOG_WARN("this hasn't Binary data.");
    return NULL;
  }
}

unsigned int NlsEvent::getBinaryDataSize() {
  if (getMsgType() == Binary) {
    return _binaryData.size();
  } else {
    return 0;
  }
}

bool NlsEvent::getWakeWordAccepted() {
  if (_msgType != WakeWordVerificationCompleted) {
    return false;
  }
  return _wakeWordAccepted;
}

int NlsEvent::getStashResultSentenceId() {
  if (_msgType != SentenceEnd) {
    return -(InvalidNlsEventMsgType);
  }
  return _stashResultSentenceId;
}

int NlsEvent::getStashResultBeginTime() {
  if (_msgType != SentenceEnd) {
    return -(InvalidNlsEventMsgType);
  }
  return _stashResultBeginTime;
}

int NlsEvent::getStashResultCurrentTime() {
  if (_msgType != SentenceEnd) {
    return -(InvalidNlsEventMsgType);
  }
  return _stashResultCurrentTime;
}

const char* NlsEvent::getStashResultText() {
  if (_msgType != SentenceEnd) {
    return NULL;
  }
  return _stashResultText.c_str();
}

int NlsEvent::getUsage() { return _usage; }

}  // namespace AlibabaNls
