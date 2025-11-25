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

#include "dashParaformerTranscriberParam.h"

#include <string.h>

#include "nlog.h"
#include "nlsRequestParamInfo.h"
#include "text_utils.h"

namespace AlibabaNls {

DashParaformerTranscriberParam::DashParaformerTranscriberParam(
    const char* sdkName)
    : INlsRequestParam(TypeDashScopeParaformerRealTime, sdkName),
      _disfluencyRemovalEnabled(false),
      _languageHintsJsonArray(""),
      _resourcesJsonArray(""),
      _inputJsonObj("{}") {
  this->_task = "asr";
  this->_function = "recognition";
  this->_semanticPunctuationEnabled = false;
  this->_multiThresholdModeEnabled = false;
  this->_punctuationPredictionEnabled = true;
  this->_inverseTextNormalizationEnabled = true;
}

DashParaformerTranscriberParam::~DashParaformerTranscriberParam() {}

const char* DashParaformerTranscriberParam::getStartCommand() {
  Json::Value root, header, payload, parameters;
  Json::Reader reader;
  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "";

  if (this->_taskId == this->_oldTaskId) {
    this->_taskId = utility::TextUtils::getRandomUuid();
  }
  if (this->_taskId.empty()) {
    this->_taskId = utility::TextUtils::getRandomUuid();
  }
  this->_oldTaskId = this->_taskId;

  header[D_ACTION] = "run-task";
  header[D_STREAMING] = this->_streaming;
  header[D_TASK_ID] = this->_taskId;

  // payload ==>>
  if (!this->_taskGroup.empty()) {
    payload[D_TASK_GROUP] = this->_taskGroup;
  }
  if (!this->_task.empty()) {
    payload[D_TASK] = this->_task;
  }
  if (!this->_function.empty()) {
    payload[D_FUNCTION] = this->_function;
  }
  if (!this->_model.empty()) {
    payload[D_MODEL] = this->_model;
  }
  // payload.input ->
  if (!this->_inputJsonObj.empty()) {
    Json::Value tmp;
    if (reader.parse(this->_inputJsonObj, tmp)) {
      if (tmp.isObject()) {
        payload["input"] = tmp;
      }
    }
  } else {
    payload["input"] = Json::objectValue;
  }

  // payload.parameters ->
  if (!this->_format.empty()) {
    parameters[D_FORMAT] = this->_format;
  }
  if (this->_sampleRate > 0) {
    parameters[D_SAMPLE_RATE] = this->_sampleRate;
  }
  if (!this->_vocabularyId.empty()) {
    parameters[D_SR_VOCABULARY_ID] = this->_vocabularyId;
  }
  parameters[D_ST_DISFLUENCY_REMOVAL_ENABLED] = this->_disfluencyRemovalEnabled;
  if (!this->_languageHintsJsonArray.empty()) {
    Json::Value tmp;
    if (reader.parse(this->_languageHintsJsonArray, tmp)) {
      if (tmp.isArray()) {
        parameters[D_ST_LANGUAGE_HINTS] = tmp;
      }
    }
  }
  parameters[D_ST_SEMANTIC_PUNCTUATION_ENABLED] =
      this->_semanticPunctuationEnabled;
  if (this->_maxSentenceSilence > 0) {
    parameters[D_ST_MAX_SENTENCE_SILENCE] = this->_maxSentenceSilence;
  }
  parameters[D_ST_MULTI_THRESHOLD_MODE_ENABLED] =
      this->_multiThresholdModeEnabled;
  parameters[D_ST_PUNCTUATION_PREDICTION_ENABLED] =
      this->_punctuationPredictionEnabled;
  parameters[D_ST_HEARTBEAT] = this->_heartbeat;
  parameters[D_ST_INVERSE_TEXT_NORMALIZATION] =
      this->_inverseTextNormalizationEnabled;

  if (!this->_resourcesJsonArray.empty()) {
    Json::Value tmp;
    if (reader.parse(this->_resourcesJsonArray, tmp)) {
      if (tmp.isArray()) {
        payload[D_ST_RESOURCES] = tmp;
      }
    }
  }

  payload["parameters"] = parameters;
  root["header"] = header;
  root["payload"] = payload;
  _startCommand = Json::writeString(writerBuilder, root);
  return _startCommand.c_str();
}

const char* DashParaformerTranscriberParam::getStopCommand() {
  Json::Value root, header, payload, input(Json::objectValue);
  Json::Reader reader;
  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "";

  header[D_ACTION] = "finish-task";
  header[D_STREAMING] = this->_streaming;
  header[D_TASK_ID] = this->_taskId;

  payload["input"] = input;

  root["header"] = header;
  root["payload"] = payload;
  _stopCommand = Json::writeString(writerBuilder, root);
  return _stopCommand.c_str();
}

int DashParaformerTranscriberParam::setDisfluencyRemovalEnabled(bool enable) {
  this->_disfluencyRemovalEnabled = enable;
  return Success;
}

int DashParaformerTranscriberParam::setLanguageHints(const char* jsonArrayStr) {
  this->_languageHintsJsonArray = jsonArrayStr;
  return Success;
}

int DashParaformerTranscriberParam::setResources(const char* jsonArrayStr) {
  this->_resourcesJsonArray = jsonArrayStr;
  return Success;
}

}  // namespace AlibabaNls
