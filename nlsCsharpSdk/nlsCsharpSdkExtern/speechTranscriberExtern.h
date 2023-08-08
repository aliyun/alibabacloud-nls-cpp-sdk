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

#ifndef _NLSCPPSDK_TRANSCRIBER_EXTERN_H_
#define _NLSCPPSDK_TRANSCRIBER_EXTERN_H_

NLSAPI(int) STstart(AlibabaNls::SpeechTranscriberRequest* request) {
  return request->start();
}

NLSAPI(int) STstop(AlibabaNls::SpeechTranscriberRequest* request) {
  return request->stop();
}

NLSAPI(int)
STsendAudio(AlibabaNls::SpeechTranscriberRequest* request, uint8_t* data,
            uint64_t dataSize, int type) {
  return request->sendAudio(data, dataSize, (ENCODER_TYPE)type);
}

// 对外回调
static void onTranscriptionStarted(AlibabaNls::NlsEvent* cbEvent,
                                   void* cbParam) {
  ConvertNlsEvent(cbEvent, stEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onTranscriptionResultChanged(AlibabaNls::NlsEvent* cbEvent,
                                         void* cbParam) {
  ConvertNlsEvent(cbEvent, stEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onTranscriptionCompleted(AlibabaNls::NlsEvent* cbEvent,
                                     void* cbParam) {
  ConvertNlsEvent(cbEvent, stEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onTranscriptionClosed(AlibabaNls::NlsEvent* cbEvent,
                                  void* cbParam) {
  ConvertNlsEvent(cbEvent, stEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onTranscriptionTaskFailed(AlibabaNls::NlsEvent* cbEvent,
                                      void* cbParam) {
  ConvertNlsEvent(cbEvent, stEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onSentenceBegin(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ConvertNlsEvent(cbEvent, stEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onSentenceEnd(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ConvertNlsEvent(cbEvent, stEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

NLSAPI(int) STGetNlsEvent(NLS_EVENT_STRUCT& event) {
  WaitForSingleObject(event.eventMtx, INFINITE);

  event.statusCode = stEvent->statusCode;
  memcpy(event.msg, stEvent->msg, NLS_EVENT_RESPONSE_SIZE);
  event.msgType = stEvent->msgType;
  memcpy(event.taskId, stEvent->taskId, NLS_EVENT_ID_SIZE);
  memcpy(event.result, stEvent->result, NLS_EVENT_RESULT_SIZE);
  memcpy(event.displayText, stEvent->displayText, NLS_EVENT_TEXT_SIZE);
  memcpy(event.spokenText, stEvent->spokenText, NLS_EVENT_TEXT_SIZE);
  event.sentenceTimeOutStatus = stEvent->sentenceTimeOutStatus;
  event.sentenceIndex = stEvent->sentenceIndex;
  event.sentenceTime = stEvent->sentenceTime;
  event.sentenceBeginTime = stEvent->sentenceBeginTime;
  event.sentenceConfidence = stEvent->sentenceConfidence;
  event.wakeWordAccepted = stEvent->wakeWordAccepted;
  event.wakeWordKnown = stEvent->wakeWordKnown;
  memcpy(event.wakeWordUserId, stEvent->wakeWordUserId, NLS_EVENT_ID_SIZE);
  event.wakeWordGender = stEvent->wakeWordGender;
  memcpy(event.binaryData, stEvent->binaryData, NLS_EVENT_BINARY_SIZE);
  event.binaryDataSize = stEvent->binaryDataSize;
  event.stashResultSentenceId = stEvent->stashResultSentenceId;
  event.stashResultBeginTime = stEvent->stashResultBeginTime;
  memcpy(event.stashResultText, stEvent->stashResultText, NLS_EVENT_TEXT_SIZE);
  event.stashResultCurrentTime = stEvent->stashResultBeginTime;
  event.isValid = false;

  CleanNlsEvent(stEvent);

  ReleaseMutex(event.eventMtx);

  return 0;
}

// 设置回调
NLSAPI(int)
STOnTranscriptionStarted(AlibabaNls::SpeechTranscriberRequest* request,
                         NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnTranscriptionStarted(onTranscriptionStarted, (void*)in_param);
  return 0;
}

NLSAPI(int)
STOnTranscriptionResultChanged(AlibabaNls::SpeechTranscriberRequest* request,
                               NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnTranscriptionResultChanged(onTranscriptionResultChanged,
                                           (void*)in_param);
  return 0;
}

NLSAPI(int)
STOnTranscriptionCompleted(AlibabaNls::SpeechTranscriberRequest* request,
                           NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnTranscriptionCompleted(onTranscriptionCompleted,
                                       (void*)in_param);
  return 0;
}

NLSAPI(int)
STOnTaskFailed(AlibabaNls::SpeechTranscriberRequest* request,
               NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnTaskFailed(onTranscriptionTaskFailed, (void*)in_param);
  return 0;
}

NLSAPI(int)
STOnChannelClosed(AlibabaNls::SpeechTranscriberRequest* request,
                  NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnChannelClosed(onTranscriptionClosed, (void*)in_param);
  return 0;
}

NLSAPI(int)
STOnSentenceBegin(AlibabaNls::SpeechTranscriberRequest* request,
                  NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnSentenceBegin(onSentenceBegin, (void*)in_param);
  return 0;
}

NLSAPI(int)
STOnSentenceEnd(AlibabaNls::SpeechTranscriberRequest* request,
                NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnSentenceEnd(onSentenceEnd, (void*)in_param);
  return 0;
}

// 设置参数
NLSAPI(int)
STsetUrl(AlibabaNls::SpeechTranscriberRequest* request, const char* value) {
  return request->setUrl(value);
}

NLSAPI(int)
STsetAppKey(AlibabaNls::SpeechTranscriberRequest* request, const char* value) {
  return request->setAppKey(value);
}

NLSAPI(int)
STsetToken(AlibabaNls::SpeechTranscriberRequest* request, const char* value) {
  return request->setToken(value);
}

NLSAPI(int)
STsetFormat(AlibabaNls::SpeechTranscriberRequest* request, const char* value) {
  return request->setFormat(value);
}

NLSAPI(int)
STsetSampleRate(AlibabaNls::SpeechTranscriberRequest* request, int value) {
  return request->setSampleRate(value);
}

NLSAPI(int)
STsetIntermediateResult(AlibabaNls::SpeechTranscriberRequest* request,
                        bool value) {
  return request->setIntermediateResult(value);
}

NLSAPI(int)
STsetPunctuationPrediction(AlibabaNls::SpeechTranscriberRequest* request,
                           bool value) {
  return request->setPunctuationPrediction(value);
}

NLSAPI(int)
STsetInverseTextNormalization(AlibabaNls::SpeechTranscriberRequest* request,
                              bool value) {
  return request->setInverseTextNormalization(value);
}

NLSAPI(int)
STsetSemanticSentenceDetection(AlibabaNls::SpeechTranscriberRequest* request,
                               bool value) {
  return request->setSemanticSentenceDetection(value);
}

NLSAPI(int)
STsetMaxSentenceSilence(AlibabaNls::SpeechTranscriberRequest* request,
                        int value) {
  return request->setMaxSentenceSilence(value);
}

NLSAPI(int)
STsetCustomizationId(AlibabaNls::SpeechTranscriberRequest* request,
                     const char* value) {
  return request->setCustomizationId(value);
}

NLSAPI(int)
STsetVocabularyId(AlibabaNls::SpeechTranscriberRequest* request,
                  const char* value) {
  return request->setVocabularyId(value);
}

NLSAPI(int)
STsetTimeout(AlibabaNls::SpeechTranscriberRequest* request, int value) {
  return request->setTimeout(value);
}

NLSAPI(int)
STsetEnableNlp(AlibabaNls::SpeechTranscriberRequest* request, bool value) {
  return request->setEnableNlp(value);
}

NLSAPI(int)
STsetNlpModel(AlibabaNls::SpeechTranscriberRequest* request,
              const char* value) {
  return request->setNlpModel(value);
}

NLSAPI(int)
STsetSessionId(AlibabaNls::SpeechTranscriberRequest* request,
               const char* value) {
  return request->setSessionId(value);
}

NLSAPI(int)
STsetOutputFormat(AlibabaNls::SpeechTranscriberRequest* request,
                  const char* value) {
  return request->setOutputFormat(value);
}

NLSAPI(const char*)
STGetOutputFormat(AlibabaNls::SpeechTranscriberRequest* request) {
  const char* format = request->getOutputFormat();
  return format;
}

NLSAPI(int)
STsetPayloadParam(AlibabaNls::SpeechTranscriberRequest* request,
                  const char* value) {
  return request->setPayloadParam(value);
}

NLSAPI(int)
STsetContextParam(AlibabaNls::SpeechTranscriberRequest* request,
                  const char* value) {
  return request->setContextParam(value);
}

NLSAPI(int)
STappendHttpHeaderParam(AlibabaNls::SpeechTranscriberRequest* request,
                        const char* key, const char* value) {
  return request->AppendHttpHeaderParam(key, value);
}

#endif  // _NLSCPPSDK_TRANSCRIBER_EXTERN_H_
