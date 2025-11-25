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

INlsRequestParam::INlsRequestParam(NlsType mode, const char* sdkName,
                                   int version)
    : _enableWakeWord(false),
      _enableRecvTimeout(false),
      _enableOnMessage(false),
#ifdef ENABLE_CONTINUED
      _enableReconnect(false),
#endif
      _timeout(D_DEFAULT_CONNECTION_TIMEOUT_MS),
      _recvTimeout(D_DEFAULT_RECV_TIMEOUT_MS),
      _sendTimeout(D_DEFAULT_SEND_TIMEOUT_MS),
      _sampleRate(D_DEFAULT_VALUE_SAMPLE_RATE),
      _requestType(SpeechNormal),
      _model(""),
      _url(D_DEFAULT_URL),
      _outputFormat(D_DEFAULT_VALUE_ENCODE_UTF8),
      _appKey(""),
      _token(""),
      _format(D_DEFAULT_VALUE_AUDIO_ENCODE),
      _taskId(""),
      _oldTaskId(""),
      _mode(mode),
      _sdkName(sdkName),
      _startCommand(""),
      _controlCommand(""),
      _stopCommand(""),
      _continueCommand("continue-task"),
      _header(Json::objectValue),
      _payload(Json::objectValue),
      _context(Json::objectValue),
      _httpHeader(Json::objectValue),
      _httpHeaderString(""),
      _streaming("duplex"),
      _taskGroup("audio"),
      _task(""),
      _function(""),
      // about speech transcriber and recognizer
      _punctuationPredictionEnabled(false),
      _inverseTextNormalizationEnabled(false),
      _semanticPunctuationEnabled(false),
      _multiThresholdModeEnabled(false),
      _maxSentenceSilence(0),
      _heartbeat(false),
      _vocabularyId(""),
      // about speech synthesizer
      _voice(""),
      _enableSsml(false),
      _wordTimestampEnabled(false),
      _bitRate(0),
      _enableAigcTag(false),
      _version(version) {
#if defined(_MSC_VER)
  _outputFormat = D_DEFAULT_VALUE_ENCODE_GBK;
#endif
  _payload[D_FORMAT] = D_DEFAULT_VALUE_AUDIO_ENCODE;
  _payload[D_SAMPLE_RATE] = D_DEFAULT_VALUE_SAMPLE_RATE;
  _context[D_SDK_CLIENT] = getSdkInfo();
}

INlsRequestParam::~INlsRequestParam() {}

INlsRequestParam& INlsRequestParam::operator=(const INlsRequestParam& other) {
  if (this != &other) {
    _enableWakeWord = other._enableWakeWord;
    _enableRecvTimeout = other._enableRecvTimeout;
    _enableOnMessage = other._enableOnMessage;
#ifdef ENABLE_CONTINUED
    _enableReconnect = other._enableReconnect;
#endif
    _timeout = other._timeout;
    _recvTimeout = other._recvTimeout;
    _sendTimeout = other._sendTimeout;
    _sampleRate = other._sampleRate;
    _requestType = other._requestType;
    _model = other._model;
    _url = other._url;
    _outputFormat = other._outputFormat;
    _appKey = other._appKey;
    _token = other._token;
    _apikey = other._apikey;
    _format = other._format;
    _taskId = other._taskId;
    _oldTaskId = other._oldTaskId;
    _mode = other._mode;
    _sdkName = other._sdkName;
    _startCommand = other._startCommand;
    _controlCommand = other._controlCommand;
    _stopCommand = other._stopCommand;
    _continueCommand = other._continueCommand;
    _header = other._header;
    _payload = other._payload;
    _context = other._context;
    _httpHeader = other._httpHeader;
    _httpHeaderString = other._httpHeaderString;
    _streaming = other._streaming;
    _taskGroup = other._taskGroup;
    _task = other._task;
    _function = other._function;
    // about speech transcriber and recognizer
    _punctuationPredictionEnabled = other._punctuationPredictionEnabled;
    _inverseTextNormalizationEnabled = other._inverseTextNormalizationEnabled;
    _semanticPunctuationEnabled = other._semanticPunctuationEnabled;
    _multiThresholdModeEnabled = other._multiThresholdModeEnabled;
    _maxSentenceSilence = other._maxSentenceSilence;
    _heartbeat = other._heartbeat;
    _vocabularyId = other._vocabularyId;
    // about speech synthesizer
    _voice = other._voice;
    _enableSsml = other._enableSsml;
    _wordTimestampEnabled = other._wordTimestampEnabled;
    _bitRate = other._bitRate;
    _enableAigcTag = other._enableAigcTag;
  }
  return *this;
}

bool INlsRequestParam::operator==(const INlsRequestParam& other) const {
  return _enableWakeWord == other._enableWakeWord &&
         _enableRecvTimeout == other._enableRecvTimeout &&
         _enableOnMessage == other._enableOnMessage &&
#ifdef ENABLE_CONTINUED
         _enableReconnect == other._enableReconnect &&
#endif
         _timeout == other._timeout && _recvTimeout == other._recvTimeout &&
         _sendTimeout == other._sendTimeout &&
         _sampleRate == other._sampleRate &&
         _requestType == other._requestType && _url == other._url &&
         _outputFormat == other._outputFormat && _appKey == other._appKey &&
         _format == other._format && _mode == other._mode &&
         _sdkName == other._sdkName;
}

Json::Value INlsRequestParam::getSdkInfo() {
  Json::Value sdkInfo;

  if (!_sdkName.empty() && _sdkName.compare("csharp") == 0) {
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
  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";

  try {
    if (_taskId == _oldTaskId) {
      _taskId = utility::TextUtils::getRandomUuid();
    }
    if (_taskId.empty()) {
      _taskId = utility::TextUtils::getRandomUuid();
    }
    _header[D_TASK_ID] = _taskId;
    // LOG_DEBUG("TaskId:%s", _taskId.c_str());
    _oldTaskId = _taskId;
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();

    root[D_HEADER] = _header;
    root[D_PAYLOAD] = _payload;
    root[D_CONTEXT] = _context;

    _startCommand = Json::writeString(writer, root);
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  return _startCommand.c_str();
}

const char* INlsRequestParam::getControlCommand(const char* message) {
  Json::Value root;
  Json::Value inputRoot;
  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  Json::CharReaderBuilder reader;
  std::istringstream iss(message);

  try {
    if (!Json::parseFromStream(reader, iss, &inputRoot, NULL)) {
      LOG_ERROR("Parse json(%s) failed!", message);
      return NULL;
    }

    if (!inputRoot.isObject()) {
      LOG_ERROR("Json value(%s) isn't a json object.", message);
      return NULL;
    }

    _header[D_TASK_ID] = _taskId;
    LOG_DEBUG("TaskId:%s", _taskId.c_str());
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();

    root[D_HEADER] = _header;
    if (!inputRoot[D_PAYLOAD].isNull()) {
      root[D_PAYLOAD] = inputRoot[D_PAYLOAD];
    }
    if (!inputRoot[D_CONTEXT].isNull()) {
      root[D_CONTEXT] = inputRoot[D_CONTEXT];
    }

    _controlCommand = Json::writeString(writer, root);
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  return _controlCommand.c_str();
}

const char* INlsRequestParam::getStopCommand() {
  Json::Value root;
  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";

  try {
    _header[D_TASK_ID] = _taskId;
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();

    root[D_HEADER] = _header;
    root[D_CONTEXT] = _context;

    _stopCommand = Json::writeString(writer, root);
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

const char* INlsRequestParam::getFlushFlowingTextCommand(
    const char* parameters) {
  return "";
}

int INlsRequestParam::setPayloadParam(const char* value) {
  Json::Value root;
  Json::CharReaderBuilder reader;
  Json::Value::iterator iter;
  Json::Value::Members members;

  if (value == NULL) {
    LOG_ERROR("Input value is nullptr");
    return -(SetParamsEmpty);
  }

  try {
    std::string tmpValue = value;
    std::istringstream iss(tmpValue);
    if (!Json::parseFromStream(reader, iss, &root, NULL)) {
      LOG_ERROR("Parse json(%s) failed!", tmpValue.c_str());
      return -(JsonParseFailed);
    }

    if (!root.isObject()) {
      LOG_ERROR("Json value(%s) isn't a json object.", tmpValue.c_str());
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
  Json::CharReaderBuilder reader;
  Json::Value::iterator iter;
  Json::Value::Members members;
  std::string tmpValue = value;
  std::istringstream iss(tmpValue);

  try {
    if (!Json::parseFromStream(reader, iss, &root, NULL)) {
      LOG_ERROR("Parse json(%s) failed!", tmpValue.c_str());
      return -(JsonParseFailed);
    }

    if (!root.isObject()) {
      LOG_ERROR("Json value(%s) isn't a json object.", tmpValue.c_str());
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
  _appKey = appKey;
  _header[D_APP_KEY] = appKey;
}

void INlsRequestParam::setFormat(const char* format) {
  _format = format;
  _payload[D_FORMAT] = format;
}

void INlsRequestParam::setModel(const char* model) { this->_model = model; }

void INlsRequestParam::setHeartbeat(bool enable) { this->_heartbeat = enable; }

void INlsRequestParam::setSemanticPunctuationEnabled(bool enable) {
  this->_semanticPunctuationEnabled = enable;
}

void INlsRequestParam::setMaxSentenceSilence(int value) {
  this->_maxSentenceSilence = value;
}

void INlsRequestParam::setMultiThresholdModeEnabled(bool enable) {
  this->_multiThresholdModeEnabled = enable;
}

void INlsRequestParam::setIntermediateResult(bool value) {
  _payload[D_SR_INTERMEDIATE_RESULT] = value;
}

void INlsRequestParam::setPunctuationPrediction(bool value) {
  _punctuationPredictionEnabled = value;
  _payload[D_SR_PUNCTUATION_PREDICTION] = value;
}

void INlsRequestParam::setTextNormalization(bool value) {
  _inverseTextNormalizationEnabled = value;
  _payload[D_SR_TEXT_NORMALIZATION] = value;
}

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

  _vocabularyId = value;
  _payload[D_SR_VOCABULARY_ID] = value;

  return Success;
}

void INlsRequestParam::setSentenceDetection(bool value) {
  _payload[D_ST_SENTENCE_DETECTION] = value;
}

void INlsRequestParam::setSampleRate(int sampleRate) {
  _sampleRate = sampleRate;
  _payload[D_SAMPLE_RATE] = sampleRate;
}

int INlsRequestParam::setEnableWakeWordVerification(bool value) {
  _payload[D_DA_WAKE_WORD_VERIFICATION] = value;

  _enableWakeWord = value;

  return Success;
}

int INlsRequestParam::setVoice(const char* value) {
  if (value == NULL) {
    return -(InvalidInputParam);
  }
  _payload[D_SY_VOICE] = value;
  _voice = value;
  return Success;
}

void INlsRequestParam::setSsmlEnabled(bool enable) { _enableSsml = enable; }

void INlsRequestParam::setWordTimestampEnabled(bool enable) {
  _wordTimestampEnabled = enable;
}

int INlsRequestParam::setBitRate(int rate) { _bitRate = rate; }

void INlsRequestParam::setAigcTagEnabled(bool enable) {
  _enableAigcTag = enable;
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

time_t INlsRequestParam::getRecvTimeout() { return _recvTimeout; }

time_t INlsRequestParam::getSendTimeout() { return _sendTimeout; }

std::string INlsRequestParam::getOutputFormat() { return _outputFormat; }

std::string INlsRequestParam::getTaskId() { return _taskId; }

NlsRequestType INlsRequestParam::getNlsRequestType() { return _requestType; }

std::string INlsRequestParam::getSdkName() { return _sdkName; }

int INlsRequestParam::getVersion() { return _version; }

}  // namespace AlibabaNls
