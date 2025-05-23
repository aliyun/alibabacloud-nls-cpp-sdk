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

#include "dialogAssistantListener.h"

#include "dialogAssistantRequest.h"
#include "nlog.h"

namespace AlibabaNls {

DialogAssistantListener::DialogAssistantListener(DialogAssistantCallback* cb)
    : _callback(cb) {}

DialogAssistantListener::~DialogAssistantListener() {}

void DialogAssistantListener::handlerFrame(NlsEvent& str) {
  NlsEvent::EventType type = str.getMsgType();

  if (NULL == _callback) {
    LOG_ERROR("the callback is NULL");
    return;
  }

  switch (type) {
    case NlsEvent::RecognitionStarted:
      if (NULL != _callback->_onRecognitionStarted) {
        _callback->_onRecognitionStarted(
            &str, _callback->_paramap[NlsEvent::RecognitionStarted]);
      }
      break;
    case NlsEvent::RecognitionCompleted:
      if (NULL != _callback->_onRecognitionCompleted) {
        _callback->_onRecognitionCompleted(
            &str, _callback->_paramap[NlsEvent::RecognitionCompleted]);
      }
      break;
    case NlsEvent::RecognitionResultChanged:
      if (NULL != _callback->_onRecognitionResultChanged) {
        _callback->_onRecognitionResultChanged(
            &str, _callback->_paramap[NlsEvent::RecognitionResultChanged]);
      }
      break;
    case NlsEvent::DialogResultGenerated:
      if (NULL != _callback->_onDialogResultGenerated) {
        _callback->_onDialogResultGenerated(
            &str, _callback->_paramap[NlsEvent::DialogResultGenerated]);
      }
      break;
    case NlsEvent::Close:
      if (NULL != _callback->_onChannelClosed) {
        _callback->_onChannelClosed(&str, _callback->_paramap[NlsEvent::Close]);
      }
      break;
    case NlsEvent::WakeWordVerificationCompleted:
      if (NULL != _callback->_onWakeWordVerificationCompleted) {
        _callback->_onWakeWordVerificationCompleted(
            &str, _callback->_paramap[NlsEvent::WakeWordVerificationCompleted]);
      }
      break;
    default:
      if (NULL != _callback->_onTaskFailed) {
        _callback->_onTaskFailed(&str,
                                 _callback->_paramap[NlsEvent::TaskFailed]);
      }
      break;
  }

  return;
}

}  // namespace AlibabaNls
