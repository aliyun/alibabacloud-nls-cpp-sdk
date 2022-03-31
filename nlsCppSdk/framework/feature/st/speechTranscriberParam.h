/*
 * Copyright 2015 Alibaba Group Holding Limited
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

#ifndef NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_PARAM_H
#define NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_PARAM_H

#include <string>
#include "iNlsRequestParam.h"

namespace AlibabaNls {

class SpeechTranscriberParam : public INlsRequestParam {

public:
    SpeechTranscriberParam(const char* sdkName = "cpp");
    ~SpeechTranscriberParam();

    const char*  getStartCommand();
    const char*  getStopCommand();
    const char*  getControlCommand(const char* message);

    int setMaxSentenceSilence(int value);
    int setEnableNlp(bool enable);
    int setNlpModel(const char* value);
    int setEnableWords(bool enable);
    int setEnableIgnoreSentenceTimeout(bool enable);
    int setDisfluency(bool enable);
    int setSpeechNoiseThreshold(float value);
};

}

#endif //NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_PARAM_H
