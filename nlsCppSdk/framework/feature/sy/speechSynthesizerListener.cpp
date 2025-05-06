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

#include "speechSynthesizerListener.h"

#include "nlog.h"
#include "speechSynthesizerRequest.h"
#include "text_utils.h"

namespace AlibabaNls {

SpeechSynthesizerListener::SpeechSynthesizerListener(
    SpeechSynthesizerCallback* cb)
    : _callback(cb) {}

SpeechSynthesizerListener::~SpeechSynthesizerListener() {}

void SpeechSynthesizerListener::handlerFrame(NlsEvent& str) {
  NlsEvent::EventType type = str.getMsgType();

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_a, timewait_b, timewait_c, timewait_end;
  uint64_t timewait_b1, timewait_b2, timewait_c0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  timewait_a = utility::TextUtils::GetTimestampMs();
#endif

  if (NULL == _callback) {
    LOG_ERROR("callback is NULL");
    return;
  }

  switch (type) {
    case NlsEvent::SynthesisStarted:
      if (NULL != _callback->_onSynthesisStarted) {
        _callback->_onSynthesisStarted(
            &str, _callback->_paramap[NlsEvent::SynthesisStarted]);
      }
      break;
    case NlsEvent::SynthesisCompleted:
#ifdef ENABLE_NLS_DEBUG_2
      timewait_b = utility::TextUtils::GetTimestampMs();
      if (NULL != _callback->_onSynthesisCompleted) {
        timewait_b1 = utility::TextUtils::GetTimestampMs();
        void* user = _callback->_paramap[NlsEvent::SynthesisCompleted];
        timewait_b2 = utility::TextUtils::GetTimestampMs();
        _callback->_onSynthesisCompleted(&str, user);
        timewait_c0 = utility::TextUtils::GetTimestampMs();
      }
      timewait_c = utility::TextUtils::GetTimestampMs();
      if (timewait_c - timewait_b > 50) {
        LOG_WARN(
            "SynthesisCompleted excessive:%llu, including if:%llu map:%llu "
            "callback:%llu",
            timewait_c - timewait_b, timewait_b1 - timewait_b,
            timewait_b2 - timewait_b1, timewait_c0 - timewait_b2);
      }
#else
      if (NULL != _callback->_onSynthesisCompleted) {
        _callback->_onSynthesisCompleted(
            &str, _callback->_paramap[NlsEvent::SynthesisCompleted]);
      }
#endif
      break;
    case NlsEvent::Close:
      if (NULL != _callback->_onChannelClosed) {
        _callback->_onChannelClosed(&str, _callback->_paramap[NlsEvent::Close]);
      }
      break;
    case NlsEvent::Binary:
#ifdef ENABLE_NLS_DEBUG_2
      timewait_b = utility::TextUtils::GetTimestampMs();
#endif
      if (NULL != _callback->_onBinaryDataReceived) {
        _callback->_onBinaryDataReceived(&str,
                                         _callback->_paramap[NlsEvent::Binary]);
      }
#ifdef ENABLE_NLS_DEBUG_2
      timewait_c = utility::TextUtils::GetTimestampMs();
#endif
      break;
    case NlsEvent::MetaInfo:
      if (NULL != _callback->_onMetaInfo) {
        _callback->_onMetaInfo(&str, _callback->_paramap[NlsEvent::MetaInfo]);
      }
      break;
    case NlsEvent::Message:
      if (NULL != _callback->_onMessage) {
        _callback->_onMessage(&str, _callback->_paramap[NlsEvent::Message]);
      }
      break;
    default:
      if (NULL != _callback->_onTaskFailed) {
        _callback->_onTaskFailed(&str,
                                 _callback->_paramap[NlsEvent::TaskFailed]);
      }
      break;
  }

#ifdef ENABLE_NLS_DEBUG_2
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("type:%d excessive:%llu, including %llu %llu", type,
             timewait_end - timewait_start, timewait_a - timewait_start,
             timewait_c - timewait_b);
  }
#endif
  return;
}

}  // namespace AlibabaNls
