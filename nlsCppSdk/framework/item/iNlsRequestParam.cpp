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

#include "iNlsRequestParam.h"
#include "Config.h"
#include "connectNode.h"
#include "nlog.h"
#include "nlsGlobal.h"
#include "nlsRequestParamInfo.h"
#include "text_utils.h"

namespace AlibabaNls {

#if defined(__ANDROID__)
const char g_sdk_name[] = "nls-cpp-sdk3.x-android";
#elif defined(_MSC_VER)
const char g_sdk_name[] = "nls-cpp-sdk3.x-windows";
const char g_csharp_sdk_name[] = "nls-csharp-sdk3.x-windows";
#elif defined(__APPLE__)
const char g_sdk_name[] = "nls-cpp-sdk3.x-ios";
#elif defined(__linux__)
const char g_sdk_name[] = "nls-cpp-sdk3.x-linux";
#else
const char g_sdk_name[] = "nls-cpp-sdk3.x-unknown";
#endif

const char g_csharp_sdk_language[] = "Csharp";
const char g_sdk_language[] = "C++";
const char g_sdk_version[] = NLS_SDK_VERSION_STR;

INlsRequestParam::INlsRequestParam(NlsType mode, const char* sdkName)
    : _enableWakeWord(false),
      _enableRecvTimeout(false),
      _enableOnMessage(false),
#ifdef ENABLE_CONTINUED
      _enableReconnect(false),
#endif
      _timeout(D_DEFAULT_CONNECTION_TIMEOUT_MS),
      _recv_timeout(D_DEFAULT_RECV_TIMEOUT_MS),
      _send_timeout(D_DEFAULT_SEND_TIMEOUT_MS),
      _sampleRate(D_DEFAULT_VALUE_SAMPLE_RATE),
      _requestType(SpeechNormal),
      _url(D_DEFAULT_URL),
      _outputFormat(D_DEFAULT_VALUE_ENCODE_UTF8),
      _token(""),
      _format(D_DEFAULT_VALUE_AUDIO_ENCODE),
      _task_id(""),
      _mode(mode),
      _sdk_name(sdkName),
      _startCommand(""),
      _controlCommand(""),
      _stopCommand(""),
      _header(Json::objectValue),
      _payload(Json::objectValue),
      _context(Json::objectValue),
      _httpHeader(Json::objectValue),
      _httpHeaderString("") {
#if defined(_MSC_VER)
  _outputFormat = D_DEFAULT_VALUE_ENCODE_GBK;
#endif
  _payload[D_FORMAT] = D_DEFAULT_VALUE_AUDIO_ENCODE;
  _payload[D_SAMPLE_RATE] = D_DEFAULT_VALUE_SAMPLE_RATE;
  _context[D_SDK_CLIENT] = getSdkInfo();
}

INlsRequestParam::~INlsRequestParam() {}

Json::Value INlsRequestParam::getSdkInfo() {
  Json::Value sdkInfo;

  if (!_sdk_name.empty() && _sdk_name.compare("csharp") == 0) {
#ifdef _MSC_VER
    sdkInfo[D_SDK_NAME] = g_csharp_sdk_name;
#else
    sdkInfo[D_SDK_NAME] = g_sdk_name;
#endif
    sdkInfo[D_SDK_LANGUAGE] = g_csharp_sdk_language;
  } else {
    sdkInfo[D_SDK_NAME] = g_sdk_name;
    sdkInfo[D_SDK_LANGUAGE] = g_sdk_language;
  }
  sdkInfo[D_SDK_VERSION] = g_sdk_version;

  return sdkInfo;
}

const char* INlsRequestParam::getStartCommand() {
  Json::Value root;
  Json::FastWriter writer;

  try {
    _task_id = utility::TextUtils::getRandomUuid();
    _header[D_TASK_ID] = _task_id;
    // LOG_DEBUG("TaskId:%s", _task_id.c_str());
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();

    root[D_HEADER] = _header;
    root[D_PAYLOAD] = _payload;
    root[D_CONTEXT] = _context;

    _startCommand = writer.write(root);
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  return _startCommand.c_str();
}

const char* INlsRequestParam::getControlCommand(const char* message) {
  Json::Value root;
  Json::Value inputRoot;
  Json::FastWriter writer;
  Json::Reader reader;

  try {
    if (!reader.parse(message, inputRoot)) {
      LOG_ERROR("Parse json(%s) failed!", message);
      return NULL;
    }

    if (!inputRoot.isObject()) {
      LOG_ERROR("Json value isn't a json object.");
      return NULL;
    }

    _header[D_TASK_ID] = _task_id;
    LOG_DEBUG("TaskId:%s", _task_id.c_str());
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();

    root[D_HEADER] = _header;
    if (!inputRoot[D_PAYLOAD].isNull()) {
      root[D_PAYLOAD] = inputRoot[D_PAYLOAD];
    }
    if (!inputRoot[D_CONTEXT].isNull()) {
      root[D_CONTEXT] = inputRoot[D_CONTEXT];
    }

    _controlCommand = writer.write(root);
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  return _controlCommand.c_str();
}

const char* INlsRequestParam::getStopCommand() {
  Json::Value root;
  Json::FastWriter writer;

  try {
    _header[D_TASK_ID] = _task_id;
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();

    root[D_HEADER] = _header;
    root[D_CONTEXT] = _context;

    _stopCommand = writer.write(root);
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  return _stopCommand.c_str();
}

const char* INlsRequestParam::getExecuteDialog() { return ""; }

const char* INlsRequestParam::getStopWakeWordCommand() { return ""; }

const char* INlsRequestParam::getRunFlowingSynthesisCommand(const char* text) {
  return "";
}

const char* INlsRequestParam::getFlushFlowingTextCommand() { return ""; }

int INlsRequestParam::setPayloadParam(const char* value) {
  Json::Value root;
  Json::Reader reader;
  Json::Value::iterator iter;
  Json::Value::Members members;

  if (value == NULL) {
    LOG_ERROR("Input value is nullptr");
    return -(SetParamsEmpty);
  }

  try {
    std::string tmpValue = value;
    if (!reader.parse(tmpValue, root)) {
      LOG_ERROR("Parse json(%s) failed!", tmpValue.c_str());
      return -(JsonParseFailed);
    }

    if (!root.isObject()) {
      LOG_ERROR("Json value isn't a json object.");
      return -(JsonObjectError);
    }

    std::string jsonKey;
    members = root.getMemberNames();
    Json::Value::Members::iterator it = members.begin();
    for (; it != members.end(); ++it) {
      jsonKey = *it;
      if (jsonKey.empty()) {
        LOG_ERROR("Get empty member in %s", tmpValue.c_str());
        continue;
      }
      _payload[jsonKey.c_str()] = root[jsonKey.c_str()];
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return -(JsonParseFailed);
  }
  return Success;
}

int INlsRequestParam::removePayloadParam(const char* key) {
  if (key == NULL) {
    LOG_ERROR("Input key is nullptr");
    return -(SetParamsEmpty);
  }

  try {
    Json::Value root;
    std::string tmpValue = key;
    if (_payload.isMember(tmpValue)) {
      _payload.removeMember(tmpValue);
      LOG_DEBUG("remove member %s", tmpValue.c_str());
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return -(JsonParseFailed);
  }
  return Success;
}

int INlsRequestParam::setContextParam(const char* value) {
  Json::Value root;
  Json::Reader reader;
  Json::Value::iterator iter;
  Json::Value::Members members;
  std::string tmpValue = value;

  try {
    if (!reader.parse(tmpValue, root)) {
      LOG_ERROR("Parse json(%s) failed!", tmpValue.c_str());
      return -(JsonParseFailed);
    }

    if (!root.isObject()) {
      LOG_ERROR("Json value isn't a json object.");
      return -(JsonObjectError);
    }

    std::string jsonKey;
    members = root.getMemberNames();
    Json::Value::Members::iterator it = members.begin();
    for (; it != members.end(); ++it) {
      jsonKey = *it;
      if (jsonKey.empty()) {
        LOG_ERROR("Get empty member in %s", tmpValue.c_str());
        continue;
      }
      _context[jsonKey.c_str()] = root[jsonKey.c_str()];
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return -(JsonParseFailed);
  }
  return Success;
}

void INlsRequestParam::setAppKey(const char* appKey) {
  _header[D_APP_KEY] = appKey;
};

void INlsRequestParam::setFormat(const char* format) {
  _format = format;
  _payload[D_FORMAT] = format;
};

void INlsRequestParam::setIntermediateResult(bool value) {
  _payload[D_SR_INTERMEDIATE_RESULT] = value;
};

void INlsRequestParam::setPunctuationPrediction(bool value) {
  _payload[D_SR_PUNCTUATION_PREDICTION] = value;
};

void INlsRequestParam::setTextNormalization(bool value) {
  _payload[D_SR_TEXT_NORMALIZATION] = value;
};

int INlsRequestParam::setCustomizationId(const char* value) {
  if (value == NULL) {
    return -(SetParamsEmpty);
  }

  _payload[D_SR_CUSTOMIZATION_ID] = value;

  return Success;
}

int INlsRequestParam::setVocabularyId(const char* value) {
  if (value == NULL) {
    return -(SetParamsEmpty);
  }

  _payload[D_SR_VOCABULARY_ID] = value;

  return Success;
}

void INlsRequestParam::setSentenceDetection(bool value) {
  _payload[D_ST_SENTENCE_DETECTION] = value;
};

void INlsRequestParam::setSampleRate(int sampleRate) {
  _sampleRate = sampleRate;
  _payload[D_SAMPLE_RATE] = sampleRate;
};

int INlsRequestParam::setEnableWakeWordVerification(bool value) {
  _payload[D_DA_WAKE_WORD_VERIFICATION] = value;

  _enableWakeWord = value;

  return Success;
}

int INlsRequestParam::setSessionId(const char* sessionId) {
  _payload[D_DA_SESSION_ID] = sessionId;
  return Success;
}

int INlsRequestParam::AppendHttpHeader(const char* key, const char* value) {
  if (key == NULL || value == NULL) {
    LOG_ERROR("key or value is nullptr!");
    return -(InvalidInputParam);
  }
  _httpHeader[key] = value;
  return Success;
}

std::string INlsRequestParam::GetHttpHeader() {
  Json::Value::iterator iter;
  Json::Value::Members members;

  _httpHeaderString.clear();

  if (_httpHeader.empty()) {
    return _httpHeaderString;
  }

  try {
    std::string jsonKey;
    members = _httpHeader.getMemberNames();
    Json::Value::Members::iterator it = members.begin();
    for (; it != members.end(); ++it) {
      jsonKey = *it;
      if (jsonKey.empty()) {
        LOG_ERROR("Get empty member!!!");
        continue;
      }

      _httpHeaderString += jsonKey;
      _httpHeaderString += ": ";
      _httpHeaderString += _httpHeader[jsonKey].asString();
      _httpHeaderString += "\r\n";
    }

    LOG_DEBUG("HttpHeader:%s", _httpHeaderString.c_str());
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return _httpHeaderString;
  }
  return _httpHeaderString;
}

time_t INlsRequestParam::getTimeout() { return _timeout; }

bool INlsRequestParam::getEnableRecvTimeout() { return _enableRecvTimeout; }

bool INlsRequestParam::getEnableOnMessage() { return _enableOnMessage; }

time_t INlsRequestParam::getRecvTimeout() { return _recv_timeout; }

time_t INlsRequestParam::getSendTimeout() { return _send_timeout; }

std::string INlsRequestParam::getOutputFormat() { return _outputFormat; }

std::string INlsRequestParam::getTaskId() { return _task_id; }

NlsRequestType INlsRequestParam::getNlsRequestType() { return _requestType; }

}  // namespace AlibabaNls
