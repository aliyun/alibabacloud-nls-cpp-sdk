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

#ifndef _NLSCPPSDK_RECOGNIZER_EXTERN_H_
#define _NLSCPPSDK_RECOGNIZER_EXTERN_H_

NLSAPI(int) SRstart(AlibabaNls::SpeechRecognizerRequest* request) {
  return request->start();
}

NLSAPI(int) SRstop(AlibabaNls::SpeechRecognizerRequest* request) {
  return request->stop();
}

NLSAPI(int)
SRsendAudio(AlibabaNls::SpeechRecognizerRequest* request, uint8_t* data,
            uint64_t dataSize, int type) {
  return request->sendAudio(data, dataSize, (ENCODER_TYPE)type);
}

// 对外回调
static void onRecognitionStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ConvertNlsEvent(cbEvent, srEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onRecognitionResultChanged(AlibabaNls::NlsEvent* cbEvent,
                                       void* cbParam) {
  ConvertNlsEvent(cbEvent, srEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onRecognitionCompleted(AlibabaNls::NlsEvent* cbEvent,
                                   void* cbParam) {
  ConvertNlsEvent(cbEvent, srEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onRecognitionClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ConvertNlsEvent(cbEvent, srEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onRecognitionTaskFailed(AlibabaNls::NlsEvent* cbEvent,
                                    void* cbParam) {
  ConvertNlsEvent(cbEvent, srEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

NLSAPI(int) SRGetNlsEvent(NLS_EVENT_STRUCT& event) {
  WaitForSingleObject(event.eventMtx, INFINITE);

  event.statusCode = srEvent->statusCode;
  memcpy(event.msg, srEvent->msg, NLS_EVENT_RESPONSE_SIZE);
  event.msgType = srEvent->msgType;
  memcpy(event.taskId, srEvent->taskId, NLS_EVENT_ID_SIZE);
  memcpy(event.result, srEvent->result, NLS_EVENT_RESULT_SIZE);
  memcpy(event.displayText, srEvent->displayText, NLS_EVENT_TEXT_SIZE);
  memcpy(event.spokenText, srEvent->spokenText, NLS_EVENT_TEXT_SIZE);
  event.sentenceTimeOutStatus = srEvent->sentenceTimeOutStatus;
  event.sentenceIndex = srEvent->sentenceIndex;
  event.sentenceTime = srEvent->sentenceTime;
  event.sentenceBeginTime = srEvent->sentenceBeginTime;
  event.sentenceConfidence = srEvent->sentenceConfidence;
  event.wakeWordAccepted = srEvent->wakeWordAccepted;
  event.wakeWordKnown = srEvent->wakeWordKnown;
  memcpy(event.wakeWordUserId, srEvent->wakeWordUserId, NLS_EVENT_ID_SIZE);
  event.wakeWordGender = srEvent->wakeWordGender;
  memcpy(event.binaryData, srEvent->binaryData, NLS_EVENT_BINARY_SIZE);
  event.binaryDataSize = srEvent->binaryDataSize;
  event.stashResultSentenceId = srEvent->stashResultSentenceId;
  event.stashResultBeginTime = srEvent->stashResultBeginTime;
  memcpy(event.stashResultText, srEvent->stashResultText, NLS_EVENT_TEXT_SIZE);
  event.stashResultCurrentTime = srEvent->stashResultBeginTime;
  event.isValid = false;

  CleanNlsEvent(srEvent);

  ReleaseMutex(event.eventMtx);

  return 0;
}

// 设置回调
NLSAPI(int)
SROnRecognitionStarted(AlibabaNls::SpeechRecognizerRequest* request,
                       NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnRecognitionStarted(onRecognitionStarted, (void*)in_param);
  return 0;
}

NLSAPI(int)
SROnRecognitionResultChanged(AlibabaNls::SpeechRecognizerRequest* request,
                             NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnRecognitionResultChanged(onRecognitionResultChanged,
                                         (void*)in_param);
  return 0;
}

NLSAPI(int)
SROnRecognitionCompleted(AlibabaNls::SpeechRecognizerRequest* request,
                         NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnRecognitionCompleted(onRecognitionCompleted, (void*)in_param);
  return 0;
}

NLSAPI(int)
SROnTaskFailed(AlibabaNls::SpeechRecognizerRequest* request,
               NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnTaskFailed(onRecognitionTaskFailed, (void*)in_param);
  return 0;
}

NLSAPI(int)
SROnChannelClosed(AlibabaNls::SpeechRecognizerRequest* request,
                  NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnChannelClosed(onRecognitionClosed, (void*)in_param);
  return 0;
}

// 设置参数
NLSAPI(int)
SRsetUrl(AlibabaNls::SpeechRecognizerRequest* request, const char* value) {
  return request->setUrl(value);
}

NLSAPI(int)
SRsetAppKey(AlibabaNls::SpeechRecognizerRequest* request, const char* value) {
  return request->setAppKey(value);
}

NLSAPI(int)
SRsetToken(AlibabaNls::SpeechRecognizerRequest* request, const char* value) {
  return request->setToken(value);
}

NLSAPI(int)
SRsetFormat(AlibabaNls::SpeechRecognizerRequest* request, const char* value) {
  return request->setFormat(value);
}

NLSAPI(int)
SRsetSampleRate(AlibabaNls::SpeechRecognizerRequest* request, int value) {
  return request->setSampleRate(value);
}

NLSAPI(int)
SRsetIntermediateResult(AlibabaNls::SpeechRecognizerRequest* request,
                        bool value) {
  return request->setIntermediateResult(value);
}

NLSAPI(int)
SRsetPunctuationPrediction(AlibabaNls::SpeechRecognizerRequest* request,
                           bool value) {
  return request->setPunctuationPrediction(value);
}

NLSAPI(int)
SRsetInverseTextNormalization(AlibabaNls::SpeechRecognizerRequest* request,
                              bool value) {
  return request->setInverseTextNormalization(value);
}

NLSAPI(int)
SRsetEnableVoiceDetection(AlibabaNls::SpeechRecognizerRequest* request,
                          bool value) {
  return request->setEnableVoiceDetection(value);
}

NLSAPI(int)
SRsetMaxStartSilence(AlibabaNls::SpeechRecognizerRequest* request, int value) {
  return request->setMaxStartSilence(value);
}

NLSAPI(int)
SRsetMaxEndSilence(AlibabaNls::SpeechRecognizerRequest* request, int value) {
  return request->setMaxEndSilence(value);
}

NLSAPI(int)
SRsetCustomizationId(AlibabaNls::SpeechRecognizerRequest* request,
                     const char* value) {
  return request->setCustomizationId(value);
}

NLSAPI(int)
SRsetVocabularyId(AlibabaNls::SpeechRecognizerRequest* request,
                  const char* value) {
  return request->setVocabularyId(value);
}

NLSAPI(int)
SRsetTimeout(AlibabaNls::SpeechRecognizerRequest* request, int value) {
  return request->setTimeout(value);
}

NLSAPI(int)
SRsetOutputFormat(AlibabaNls::SpeechRecognizerRequest* request,
                  const char* value) {
  return request->setOutputFormat(value);
}

NLSAPI(const char*)
SRGetOutputFormat(AlibabaNls::SpeechRecognizerRequest* request) {
  const char* format = request->getOutputFormat();
  return format;
}

NLSAPI(int)
SRsetPayloadParam(AlibabaNls::SpeechRecognizerRequest* request,
                  const char* value) {
  return request->setPayloadParam(value);
}

NLSAPI(int)
SRsetContextParam(AlibabaNls::SpeechRecognizerRequest* request,
                  const char* value) {
  return request->setContextParam(value);
}

NLSAPI(int)
SRappendHttpHeaderParam(AlibabaNls::SpeechRecognizerRequest* request,
                        const char* key, const char* value) {
  return request->AppendHttpHeaderParam(key, value);
}

#endif  // _NLSCPPSDK_RECOGNIZER_EXTERN_H_
