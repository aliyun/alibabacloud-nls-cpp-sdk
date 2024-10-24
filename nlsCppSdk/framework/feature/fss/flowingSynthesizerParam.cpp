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

#include "flowingSynthesizerParam.h"

#include <string.h>

#include <cstdlib>

#include "nlog.h"
#include "nlsRequestParamInfo.h"
#include "text_utils.h"

namespace AlibabaNls {

#define D_CMD_START_SYNTHESIZER "StartSynthesis"
#define D_NAMESPACE_FLOWING_SYNTHESIZER "FlowingSpeechSynthesizer"
#define D_CMD_RUN_SYNTHESIS "RunSynthesis"
#define D_CMD_FLUSH_TEXT "FlushText"
#define D_CMD_STOP_SYNTHESIS "StopSynthesis"

FlowingSynthesizerParam::FlowingSynthesizerParam(const char* sdkName)
    : INlsRequestParam(TypeStreamInputTts, sdkName),
      _runFlowingSynthesisCommand(""),
      _flushFlowingTextCommand("") {
  _header[D_NAMESPACE] = D_NAMESPACE_FLOWING_SYNTHESIZER;
}

FlowingSynthesizerParam::~FlowingSynthesizerParam() {}

int FlowingSynthesizerParam::setVoice(const char* value) {
  if (value == NULL) {
    return -(InvalidInputParam);
  }
  _payload[D_SY_VOICE] = value;
  return Success;
}

int FlowingSynthesizerParam::setVolume(int value) {
  _payload[D_SY_VOLUME] = value;
  return Success;
}

int FlowingSynthesizerParam::setSpeechRate(int value) {
  _payload[D_SY_SPEECH_RATE] = value;
  return Success;
}

int FlowingSynthesizerParam::setPitchRate(int value) {
  _payload[D_SY_PITCH_RATE] = value;
  return Success;
}

void FlowingSynthesizerParam::setEnableSubtitle(bool value) {
  _payload[D_SY_ENABLE_SUBTITLE] = value;
}

const char* FlowingSynthesizerParam::getStartCommand() {
  _header[D_NAME] = D_CMD_START_SYNTHESIZER;
  return INlsRequestParam::getStartCommand();
}

const char* FlowingSynthesizerParam::getStopCommand() {
  _header[D_NAME] = D_CMD_STOP_SYNTHESIS;
  return INlsRequestParam::getStopCommand();
}

const char* FlowingSynthesizerParam::getRunFlowingSynthesisCommand(
    const char* text) {
  Json::Value root;
  Json::FastWriter writer;
  _header[D_NAME] = D_CMD_RUN_SYNTHESIS;

  try {
    _header[D_TASK_ID] = _task_id;
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();
    _payload[D_SY_TEXT] = text;

    root[D_HEADER] = _header;
    root[D_PAYLOAD] = _payload;

    _runFlowingSynthesisCommand = writer.write(root);
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  return _runFlowingSynthesisCommand.c_str();
}

const char* FlowingSynthesizerParam::getFlushFlowingTextCommand() {
  Json::Value root;
  Json::FastWriter writer;
  _header[D_NAME] = D_CMD_FLUSH_TEXT;

  try {
    _header[D_TASK_ID] = _task_id;
    _header[D_MESSAGE_ID] = utility::TextUtils::getRandomUuid();

    root[D_HEADER] = _header;

    _flushFlowingTextCommand = writer.write(root);
  } catch (const std::exception& e) {
    LOG_ERROR("Json failed: %s", e.what());
    return NULL;
  }
  return _flushFlowingTextCommand.c_str();
}

}  // namespace AlibabaNls
