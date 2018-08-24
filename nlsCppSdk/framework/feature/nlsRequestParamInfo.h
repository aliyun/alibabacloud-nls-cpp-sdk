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

#ifndef NLS_SDK_REQUEST_PARAM_INFO_H
#define NLS_SDK_REQUEST_PARAM_INFO_H

#include <string>

/************************common default value************************/
#define D_DEFAULT_VALUE_ENCODE_UTF8 "UTF-8"
#define D_DEFAULT_VALUE_ENCODE_GBK "GBK"
#define D_DEFAULT_VALUE_AUDIO_ENCODE "pcm"
#define D_DEFAULT_VALUE_SAMPLE_RATE 16000
#define D_DEFAULT_VALUE_BOOL_TRUE "true"
#define D_DEFAULT_VALUE_BOOL_FALSE "false"

/************************common param************************/
#define D_URL "url"
#define D_HEADER "header"
#define D_FORMAT "format"
#define D_SAMPLE_RATE "sample_rate"
#define D_APP_KEY "appkey"
#define D_TOKEN "token"
#define D_MESSAGE_ID "message_id"
#define D_TASK_ID "task_id"
#define D_NAMESPACE "namespace"
#define D_NAME "name"

/************************speech recognizer************************/

#define D_SR_INTERMEDIATE_RESULT "enable_intermediate_result"
#define D_SR_PUNCTUATION_PREDICTION "enable_punctuation_prediction"
#define D_SR_TEXT_NORMALIZATION "enable_inverse_text_normalization"

/************************speech synthesizer************************/
#define D_SY_VOICE "voice"
#define D_SY_VOLUME "volume"
#define D_SY_SPEECH_RATE "speech_rate"
#define D_SY_PITCH_RATE "pitch_rate"
#define D_SY_METHOD "method"

#define D_SY_TEXT "text"

/**************************sdk infomation**************************/
#define D_SDK_CLIENT "sdk"
#define D_SDK_NAME "name"
#define D_SDK_VERSION "version"
#define D_SDK_LANGUAGE "language"
#define D_CUSTOM_PARAM "customParam"

#endif
