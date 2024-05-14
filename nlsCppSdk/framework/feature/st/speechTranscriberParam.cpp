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

#include "speechTranscriberParam.h"

#include <string.h>

#include "nlog.h"
#include "nlsRequestParamInfo.h"

namespace AlibabaNls {

#define D_CMD_START_TRANSCRIPTION   "StartTranscription"
#define D_CMD_CONTROL_TRANSCRIPTION "ControlTranscriber"
#define D_CMD_STOP_TRANSCRIPTION    "StopTranscription"
#define D_NAMESPACE_TRANSCRIPTION   "SpeechTranscriber"

SpeechTranscriberParam::SpeechTranscriberParam(const char* sdkName)
    : INlsRequestParam(TypeRealTime, sdkName) {
  _header[D_NAMESPACE] = D_NAMESPACE_TRANSCRIPTION;
  _controlHeaderName.clear();
}

SpeechTranscriberParam::~SpeechTranscriberParam() {}

const char* SpeechTranscriberParam::getStartCommand() {
  _header[D_NAME] = D_CMD_START_TRANSCRIPTION;
  return INlsRequestParam::getStartCommand();
}

const char* SpeechTranscriberParam::getControlCommand(const char* message) {
  if (!_controlHeaderName.empty()) {
    _header[D_NAME] = _controlHeaderName.c_str();
  } else {
    _header[D_NAME] = D_CMD_CONTROL_TRANSCRIPTION;
  }
  return INlsRequestParam::getControlCommand(message);
}

const char* SpeechTranscriberParam::getStopCommand() {
  _header[D_NAME] = D_CMD_STOP_TRANSCRIPTION;
  return INlsRequestParam::getStopCommand();
}

int SpeechTranscriberParam::setControlHeaderName(const char* name) {
  if (name && strlen(name) > 0) {
    _controlHeaderName.assign(name);
  } else {
    _controlHeaderName.clear();
  }
  return Success;
}

int SpeechTranscriberParam::setMaxSentenceSilence(int value) {
  _payload[D_ST_MAX_SENTENCE_SILENCE] = value;
  return Success;
}

int SpeechTranscriberParam::setEnableNlp(bool enable) {
  _payload[D_ST_ENABLE_NLP] = enable;
  return Success;
}

int SpeechTranscriberParam::setNlpModel(const char* value) {
  _payload[D_ST_NLP_MODEL] = value;
  return Success;
}

int SpeechTranscriberParam::setEnableWords(bool enable) {
  _payload[D_ST_ENABLE_WORDS] = enable;
  return Success;
}

int SpeechTranscriberParam::setEnableIgnoreSentenceTimeout(bool enable) {
  _payload[D_ST_IGNORE_SENTENCE_TIMEOUT] = enable;
  return Success;
}

int SpeechTranscriberParam::setDisfluency(bool enable) {
  _payload[D_ST_DISFLUENCY] = enable;
  return Success;
}

int SpeechTranscriberParam::setSpeechNoiseThreshold(float value) {
  _payload[D_ST_SPEECH_NOISE_THRESHOLD] = value;
  return Success;
}

}  // namespace AlibabaNls
