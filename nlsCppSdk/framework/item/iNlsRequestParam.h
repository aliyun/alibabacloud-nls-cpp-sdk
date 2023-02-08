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

#ifndef NLS_SDK_REQUEST_PARAM_H
#define NLS_SDK_REQUEST_PARAM_H

#include <string>
#include "json/json.h"

namespace AlibabaNls {

enum NlsRequestType {
  SpeechNormal = 0, /* 拥有正常start, sendAudio, stop调用流程的语音功能 */
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
  INlsRequestParam(NlsType mode, const char* sdkName);
  virtual ~INlsRequestParam() = 0;

  std::string getRandomUuid();
  Json::Value getSdkInfo();

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
  inline void setNlsRequestType(NlsRequestType requestType) {
    _requestType = requestType;
  };

  void setAppKey(const char* appKey);
  void setFormat(const char* format);
  void setSampleRate(int sampleRate);

  inline void setTimeout(int timeout) {
    _timeout = timeout;
  };
  inline void setEnableRecvTimeout(bool enable) {
    _enableRecvTimeout = enable;
  };
  inline void setRecvTimeout(int timeout) {
    _recv_timeout = timeout;
  };
  inline void setSendTimeout(int timeout) {
    _send_timeout = timeout;
  };

  inline void setOutputFormat(const char* outputFormat) {
    _outputFormat = outputFormat;
  };

  inline void setEnableOnMessage(bool enable) {
    _enableOnMessage = enable;
  };

  void setIntermediateResult(bool value);
  void setPunctuationPrediction(bool value);
  void setTextNormalization(bool value);
  void setSentenceDetection(bool value);
  int setEnableWakeWordVerification(bool value);
  int setContextParam(const char* value);
  int setPayloadParam(const char* value);
  int setSessionId(const char* sessionId);

  virtual const char* getStartCommand();
  virtual const char* getStopCommand();
  virtual const char* getControlCommand(const char* message);
  virtual const char* getExecuteDialog();
  virtual const char* getStopWakeWordCommand();

  virtual int setCustomizationId(const char * value);
  virtual int setVocabularyId(const char * value);

  virtual std::string getOutputFormat();
  virtual int getTimeout();
  virtual bool getEnableRecvTimeout();
  virtual int getRecvTimeout();
  virtual int getSendTimeout();
  virtual bool getEnableOnMessage();

 public:
  int AppendHttpHeader(const char* key, const char* value);
  std::string GetHttpHeader();

 public:
  bool _enableWakeWord;
  bool _enableRecvTimeout;
  bool _enableOnMessage;

  int _timeout;
  int _recv_timeout;
  int _send_timeout;

  int _sampleRate;
  NlsRequestType _requestType;

  std::string _url;
  std::string _outputFormat;
  std::string _token;
  std::string _format;

  std::string _task_id;

  NlsType _mode;
  std::string _sdk_name;

  std::string _startCommand;
  std::string _controlCommand;
  std::string _stopCommand;

  Json::Value _header;
  Json::Value _payload;
  Json::Value _context;

  Json::Value _httpHeader;
  std::string _httpHeaderString;
};

}  // namespace AlibabaNls

#endif
