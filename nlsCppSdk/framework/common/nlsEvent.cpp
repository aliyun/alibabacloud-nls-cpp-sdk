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

#include <sstream>
#include "nlsEvent.h"
#include "nlog.h"
#include "json/json.h"

namespace AlibabaNls {

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
  this->_sentenceBeginTime = ne._sentenceBeginTime;
  this->_sentenceConfidence = ne._sentenceConfidence;
  this->_sentenceWordsList = ne._sentenceWordsList;

  this->_stashResultSentenceId = ne._stashResultSentenceId;
  this->_stashResultBeginTime = ne._stashResultBeginTime;
  this->_stashResultCurrentTime = ne._stashResultCurrentTime;
  this->_stashResultText = ne._stashResultText;
}

NlsEvent::NlsEvent(
    const char * msg, int code, EventType type, std::string & taskId) :
    _statusCode(code), _msg(msg), _msgType(type), _taskId(taskId) {}

NlsEvent::NlsEvent(std::string & msg) : _msg(msg) {
  _statusCode = 0;
  _sentenceTimeOutStatus = 0;
  _sentenceIndex = 0;
  _sentenceTime = 0;
  _sentenceBeginTime = 0;
  _sentenceConfidence = 0.0;

  _stashResultSentenceId = 0;
  _stashResultBeginTime = 0;
  _stashResultCurrentTime = 0;

  _msgType = Message;
}

NlsEvent::~NlsEvent() {}

int NlsEvent::parseJsonMsg(bool ignore) {
  if (_msg.empty()) {
    return -(NlsEventMsgEmpty);
  }

  int retCode = Success;
  Json::Reader reader;
  Json::Value head(Json::objectValue);
  Json::Value payload(Json::objectValue);
  Json::Value root(Json::objectValue);
  Json::Value stashResult(Json::objectValue);

  if (!reader.parse(_msg, root)) {
    LOG_ERROR("_msg:%s", _msg.c_str());
    return -(JsonParseFailed);
  }

  // parse head
  if (!root["header"].isNull() && root["header"].isObject()) {
    head = root["header"];
    // name
    if (!head["name"].isNull() && head["name"].isString()) {
      std::string name = head["name"].asCString();
      retCode = parseMsgType(name);
      if (retCode < 0) {
        if (ignore == false) {
          if (retCode == -(InvalidNlsEventMsgType)) {
            LOG_ERROR("Event Msg Type is invalid: %s", _msg.c_str());
            return retCode;
          }
        }
      }
    }

    // status
    if (!head["status"].isNull() && head["status"].isInt()) {
      _statusCode = head["status"].asInt();
    } else {
      if (ignore == false) {
        return -(InvalidNlsEventMsgStatusCode);
      }
    }

    // task_id
    if (!head["task_id"].isNull() && head["task_id"].isString()) {
      _taskId = head["task_id"].asCString();
    }
  } else {
    if (!root["channelClosed"].isNull()) {
      _msgType = Close;
    } else if (!root["TaskFailed"].isNull()) {
      _msgType = TaskFailed;
    } else {
      if (ignore == false) {
        return -(InvalidNlsEventMsgHeader);
      }
    }
  }

  // parse payload
  if (_msgType != SynthesisCompleted && _msgType != MetaInfo) {
    if (!root["payload"].isNull() && root["payload"].isObject()) {
      payload = root["payload"];
      // result
      if (!payload["result"].isNull() && payload["result"].isString()) {
        _result = payload["result"].asCString();
      }

      // index
      if (!payload["index"].isNull() && payload["index"].isInt()) {
        _sentenceIndex = payload["index"].asInt();
      }

      // time
      if (!payload["time"].isNull() && payload["time"].isInt()) {
        _sentenceTime = payload["time"].asInt();
      }

      // begin_time
      if (!payload["begin_time"].isNull() && payload["begin_time"].isInt()) {
        _sentenceBeginTime = payload["begin_time"].asInt();
      }

      // confidence
      if (!payload["confidence"].isNull() && payload["confidence"].isDouble()) {
        _sentenceConfidence = payload["confidence"].asDouble();
      }

      // display_text
      if (!payload["display_text"].isNull() &&
          payload["display_text"].isString()) {
        _displayText = payload["display_text"].asCString();
      }

      // spoken_text
      if (!payload["spoken_text"].isNull() && payload["spoken_text"].isString()) {
        _spokenText = payload["spoken_text"].asCString();
      }

      // sentence timeOut status
      if (!payload["status"].isNull() && payload["status"].isInt()) {
        _sentenceTimeOutStatus = payload["status"].asInt();
      }

      // example: "words":[{"text":"一二三四","startTime":810,"endTime":2460}]
      if (!payload["words"].isNull() && payload["words"].isArray()) {
        Json::Value wordArray = payload["words"];
        int iSize = wordArray.size();
        WordInfomation wordInfo;

        for (int nIndex = 0; nIndex < iSize; nIndex++) {
          if (wordArray[nIndex].isMember("text") &&
              wordArray[nIndex]["text"].isString()) {
            wordInfo.text = wordArray[nIndex]["text"].asCString();
          }
          if (wordArray[nIndex].isMember("startTime") &&
              wordArray[nIndex]["startTime"].isInt()) {
            wordInfo.startTime = wordArray[nIndex]["startTime"].asInt();
          }
          if (wordArray[nIndex].isMember("endTime") &&
              wordArray[nIndex]["endTime"].isInt()) {
            wordInfo.endTime = wordArray[nIndex]["endTime"].asInt();
          }
          // LOG_DEBUG("List Push: %s %d %d",
          //     wordInfo.text.c_str(), wordInfo.startTime, wordInfo.endTime);

          _sentenceWordsList.push_back(wordInfo);
        }  // for
      }

      // WakeWordVerificationCompleted
      if (_msgType == NlsEvent::WakeWordVerificationCompleted) {
        if (!payload["accepted"].isNull() && payload["accepted"].isBool()) {
          _wakeWordAccepted = payload["accepted"].asBool();
        }
        if (!payload["known"].isNull() && payload["known"].isBool()) {
          _wakeWordKnown = payload["known"].asBool();
        }
        if (!payload["user_id"].isNull() && payload["user_id"].isString()) {
          _wakeWordUserId = payload["user_id"].asCString();
        }
        if (!payload["gender"].isNull() && payload["gender"].isInt()) {
          _wakeWordGender = payload["gender"].asInt();
        }
      }

      // stashResult
      if (_msgType == NlsEvent::SentenceEnd) {
        if (!payload["stash_result"].isNull() &&
            payload["stash_result"].isObject()) {
          stashResult = payload["stash_result"];
          if (!stashResult["sentenceId"].isNull() &&
              stashResult["sentenceId"].isInt()) {
            _stashResultSentenceId = stashResult["sentenceId"].asInt();
          }
          if (!stashResult["beginTime"].isNull() &&
              stashResult["beginTime"].isInt()) {
            _stashResultBeginTime = stashResult["beginTime"].asInt();
          }
          if (!stashResult["currentTime"].isNull() &&
              stashResult["currentTime"].isInt()) {
            _stashResultCurrentTime = stashResult["currentTime"].asInt();
          }
          if (!stashResult["text"].isNull() && stashResult["text"].isString()) {
            _stashResultText = stashResult["text"].asCString();
          }
        }
      }
    }
  }

  return Success;
}

int NlsEvent::parseMsgType(std::string name) {
  if (name == "TaskFailed") {
    _msgType = NlsEvent::TaskFailed;
  } else if (name == "RecognitionStarted") {
    _msgType = NlsEvent::RecognitionStarted;
  } else if (name == "RecognitionCompleted") {
    _msgType = NlsEvent::RecognitionCompleted;
  } else if (name == "RecognitionResultChanged") {
    _msgType = NlsEvent::RecognitionResultChanged;
  } else if (name == "TranscriptionStarted") {
    _msgType = NlsEvent::TranscriptionStarted;
  } else if (name == "SentenceBegin") {
    _msgType = NlsEvent::SentenceBegin;
  } else if (name == "TranscriptionResultChanged") {
    _msgType = NlsEvent::TranscriptionResultChanged;
  } else if (name == "SentenceEnd") {
    _msgType = NlsEvent::SentenceEnd;
  } else if (name == "TranscriptionCompleted") {
    _msgType = NlsEvent::TranscriptionCompleted;
  } else if (name == "SynthesisStarted") {
    _msgType = NlsEvent::SynthesisStarted;
  } else if (name == "SynthesisCompleted") {
    _msgType = NlsEvent::SynthesisCompleted;
  } else if (name == "DialogResultGenerated") {
    _msgType = NlsEvent::DialogResultGenerated;
  } else if (name == "WakeWordVerificationCompleted") {
    _msgType = NlsEvent::WakeWordVerificationCompleted;
  } else if (name == "SentenceSemantics") {
    _msgType = NlsEvent::SentenceSemantics;
  }  else if (name == "MetaInfo") {
    _msgType = NlsEvent::MetaInfo;
  } else {
//    LOG_ERROR("EVENT: type is invalid. [%s].", _msg.c_str());
    return -(InvalidNlsEventMsgType);
  }

  return Success;
}

int NlsEvent::getStatusCode() {
  return _statusCode;
}

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

NlsEvent::EventType NlsEvent::getMsgType() {
  return _msgType;
}

std::string NlsEvent::getMsgTypeString() {
  std::string ret_str = "Unknown";

  switch (_msgType) {
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
  }

  return ret_str;
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
  if (_msgType != RecognitionResultChanged &&
      _msgType != RecognitionCompleted &&
      _msgType != TranscriptionResultChanged &&
      _msgType != SentenceEnd) {
    return NULL;
  }
  return _result.c_str();
}

int NlsEvent::getSentenceIndex() {
  if (_msgType != SentenceBegin &&
      _msgType != SentenceEnd &&
      _msgType != TranscriptionResultChanged) {
    return -(InvalidNlsEventMsgType);
  }
  return _sentenceIndex;
}

int NlsEvent::getSentenceTime() {
  if (_msgType != SentenceBegin &&
      _msgType != SentenceEnd &&
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

NlsEvent::NlsEvent(std::vector<unsigned char> data, int code,
                   EventType type, std::string taskId) :
    _statusCode(code),
    _msgType(type),
    _taskId(taskId),
    _binaryData(data) {
  // LOG_DEBUG("Binary data event:%d.", data.size());
  this->_msg = "";
}

std::vector<unsigned char> NlsEvent::getBinaryData() {
  if (this->getMsgType() == Binary) {
    return this->_binaryData;
  } else {
    LOG_WARN("this hasn't Binary data.");
    return _binaryData;
  }
}

const bool NlsEvent::getWakeWordAccepted() {
  if (_msgType != WakeWordVerificationCompleted) {
    return false;
  }
  return _wakeWordAccepted;
}

const int NlsEvent::getStashResultSentenceId() {
  if (_msgType != SentenceEnd ) {
    return -(InvalidNlsEventMsgType);
  }
  return _stashResultSentenceId;
}

const int NlsEvent::getStashResultBeginTime() {
  if (_msgType != SentenceEnd ) {
    return -(InvalidNlsEventMsgType);
  }
  return _stashResultBeginTime;
}

const int NlsEvent::getStashResultCurrentTime() {
  if (_msgType != SentenceEnd ) {
    return -(InvalidNlsEventMsgType);
  }
  return _stashResultCurrentTime;
}

const char* NlsEvent::getStashResultText() {
  if (_msgType != SentenceEnd ) {
    return NULL;
  }
  return _stashResultText.c_str();
}

}  // namespace AlibabaNls
