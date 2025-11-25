/*
 * Copyright 2025 Alibaba Group Holding Limited
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

#include <stdlib.h>
#include <string.h>

#include <sstream>

#include "json/json.h"
#include "nlog.h"
#include "nlsEventInner.h"

namespace AlibabaNls {

NlsEventInner::NlsEventInner()
    : _statusCode(0),
      _msg(""),
      _msgType(NlsEvent::TaskFailed),
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

NlsEventInner::NlsEventInner(const std::string& msg, NlsType nlsType,
                             NlsServiceProtocol serviceProtocol)
    : _msg(msg),
      _statusCode(Success),
      _msgType(NlsEvent::Message),
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

NlsEventInner::NlsEventInner(unsigned char* data, int dataBytes, int code,
                             NlsEvent::EventType type,
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

NlsEventInner::~NlsEventInner() {
  if (_binaryDataInChar) {
    free(_binaryDataInChar);
    _binaryDataInChar = NULL;
    _binaryDataSize = 0;
  }
}

NlsEventInner& NlsEventInner::operator=(const NlsEventInner& event) {
  if (this != &event) {
    this->_statusCode = event._statusCode;
    this->_msg = event._msg;
    this->_msgType = event._msgType;
    this->_taskId = event._taskId;
    this->_result = event._result;
    this->_displayText = event._displayText;
    this->_spokenText = event._spokenText;
    this->_sentenceTimeOutStatus = event._sentenceTimeOutStatus;
    this->_sentenceIndex = event._sentenceIndex;
    this->_sentenceTime = event._sentenceTime;
    this->_sentenceBeginTime = event._sentenceBeginTime;
    this->_sentenceConfidence = event._sentenceConfidence;
    this->_sentenceWordsList = event._sentenceWordsList;
    this->_wakeWordAccepted = event._wakeWordAccepted;
    this->_wakeWordKnown = event._wakeWordKnown;
    this->_wakeWordUserId = event._wakeWordUserId;
    this->_wakeWordGender = event._wakeWordGender;

    this->_binaryData = event._binaryData;

    this->_stashResultSentenceId = event._stashResultSentenceId;
    this->_stashResultBeginTime = event._stashResultBeginTime;
    this->_stashResultText = event._stashResultText;
    this->_stashResultCurrentTime = event._stashResultCurrentTime;

    this->_usage = event._usage;

    this->_nlsType = event._nlsType;
    this->_serviceProtocol = event._serviceProtocol;
  }
  return *this;
}

int NlsEventInner::transferEvent(NlsEvent* target) {
  target->_statusCode = this->_statusCode;
  target->_msg = this->_msg;
  target->_msgType = this->_msgType;
  target->_taskId = this->_taskId;
  target->_result = this->_result;
  target->_displayText = this->_displayText;
  target->_spokenText = this->_spokenText;
  target->_sentenceTimeOutStatus = this->_sentenceTimeOutStatus;
  target->_sentenceIndex = this->_sentenceIndex;
  target->_sentenceTime = this->_sentenceTime;
  target->_sentenceBeginTime = this->_sentenceBeginTime;
  target->_sentenceConfidence = this->_sentenceConfidence;
  target->_sentenceWordsList = this->_sentenceWordsList;
  target->_wakeWordAccepted = this->_wakeWordAccepted;
  target->_wakeWordKnown = this->_wakeWordKnown;
  target->_wakeWordUserId = this->_wakeWordUserId;
  target->_wakeWordGender = this->_wakeWordGender;

  target->_binaryData = this->_binaryData;

  target->_stashResultSentenceId = this->_stashResultSentenceId;
  target->_stashResultBeginTime = this->_stashResultBeginTime;
  target->_stashResultText = this->_stashResultText;
  target->_stashResultCurrentTime = this->_stashResultCurrentTime;

  target->_usage = this->_usage;

  target->_nlsType = this->_nlsType;
  target->_serviceProtocol = this->_serviceProtocol;

  return Success;
}

int NlsEventInner::parseJsonMsg(bool ignore) {
  if (_msg.empty()) {
    return -(NlsEventMsgEmpty);
  }

  try {
    Json::CharReaderBuilder reader;
    Json::Value head(Json::objectValue);
    Json::Value payload(Json::objectValue);
    Json::Value root(Json::objectValue);
    Json::Value stashResult(Json::objectValue);
    std::istringstream iss(_msg);

    if (!Json::parseFromStream(reader, iss, &root, NULL)) {
      LOG_ERROR("_msg:%s", _msg.c_str());
      return -(JsonParseFailed);
    } else {
      // LOG_DEBUG("parsing json: %s", _msg.c_str());
    }

    // parse head
    if (!root["header"].isNull() && root["header"].isObject()) {
      head = root["header"];
      // name
      if ((!head["name"].isNull() && head["name"].isString()) ||
          (!head["event"].isNull() && head["event"].isString())) {
        std::string name = "";
        if (_serviceProtocol == WsServiceProtocolDashScope) {
          name = head["event"].asCString();
        } else {
          name = head["name"].asCString();
        }
        int retCode = parseMsgType(name);
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
      if (_serviceProtocol != WsServiceProtocolDashScope) {
        if (!head["status"].isNull() && head["status"].isInt()) {
          _statusCode = head["status"].asInt();
        } else {
          if (ignore == false) {
            return -(InvalidNlsEventMsgStatusCode);
          }
        }
      }

      // task_id
      if (!head["task_id"].isNull() && head["task_id"].isString()) {
        _taskId = head["task_id"].asCString();
      }
    } else {
      if (!root["channelClosed"].isNull()) {
        _msgType = NlsEvent::Close;
      } else if (!root["TaskFailed"].isNull()) {
        _msgType = NlsEvent::TaskFailed;
      } else {
        if (ignore == false) {
          return -(InvalidNlsEventMsgHeader);
        }
      }
    }

    // parse payload
    if (_serviceProtocol == WsServiceProtocolDashScope) {
      int ret = Success;
      if (_nlsType == TypeDashScopeFunAsrRealTime) {
        ret = convertFunAsrStResultGenerated();
      } else if (_nlsType == TypeDashScopeParaformerRealTime) {
        ret = convertParaformerStResultGenerated();
      } else if (_nlsType == TypeDashSceopCosyVoiceStreamInputTts) {
        ret = convertCosyVoiceFssResultGenerated();
      } else {
        LOG_ERROR("invalid nls type:%d.", _nlsType);
      }
      if (ret) return ret;
    } else {
      if (_msgType != NlsEvent::SynthesisCompleted &&
          _msgType != NlsEvent::MetaInfo) {
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
          if (!payload["begin_time"].isNull() &&
              payload["begin_time"].isInt()) {
            _sentenceBeginTime = payload["begin_time"].asInt();
          }

          // confidence
          if (!payload["confidence"].isNull() &&
              payload["confidence"].isDouble()) {
            _sentenceConfidence = payload["confidence"].asDouble();
          }

          // display_text
          if (!payload["display_text"].isNull() &&
              payload["display_text"].isString()) {
            _displayText = payload["display_text"].asCString();
          }

          // spoken_text
          if (!payload["spoken_text"].isNull() &&
              payload["spoken_text"].isString()) {
            _spokenText = payload["spoken_text"].asCString();
          }

          // sentence timeOut status
          if (!payload["status"].isNull() && payload["status"].isInt()) {
            _sentenceTimeOutStatus = payload["status"].asInt();
          }

          // example:
          // "words":[{"text":"一二三四","startTime":810,"endTime":2460}]
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
              //     wordInfo.text.c_str(), wordInfo.startTime,
              //     wordInfo.endTime);

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
              if (!stashResult["text"].isNull() &&
                  stashResult["text"].isString()) {
                _stashResultText = stashResult["text"].asCString();
              }
            }
          }
        }
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return -(JsonParseFailed);
  }
  return Success;
}

const char* NlsEventInner::getAllResponse() {
  if (this->getMsgType() == NlsEvent::Binary) {
    // LOG_DEBUG("this is Binary data.");
  }
  return this->_msg.c_str();
}

NlsEvent::EventType NlsEventInner::getMsgType() { return _msgType; }

int NlsEventInner::parseMsgType(std::string name) {
  if (name == "TaskFailed" || name == "task-failed") {
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
  } else if (name == "MetaInfo") {
    _msgType = NlsEvent::MetaInfo;
  } else if (name == "SentenceSynthesis") {
    _msgType = NlsEvent::SentenceSynthesis;
  } else if (name == "task-started") {
    _msgType = NlsEvent::TaskStarted;
  } else if (name == "result-generated") {
    _msgType = NlsEvent::ResultGenerated;
  } else if (name == "task-finished") {
    _msgType = NlsEvent::TaskFinished;
  } else {
    //    LOG_ERROR("EVENT: type is invalid. [%s].", _msg.c_str());
    return -(InvalidNlsEventMsgType);
  }

  return Success;
}

int NlsEventInner::convertFunAsrStResultGenerated() {
  if (_msg.empty()) {
    return -(NlsEventMsgEmpty);
  }

  if (_msgType == NlsEvent::TaskStarted) {
    _msgType = NlsEvent::TranscriptionStarted;
    return Success;
  } else if (_msgType == NlsEvent::TaskFinished) {
    _msgType = NlsEvent::TranscriptionCompleted;
    return Success;
  }

  try {
    Json::CharReaderBuilder reader;
    Json::Value root(Json::objectValue);
    std::istringstream iss(_msg);

    if (!Json::parseFromStream(reader, iss, &root, NULL)) {
      LOG_ERROR("_msg:%s", _msg.c_str());
      return -(JsonParseFailed);
    }

    if (!root["payload"].isNull() && root["payload"].isObject()) {
      Json::Value payload(Json::objectValue);
      payload = root["payload"];
      if (!payload["output"].isNull()) {
        Json::Value output = payload["output"];
        if (!output["sentence"].isNull()) {
          Json::Value sentence = output["sentence"];

          // payload.output.sentence.sentence_begin
          // payload.output.sentence.sentence_end
          bool sentence_end = false;
          bool sentence_begin = false;
          if (!sentence["sentence_end"].isNull()) {
            sentence_end = sentence["sentence_end"].asBool();
          }
          if (!sentence["sentence_begin"].isNull()) {
            sentence_begin = sentence["sentence_begin"].asBool();
          }
          if (sentence_end) {
            _msgType = NlsEvent::SentenceEnd;
          } else if (sentence_begin) {
            _msgType = NlsEvent::SentenceBegin;
          } else {
            _msgType = NlsEvent::TranscriptionResultChanged;
          }

          // payload.output.sentence.begin_time
          if (!sentence["begin_time"].isNull() &&
              sentence["begin_time"].isInt()) {
            _sentenceBeginTime = sentence["begin_time"].asInt();
            _sentenceTime = _sentenceBeginTime;
          }
          // payload.output.sentence.end_time
          if (!sentence["end_time"].isNull() && sentence["end_time"].isInt()) {
            _sentenceTime = sentence["end_time"].asInt();
          }

          // payload.output.sentence.text
          if (!sentence["text"].isNull() && sentence["text"].isString()) {
            _result = sentence["text"].asCString();
          }

          // payload.output.sentence.sentence_id
          if (!sentence["sentence_id"].isNull() &&
              sentence["sentence_id"].isInt()) {
            _sentenceIndex = sentence["sentence_id"].asInt();
          }

          // payload.output.sentence.words
          // "words":[{"text":"一","begin_time":170,"end_time":295}]
          if (!sentence["words"].isNull() && sentence["words"].isArray()) {
            Json::Value wordArray = sentence["words"];
            int iSize = wordArray.size();
            WordInfomation wordInfo;

            for (int nIndex = 0; nIndex < iSize; nIndex++) {
              if (wordArray[nIndex].isMember("text") &&
                  wordArray[nIndex]["text"].isString()) {
                wordInfo.text = wordArray[nIndex]["text"].asCString();
              }
              if (wordArray[nIndex].isMember("begin_time") &&
                  wordArray[nIndex]["begin_time"].isInt()) {
                wordInfo.startTime = wordArray[nIndex]["begin_time"].asInt();
              }
              if (wordArray[nIndex].isMember("end_time") &&
                  wordArray[nIndex]["end_time"].isInt()) {
                wordInfo.endTime = wordArray[nIndex]["end_time"].asInt();
              }
              _sentenceWordsList.push_back(wordInfo);
            }  // for
          }
        }
      }

      if (!payload["usage"].isNull()) {
        Json::Value usage = payload["usage"];
        // payload.usage.duration
        if (!usage["duration"].isNull() && usage["duration"].isInt()) {
          _usage = usage["duration"].asInt();
        }
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return -(JsonParseFailed);
  }
  return Success;
}

int NlsEventInner::convertParaformerStResultGenerated() {
  if (_msg.empty()) {
    return -(NlsEventMsgEmpty);
  }

  if (_msgType == NlsEvent::TaskStarted) {
    _msgType = NlsEvent::TranscriptionStarted;
    return Success;
  } else if (_msgType == NlsEvent::TaskFinished) {
    _msgType = NlsEvent::TranscriptionCompleted;
    return Success;
  }

  try {
    Json::CharReaderBuilder reader;
    Json::Value root(Json::objectValue);
    std::istringstream iss(_msg);

    if (!Json::parseFromStream(reader, iss, &root, NULL)) {
      LOG_ERROR("_msg:%s", _msg.c_str());
      return -(JsonParseFailed);
    }

    if (!root["payload"].isNull() && root["payload"].isObject()) {
      Json::Value payload(Json::objectValue);
      payload = root["payload"];
      if (!payload["output"].isNull()) {
        Json::Value output = payload["output"];
        if (!output["sentence"].isNull()) {
          Json::Value sentence = output["sentence"];

          // payload.output.sentence.sentence_begin
          // payload.output.sentence.sentence_end
          bool sentence_end = false;
          bool sentence_begin = false;
          if (!sentence["sentence_end"].isNull()) {
            sentence_end = sentence["sentence_end"].asBool();
          }
          if (!sentence["sentence_begin"].isNull()) {
            sentence_begin = sentence["sentence_begin"].asBool();
          }
          if (sentence_end) {
            _msgType = NlsEvent::SentenceEnd;
          } else if (sentence_begin) {
            _msgType = NlsEvent::SentenceBegin;
          } else {
            _msgType = NlsEvent::TranscriptionResultChanged;
          }

          // payload.output.sentence.begin_time
          if (!sentence["begin_time"].isNull() &&
              sentence["begin_time"].isInt()) {
            _sentenceBeginTime = sentence["begin_time"].asInt();
            _sentenceTime = _sentenceBeginTime;
          }
          // payload.output.sentence.end_time
          if (!sentence["end_time"].isNull() && sentence["end_time"].isInt()) {
            _sentenceTime = sentence["end_time"].asInt();
          }

          // payload.output.sentence.text
          if (!sentence["text"].isNull() && sentence["text"].isString()) {
            _result = sentence["text"].asCString();
          }

          // payload.output.sentence.sentence_id
          if (!sentence["sentence_id"].isNull() &&
              sentence["sentence_id"].isInt()) {
            _sentenceIndex = sentence["sentence_id"].asInt();
          }

          // payload.output.sentence.words
          // "words":[{"text":"一","begin_time":170,"end_time":295}]
          if (!sentence["words"].isNull() && sentence["words"].isArray()) {
            Json::Value wordArray = sentence["words"];
            int iSize = wordArray.size();
            WordInfomation wordInfo;

            for (int nIndex = 0; nIndex < iSize; nIndex++) {
              if (wordArray[nIndex].isMember("text") &&
                  wordArray[nIndex]["text"].isString()) {
                wordInfo.text = wordArray[nIndex]["text"].asCString();
              }
              if (wordArray[nIndex].isMember("begin_time") &&
                  wordArray[nIndex]["begin_time"].isInt()) {
                wordInfo.startTime = wordArray[nIndex]["begin_time"].asInt();
              }
              if (wordArray[nIndex].isMember("end_time") &&
                  wordArray[nIndex]["end_time"].isInt()) {
                wordInfo.endTime = wordArray[nIndex]["end_time"].asInt();
              }
              _sentenceWordsList.push_back(wordInfo);
            }  // for
          }
        }
      }

      if (!payload["usage"].isNull()) {
        Json::Value usage = payload["usage"];
        // payload.usage.duration
        if (!usage["duration"].isNull() && usage["duration"].isInt()) {
          _usage = usage["duration"].asInt();
        }
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return -(JsonParseFailed);
  }
  return Success;
}

int NlsEventInner::convertCosyVoiceFssResultGenerated() {
  if (_msg.empty()) {
    return -(NlsEventMsgEmpty);
  }

  if (_msgType == NlsEvent::TaskStarted) {
    _msgType = NlsEvent::SynthesisStarted;
    return Success;
  } else if (_msgType == NlsEvent::TaskFinished) {
    _msgType = NlsEvent::SynthesisCompleted;
    return Success;
  }

  try {
    Json::CharReaderBuilder reader;
    Json::Value root(Json::objectValue);
    std::istringstream iss(_msg);
    bool validMessage = false;

    if (!Json::parseFromStream(reader, iss, &root, NULL)) {
      LOG_ERROR("_msg:%s", _msg.c_str());
      return -(JsonParseFailed);
    }

    if (_msgType == NlsEvent::TaskFailed) {
      validMessage = true;
    }

    if (!root["payload"].isNull() && root["payload"].isObject()) {
      Json::Value payload(Json::objectValue);
      payload = root["payload"];
      if (!payload["output"].isNull()) {
        Json::Value output = payload["output"];
        if (!output["sentence"].isNull()) {
          Json::Value sentence = output["sentence"];
          _msgType = NlsEvent::SentenceSynthesis;

          // sentence.index
          if (!sentence["index"].isNull() && sentence["index"].isInt()) {
            _sentenceIndex = sentence["index"].asInt();
          }

          // setence.words[]
          if (!sentence["words"].isNull() && sentence["words"].isArray()) {
            Json::Value wordArray = sentence["words"];
            int iSize = wordArray.size();
            if (iSize == 0) {
              LOG_VERBOSE("Ignore this msg: %s", _msg.c_str());
            } else {
              validMessage = true;
            }
            WordInfomation wordInfo;
            for (int nIndex = 0; nIndex < iSize; nIndex++) {
              if (wordArray[nIndex].isMember("text") &&
                  wordArray[nIndex]["text"].isString()) {
                wordInfo.text = wordArray[nIndex]["text"].asCString();
              }
              if (wordArray[nIndex].isMember("begin_time") &&
                  wordArray[nIndex]["begin_time"].isInt()) {
                wordInfo.startTime = wordArray[nIndex]["begin_time"].asInt();
              }
              if (wordArray[nIndex].isMember("end_time") &&
                  wordArray[nIndex]["end_time"].isInt()) {
                wordInfo.endTime = wordArray[nIndex]["end_time"].asInt();
              }
              _sentenceWordsList.push_back(wordInfo);
            }  // for
          }
        }
      }

      if (!payload["usage"].isNull()) {
        Json::Value usage = payload["usage"];
        // payload.usage.characters
        if (!usage["characters"].isNull() && usage["characters"].isInt()) {
          _usage = usage["characters"].asInt();
          validMessage = true;
        }
      }
    }

    if (!validMessage) {
      return -(IgnoredNlsEventMsg);
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return -(JsonParseFailed);
  }
  return Success;
}

}  // namespace AlibabaNls
