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
#include "speechRecognizerParam.h"
#include "nlsRequestParamInfo.h"
#include "nlog.h"

namespace AlibabaNls {

#define D_CMD_START_RECOGNITION "StartRecognition"
#define D_CMD_STOP_RECOGNITION "StopRecognition"
#define D_NAMESPACE_RECOGNITION "SpeechRecognizer"

SpeechRecognizerParam::SpeechRecognizerParam() : INlsRequestParam(TypeAsr) {
	_header[D_NAMESPACE] = D_NAMESPACE_RECOGNITION;
}

SpeechRecognizerParam::~SpeechRecognizerParam() {}

const char* SpeechRecognizerParam::getStartCommand() {
  _header[D_NAME] = D_CMD_START_RECOGNITION;
  LOG_DEBUG("SpeechRecognizerParam Start.");
  return INlsRequestParam::getStartCommand();
}

const char* SpeechRecognizerParam::getStopCommand() {
  _header[D_NAME] = D_CMD_STOP_RECOGNITION;
  LOG_DEBUG("SpeechRecognizerParam Stop.");
  return INlsRequestParam::getStopCommand();
}

int SpeechRecognizerParam::setEnableVoiceDetection(bool value) {
  _payload[D_SR_VOICE_DETECTION] = value;
  return 0;
}

int SpeechRecognizerParam::setMaxStartSilence(int value) {
  _payload[D_SR_MAX_START_SILENCE] = value;
  return 0;
}

int SpeechRecognizerParam::setMaxEndSilence(int value) {
  _payload[D_SR_MAX_END_SILENCE] = value;
  return 0;
}

}
