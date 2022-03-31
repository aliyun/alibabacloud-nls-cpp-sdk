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

#include <cstdlib>
#include <string.h>
#include "speechSynthesizerParam.h"
#include "nlsRequestParamInfo.h"
#include "nlog.h"

namespace AlibabaNls {

#define D_CMD_START_SYNTHESIZER "StartSynthesis"
#define D_NAMESPACE_SYNTHESIZER "SpeechSynthesizer"
#define D_NAMESPACE_LONG_SYNTHESIZER "SpeechLongSynthesizer"

SpeechSynthesizerParam::SpeechSynthesizerParam(int version, const char* sdkName) :
    INlsRequestParam(TypeTts, sdkName) {
  if (version == 0) {
    _header[D_NAMESPACE] = D_NAMESPACE_SYNTHESIZER;
  } else {
    _header[D_NAMESPACE] = D_NAMESPACE_LONG_SYNTHESIZER;
  }
}

SpeechSynthesizerParam::~SpeechSynthesizerParam() {}

int SpeechSynthesizerParam::setText(const char* value) {
  if (value == NULL) {
    LOG_ERROR("text is NULL.");
    return -1;
  }

  LOG_DEBUG("setText(%dbytes): %s", strlen(value), value);
  _payload[D_SY_TEXT] = value;

  return 0;
}

int SpeechSynthesizerParam::setVoice(const char* value) {
  if (value == NULL) {
    return -1;
  }

  _payload[D_SY_VOICE] = value;

  return 0;
}

int SpeechSynthesizerParam::setVolume(int value) {
  _payload[D_SY_VOLUME] = value;
  return 0;
}

int SpeechSynthesizerParam::setSpeechRate(int value) {
  _payload[D_SY_SPEECH_RATE] = value;
  return 0;
}

int SpeechSynthesizerParam::setPitchRate(int value) {
  _payload[D_SY_PITCH_RATE] = value;
  return 0;
}

void SpeechSynthesizerParam::setEnableSubtitle(bool value) {
  _payload[D_SY_ENABLE_SUBTITLE] = value;
}

int SpeechSynthesizerParam::setMethod(int value) {
  _payload[D_SY_METHOD] = value;
  return 0;
}

const char* SpeechSynthesizerParam::getStartCommand() {
  _header[D_NAME] = D_CMD_START_SYNTHESIZER;
  return INlsRequestParam::getStartCommand();
}

const char* SpeechSynthesizerParam::getStopCommand() {
  return "";
}

}
