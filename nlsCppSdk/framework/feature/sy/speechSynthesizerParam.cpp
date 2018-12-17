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

#include "speechSynthesizerParam.h"
#include <cstdlib>
#include "log.h"
#include "nlsRequestParamInfo.h"

using namespace Json;
using std::string;

namespace AlibabaNls {

using namespace util;

#define D_CMD_START_SYNTHESIZER "StartSynthesis"
#define D_NAMESPACE_SYNTHESIZER "SpeechSynthesizer"

SpeechSynthesizerParam::SpeechSynthesizerParam() : INlsRequestParam(SY) {

	_header[D_NAMESPACE] = D_NAMESPACE_SYNTHESIZER;

}

SpeechSynthesizerParam::~SpeechSynthesizerParam() {

}

int SpeechSynthesizerParam::setText(const char* value) {
    if (value == NULL) {
        LOG_ERROR("text is NULL.");
        return -1;
    }

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

int SpeechSynthesizerParam::setMethod(int value) {
	_payload[D_SY_METHOD] = value;
    return 0;
}

const string SpeechSynthesizerParam::getStartCommand() {

    _header[D_NAME] = D_CMD_START_SYNTHESIZER;

    return INlsRequestParam::getStartCommand();

}

const string SpeechSynthesizerParam::getStopCommand() {
    return "";
}

}
