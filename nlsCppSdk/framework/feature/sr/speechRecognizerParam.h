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

#ifndef NLS_SDK_SPEECH_RECOGNIZER_REQUEST_PARAM_H
#define NLS_SDK_SPEECH_RECOGNIZER_REQUEST_PARAM_H

#include "iNlsRequestParam.h"

namespace AlibabaNls {

class SpeechRecognizerParam : public INlsRequestParam {

 public:
  SpeechRecognizerParam(const char* sdkName = "cpp");
  ~SpeechRecognizerParam();

  const char* getStartCommand();
  const char* getStopCommand();

  int setEnableVoiceDetection(bool value);
  int setMaxStartSilence(int value);
  int setMaxEndSilence(int value);
  int setAudioAddress(const char* value);
};

}

#endif //NLS_SDK_SPEECH_RECOGNIZER_REQUEST_PARAM_H
