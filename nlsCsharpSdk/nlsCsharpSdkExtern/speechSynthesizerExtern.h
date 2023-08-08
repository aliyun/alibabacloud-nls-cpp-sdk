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

#ifndef _NLSCPPSDK_SYNTHESIZER_EXTERN_H_
#define _NLSCPPSDK_SYNTHESIZER_EXTERN_H_

NLSAPI(int) SYstart(AlibabaNls::SpeechSynthesizerRequest* request) {
  return request->start();
}

NLSAPI(int) SYstop(AlibabaNls::SpeechSynthesizerRequest* request) {
  return request->stop();
}

NLSAPI(int) SYcancel(AlibabaNls::SpeechSynthesizerRequest* request) {
  return request->cancel();
}

// 对外回调
static void onSynthesisTaskFailed(AlibabaNls::NlsEvent* cbEvent,
                                  void* cbParam) {
  ConvertNlsEvent(cbEvent, syEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onSynthesisClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ConvertNlsEvent(cbEvent, syEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onSynthesisCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ConvertNlsEvent(cbEvent, syEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onSynthesisMetaInfo(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ConvertNlsEvent(cbEvent, syEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

static void onSynthesisDataReceived(AlibabaNls::NlsEvent* cbEvent,
                                    void* cbParam) {
  ConvertNlsEvent(cbEvent, syEvent);
  if (cbParam) {
    UserCallback* in_param = (UserCallback*)cbParam;
    if (in_param->delegate_callback) {
      in_param->delegate_callback(in_param->user_handler);
    }
  }
  return;
}

NLSAPI(int) SYGetNlsEvent(NLS_EVENT_STRUCT& event) {
  WaitForSingleObject(event.eventMtx, INFINITE);

  event.statusCode = syEvent->statusCode;
  memcpy(event.msg, syEvent->msg, NLS_EVENT_RESPONSE_SIZE);
  event.msgType = syEvent->msgType;
  memcpy(event.taskId, syEvent->taskId, NLS_EVENT_ID_SIZE);
  memcpy(event.result, syEvent->result, NLS_EVENT_RESULT_SIZE);
  memcpy(event.displayText, syEvent->displayText, NLS_EVENT_TEXT_SIZE);
  memcpy(event.spokenText, syEvent->spokenText, NLS_EVENT_TEXT_SIZE);
  event.sentenceTimeOutStatus = syEvent->sentenceTimeOutStatus;
  event.sentenceIndex = syEvent->sentenceIndex;
  event.sentenceTime = syEvent->sentenceTime;
  event.sentenceBeginTime = syEvent->sentenceBeginTime;
  event.sentenceConfidence = syEvent->sentenceConfidence;
  event.wakeWordAccepted = syEvent->wakeWordAccepted;
  event.wakeWordKnown = syEvent->wakeWordKnown;
  memcpy(event.wakeWordUserId, syEvent->wakeWordUserId, NLS_EVENT_ID_SIZE);
  event.wakeWordGender = syEvent->wakeWordGender;
  memcpy(event.binaryData, syEvent->binaryData, NLS_EVENT_BINARY_SIZE);
  event.binaryDataSize = syEvent->binaryDataSize;
  syEvent->binaryDataSize = 0;
  event.stashResultSentenceId = syEvent->stashResultSentenceId;
  event.stashResultBeginTime = syEvent->stashResultBeginTime;
  memcpy(event.stashResultText, syEvent->stashResultText, NLS_EVENT_TEXT_SIZE);
  event.stashResultCurrentTime = syEvent->stashResultBeginTime;
  event.isValid = false;

  CleanNlsEvent(syEvent);

  ReleaseMutex(event.eventMtx);
  return event.binaryDataSize;
}

// 设置回调
NLSAPI(int)
SYOnSynthesisCompleted(AlibabaNls::SpeechSynthesizerRequest* request,
                       NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnSynthesisCompleted(onSynthesisCompleted, (void*)in_param);
  return 0;
}

NLSAPI(int)
SYOnTaskFailed(AlibabaNls::SpeechSynthesizerRequest* request,
               NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnTaskFailed(onSynthesisTaskFailed, (void*)in_param);
  return 0;
}

NLSAPI(int)
SYOnChannelClosed(AlibabaNls::SpeechSynthesizerRequest* request,
                  NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnChannelClosed(onSynthesisClosed, (void*)in_param);
  return 0;
}

NLSAPI(int)
SYOnBinaryDataReceived(AlibabaNls::SpeechSynthesizerRequest* request,
                       NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnBinaryDataReceived(onSynthesisDataReceived, (void*)in_param);
  return 0;
}

NLSAPI(int)
SYOnMetaInfo(AlibabaNls::SpeechSynthesizerRequest* request,
             NlsCallbackDelegate c, void* user) {
  UserCallback* in_param = new UserCallback;
  in_param->delegate_callback = c;
  in_param->user_handler = user;
  request->setOnMetaInfo(onSynthesisMetaInfo, (void*)in_param);
  return 0;
}

// 设置参数
NLSAPI(int)
SYsetUrl(AlibabaNls::SpeechSynthesizerRequest* request, const char* value) {
  return request->setUrl(value);
}

NLSAPI(int)
SYsetAppKey(AlibabaNls::SpeechSynthesizerRequest* request, const char* value) {
  return request->setAppKey(value);
}

NLSAPI(int)
SYsetToken(AlibabaNls::SpeechSynthesizerRequest* request, const char* value) {
  return request->setToken(value);
}

NLSAPI(int)
SYsetFormat(AlibabaNls::SpeechSynthesizerRequest* request, const char* value) {
  return request->setFormat(value);
}

NLSAPI(int)
SYsetSampleRate(AlibabaNls::SpeechSynthesizerRequest* request, int value) {
  return request->setSampleRate(value);
}

NLSAPI(int)
SYsetText(AlibabaNls::SpeechSynthesizerRequest* request, uint8_t* text,
          uint32_t textSize) {
  char* textChar = new char[textSize + 1];
  memset(textChar, 0, textSize + 1);
  memcpy(textChar, text, textSize);
  int result = request->setText((const char*)textChar);
  delete[] textChar;
  return result;
}

NLSAPI(int)
SYsetVoice(AlibabaNls::SpeechSynthesizerRequest* request, const char* value) {
  return request->setVoice(value);
}

NLSAPI(int)
SYsetVolume(AlibabaNls::SpeechSynthesizerRequest* request, int value) {
  return request->setVolume(value);
}

NLSAPI(int)
SYsetSpeechRate(AlibabaNls::SpeechSynthesizerRequest* request, int value) {
  return request->setSpeechRate(value);
}

NLSAPI(int)
SYsetPitchRate(AlibabaNls::SpeechSynthesizerRequest* request, int value) {
  return request->setPitchRate(value);
}

NLSAPI(int)
SYsetMethod(AlibabaNls::SpeechSynthesizerRequest* request, int value) {
  return request->setMethod(value);
}

NLSAPI(int)
SYsetEnableSubtitle(AlibabaNls::SpeechSynthesizerRequest* request, bool value) {
  return request->setEnableSubtitle(value);
}

NLSAPI(int)
SYsetPayloadParam(AlibabaNls::SpeechSynthesizerRequest* request,
                  const char* value) {
  return request->setPayloadParam(value);
}

NLSAPI(int)
SYsetTimeout(AlibabaNls::SpeechSynthesizerRequest* request, int value) {
  return request->setTimeout(value);
}

NLSAPI(int)
SYsetOutputFormat(AlibabaNls::SpeechSynthesizerRequest* request,
                  const char* value) {
  return request->setOutputFormat(value);
}

NLSAPI(const char*)
SYGetOutputFormat(AlibabaNls::SpeechSynthesizerRequest* request) {
  const char* format = request->getOutputFormat();
  return format;
}

NLSAPI(int)
SYsetContextParam(AlibabaNls::SpeechSynthesizerRequest* request,
                  const char* value) {
  return request->setContextParam(value);
}

NLSAPI(int)
SYappendHttpHeaderParam(AlibabaNls::SpeechSynthesizerRequest* request,
                        const char* key, const char* value) {
  return request->AppendHttpHeaderParam(key, value);
}

#endif  // _NLSCPPSDK_SYNTHESIZER_EXTERN_H_
