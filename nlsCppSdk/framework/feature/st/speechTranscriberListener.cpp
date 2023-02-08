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

#include "speechTranscriberListener.h"
#include "speechTranscriberRequest.h"
#include "nlog.h"

namespace AlibabaNls {

SpeechTranscriberListener::SpeechTranscriberListener(
    SpeechTranscriberCallback* cb) : _callback(cb) {}

SpeechTranscriberListener::~SpeechTranscriberListener() {}

void SpeechTranscriberListener::handlerFrame(NlsEvent str) {
  NlsEvent::EventType type = str.getMsgType();

  switch(type) {
    case NlsEvent::TranscriptionStarted:
      if (NULL != _callback->_onTranscriptionStarted) {
        _callback->_onTranscriptionStarted(
            &str, _callback->_paramap[NlsEvent::TranscriptionStarted]);
      }
      break;
    case NlsEvent::SentenceBegin:
      if (NULL != _callback->_onSentenceBegin) {
        _callback->_onSentenceBegin(
            &str, _callback->_paramap[NlsEvent::SentenceBegin]);
      }
      break;
    case NlsEvent::TranscriptionResultChanged:
      if (NULL != _callback->_onTranscriptionResultChanged) {
        _callback->_onTranscriptionResultChanged(
            &str, _callback->_paramap[NlsEvent::TranscriptionResultChanged]);
      }
      break;
    case NlsEvent::SentenceEnd:
      if (NULL != _callback->_onSentenceEnd) {
        _callback->_onSentenceEnd(
            &str, _callback->_paramap[NlsEvent::SentenceEnd]);
      }
      break;
    case NlsEvent::SentenceSemantics:
      if (NULL != _callback->_onSentenceSemantics) {
        _callback->_onSentenceSemantics(&str, _callback->_paramap[NlsEvent::SentenceSemantics]);
      }
      break;
    case NlsEvent::TranscriptionCompleted:
      if (NULL != _callback->_onTranscriptionCompleted) {
        _callback->_onTranscriptionCompleted(
            &str, _callback->_paramap[NlsEvent::TranscriptionCompleted]);
      }
      break;
    case NlsEvent::Message:
      if (NULL != _callback->_onMessage) {
        _callback->_onMessage(&str, _callback->_paramap[NlsEvent::Message]);
      }
      break;
    case NlsEvent::Close:
      if (NULL != _callback->_onChannelClosed) {
        _callback->_onChannelClosed(
            &str, _callback->_paramap[NlsEvent::Close]);
      }
      break;
    default:
      if (NULL != _callback->_onTaskFailed) {
        _callback->_onTaskFailed(&str, _callback->_paramap[NlsEvent::TaskFailed]);
      }
      break;
  }

  return;
}

}
