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

#include <string>
#include "dialogAssistantParam.h"
#include "nlsRequestParamInfo.h"
#include "nlog.h"

namespace AlibabaNls {

#define D_CMD_START_RECOGNITION "StartRecognition"
#define D_CMD_STOP_RECOGNITION "StopRecognition"
#define D_CMD_EXECUTE_RECOGNITION "ExecuteDialog"
#define D_CMD_STOP_WAKEWORD_VERIFICATION "StopWakeWordVerification"
#define D_NAMESPACE_RECOGNITION "DialogAssistant"
#define D_NAMESPACE_RECOGNITION_V2 "DialogAssistant.v2"

DialogAssistantParam::DialogAssistantParam(int version, const char* sdkName) :
    INlsRequestParam(TypeDialog, sdkName) {
  if (version == 0) {
    _header[D_NAMESPACE] = D_NAMESPACE_RECOGNITION;
  } else {
    _header[D_NAMESPACE] = D_NAMESPACE_RECOGNITION_V2;
    _header[D_DA_ENABLE_MUTI_GROUP] = true;
  }

  _payload[D_DA_SESSION_ID] = _task_id.c_str();
}

DialogAssistantParam::~DialogAssistantParam() {}

const char* DialogAssistantParam::getStartCommand() {
  _header[D_NAME] = D_CMD_START_RECOGNITION;

  return INlsRequestParam::getStartCommand();
}

const char* DialogAssistantParam::getStopCommand() {
  _header[D_NAME] = D_CMD_STOP_RECOGNITION;

  return INlsRequestParam::getStopCommand();
}

const char* DialogAssistantParam::getExecuteDialog() {
  _header[D_NAME] = D_CMD_EXECUTE_RECOGNITION;

  return INlsRequestParam::getStartCommand();
}

const char* DialogAssistantParam::getStopWakeWordCommand() {
  _header[D_NAME] = D_CMD_STOP_WAKEWORD_VERIFICATION;

  return INlsRequestParam::getStopCommand();
}

int DialogAssistantParam::setQueryParams(const char* value) {
  std::string tmpValue = "{\"key\":";
  tmpValue += value;
  tmpValue += "}";

  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(tmpValue, root)) {
    LOG_ERROR("parse json fail: %s", value);
    return -1;
  }

  if (!root.isObject()) {
    LOG_ERROR("Params value is n't a json object.");
    return -1;
  }

  _payload[D_DA_QUERY_PARAMS] = root["key"];

  return 0;
}

int DialogAssistantParam::setQueryContext(const char* value) {
  _payload[D_DA_QUERY_CONTEXT] = value;

  return 0;
}

int DialogAssistantParam::setQuery(const char* value) {
  _payload[D_DA_QUERY] = value;

  return 0;
}

int DialogAssistantParam::setWakeWordModel(const char* value) {
  _payload[D_DA_WAKE_WORD_MODEL] = value;

  return 0;
}

int DialogAssistantParam::setWakeWord(const char* value) {
  _payload[D_DA_WAKE_WORD] = value;

  return 0;
}

void DialogAssistantParam::setEnableMultiGroup(bool value) {
  _header["enable_multi_group"] = value;
}

}
