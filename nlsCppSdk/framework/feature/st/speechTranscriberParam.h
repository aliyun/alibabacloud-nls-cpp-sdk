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

#include <set>
#include <string>
#include "iNlsRequestParam.h"
#include "json/json.h"
#include "util/dataStruct.h"

class SpeechTranscriberParam : public INlsRequestParam {

public:
    SpeechTranscriberParam();
    ~SpeechTranscriberParam();

    int setIntermediateResult(const char* value);
    int setPunctuationPrediction(const char* value);
    int setTextNormalization(const char* value);

    virtual const std::string getStartCommand();
    virtual const std::string getStopCommand();

    virtual int speechParam(std::string key, std::string value);
    virtual int setContextParam(const char* key, const char* value);
};

#endif //NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_PARAM_H
