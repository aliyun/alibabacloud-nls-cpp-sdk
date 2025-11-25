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

#include "dashCosyVoiceSynthesizerParam.h"

#include <string.h>

#include <cstdlib>

#include "nlog.h"
#include "nlsRequestParamInfo.h"
#include "text_utils.h"

namespace AlibabaNls {

DashCosyVoiceSynthesizerParam::DashCosyVoiceSynthesizerParam(
    const char* sdkName)
    : INlsRequestParam(TypeDashSceopCosyVoiceStreamInputTts, sdkName),
      MaximumNumberOfWords(2000),
      _runFlowingSynthesisCommand(""),
      _volume(50),
      _rate(1.0),
      _pitch(1.0),
      _seed(0),
      _singeRoundText(""),
      _languageHintsJsonArray(""),
      _instruction(""),
      _inputJsonObj("{}") {
  this->_task = "tts";
  this->_function = "SpeechSynthesizer";
}

DashCosyVoiceSynthesizerParam::~DashCosyVoiceSynthesizerParam() {}

int DashCosyVoiceSynthesizerParam::setSingleRoundText(const char* value) {
  if (value == NULL) {
    return -(InvalidInputParam);
  }
  int wordCount = utility::TextUtils::CharsCalculate(value);
  if (wordCount > MaximumNumberOfWords || wordCount == 0) {
    return -(InvalidInputParam);
  } else {
    _singeRoundText.assign(value);
  }
  return Success;
}

int DashCosyVoiceSynthesizerParam::setVolume(int value) {
  _volume = value;
  return Success;
}

int DashCosyVoiceSynthesizerParam::setSpeechRate(float rate) {
  _rate = rate;
  return Success;
}

int DashCosyVoiceSynthesizerParam::setPitchRate(float pitch) {
  _pitch = pitch;
  return Success;
}

int DashCosyVoiceSynthesizerParam::setLanguageHints(const char* jsonArrayStr) {
  this->_languageHintsJsonArray = jsonArrayStr;
  return Success;
}

int DashCosyVoiceSynthesizerParam::setInstruction(const char* value) {
  _instruction = value;
  return Success;
}

int DashCosyVoiceSynthesizerParam::setSeed(int seed) {
  _seed = seed;
  return Success;
}

const char* DashCosyVoiceSynthesizerParam::getStartCommand() {
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
  if (!_inputJsonObj.empty()) {
    Json::Value tmp;
    if (reader.parse(_inputJsonObj, tmp)) {
      if (tmp.isObject()) {
        payload["input"] = tmp;
      }
    }
  } else {
    payload["input"] = Json::objectValue;
  }

  // payload.parameters ->
  parameters[D_SY_TEXT_TYPE] = "PlainText";
  parameters[D_SY_VOICE] = this->_voice;
  if (!this->_format.empty()) {
    parameters[D_FORMAT] = this->_format;
  }
  if (this->_sampleRate > 0) {
    parameters[D_SAMPLE_RATE] = this->_sampleRate;
  }
  if (this->_volume >= 0) {
    parameters[D_SY_VOLUME] = this->_volume;
  }
  if (this->_rate >= 0.5) {
    parameters[D_SY_RATE] = this->_rate;
  }
  if (this->_pitch >= 0.5) {
    parameters[D_SY_PITCH] = this->_pitch;
  }
  if (_singeRoundText.empty()) {
    parameters[D_SY_ENABLE_SSML] = this->_enableSsml;
  } else {
    parameters[D_SY_ENABLE_SSML] = true;
  }
  if (this->_bitRate > 0) {
    parameters[D_SY_BIT_RATE] = this->_bitRate;
  }
  parameters[D_SY_ENABLE_WORD_TIMESTAMP] = this->_wordTimestampEnabled;
  parameters[D_SY_SEED] = this->_seed;
  if (!this->_languageHintsJsonArray.empty()) {
    Json::Value tmp;
    if (reader.parse(this->_languageHintsJsonArray, tmp)) {
      if (tmp.isArray()) {
        parameters[D_ST_LANGUAGE_HINTS] = tmp;
      }
    }
  }
  if (!this->_instruction.empty()) {
    parameters[D_SY_INSTRUCTION] = this->_instruction;
  }

  payload["parameters"] = parameters;
  root["header"] = header;
  root["payload"] = payload;
  _startCommand = Json::writeString(writerBuilder, root);
  return _startCommand.c_str();
}

const char* DashCosyVoiceSynthesizerParam::getStopCommand() {
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

const char* DashCosyVoiceSynthesizerParam::getRunFlowingSynthesisCommand(
    const char* text) {
  Json::Value root, header, payload, input(Json::objectValue);
  Json::Reader reader;
  Json::StreamWriterBuilder writerBuilder;
  writerBuilder["indentation"] = "";

  header[D_ACTION] = "continue-task";
  header[D_STREAMING] = this->_streaming;
  header[D_TASK_ID] = this->_taskId;

  input["text"] = text;
  payload["input"] = input;

  root["header"] = header;
  root["payload"] = payload;
  this->_runFlowingSynthesisCommand = Json::writeString(writerBuilder, root);
  return this->_runFlowingSynthesisCommand.c_str();
}

std::string& DashCosyVoiceSynthesizerParam::getSingleRoundText() {
  return _singeRoundText;
}

void DashCosyVoiceSynthesizerParam::clearSingleRoundText() {
  _singeRoundText.clear();
}

}  // namespace AlibabaNls
