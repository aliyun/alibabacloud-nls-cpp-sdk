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


namespace AlibabaNls {

/************************common default value************************/
#define D_DEFAULT_VALUE_ENCODE_UTF8 "UTF-8"
#define D_DEFAULT_VALUE_ENCODE_GBK "GBK"
#define D_DEFAULT_VALUE_AUDIO_ENCODE "pcm"
#define D_DEFAULT_VALUE_SAMPLE_RATE 16000
#define D_DEFAULT_VALUE_BOOL_TRUE "true"
#define D_DEFAULT_VALUE_BOOL_FALSE "false"

/************************common param************************/
#define D_HEADER      "header"
#define D_PAYLOAD     "payload"
#define D_CONTEXT     "context"
#define D_FORMAT      "format"
#define D_SAMPLE_RATE "sample_rate"
#define D_APP_KEY     "appkey"
#define D_MESSAGE_ID  "message_id"
#define D_TASK_ID     "task_id"
#define D_NAMESPACE   "namespace"
#define D_NAME        "name"

/************************speech recognizer************************/
#define D_SR_VOICE_DETECTION         "enable_voice_detection"
#define D_SR_MAX_START_SILENCE       "max_start_silence"
#define D_SR_MAX_END_SILENCE         "max_end_silence"

#define D_SR_INTERMEDIATE_RESULT     "enable_intermediate_result"
#define D_SR_PUNCTUATION_PREDICTION  "enable_punctuation_prediction"
#define D_SR_TEXT_NORMALIZATION      "enable_inverse_text_normalization"

#define D_SR_CUSTOMIZATION_ID        "customization_id"
#define D_SR_VOCABULARY_ID           "vocabulary_id"
#define D_SR_AUDIO_ADDRESS           "audio_address"

/************************speech transcriber************************/
#define D_ST_SENTENCE_DETECTION      "enable_semantic_sentence_detection"
#define D_ST_MAX_SENTENCE_SILENCE    "max_sentence_silence"

#define D_ST_ENABLE_NLP              "enable_nlp"
#define D_ST_NLP_MODEL               "nlp_model"

#define D_ST_ENABLE_WORDS            "enable_words"
#define D_ST_IGNORE_SENTENCE_TIMEOUT "enable_ignore_sentence_timeout"
#define D_ST_DISFLUENCY              "disfluency"
#define D_ST_SPEECH_NOISE_THRESHOLD  "speech_noise_threshold"

/************************speech synthesizer************************/
#define D_SY_VOICE           "voice"
#define D_SY_VOLUME          "volume"
#define D_SY_SPEECH_RATE     "speech_rate"
#define D_SY_PITCH_RATE      "pitch_rate"
#define D_SY_ENABLE_SUBTITLE "enable_subtitle"
#define D_SY_METHOD          "method"

#define D_SY_TEXT            "text"

/**************************dialog assistant**************************/
#define D_DA_SESSION_ID             "session_id"
#define D_DA_QUERY                  "query"
#define D_DA_QUERY_PARAMS           "query_params"
#define D_DA_QUERY_CONTEXT          "query_context"

#define D_DA_WAKE_WORD_VERIFICATION "enable_wake_word_verification"
#define D_DA_WAKE_WORD              "wake_word"
#define D_DA_WAKE_WORD_MODEL        "wake_word_model"

#define D_DA_ENABLE_MUTI_GROUP      "enable_multi_group"

/**************************sdk infomation**************************/
#define D_SDK_CLIENT   "sdk"
#define D_SDK_NAME     "name"
#define D_SDK_VERSION  "version"
#define D_SDK_LANGUAGE "language"
#define D_CUSTOM_PARAM "customParam"

}

#endif // NLS_SDK_REQUEST_PARAM_INFO_H
