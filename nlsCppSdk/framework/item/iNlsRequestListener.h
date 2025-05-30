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

#ifndef NLS_SDK_SPEECH_LISTENER_H
#define NLS_SDK_SPEECH_LISTENER_H

#include <string>

#include "nlsEvent.h"
#include "webSocketFrameHandleBase.h"

namespace AlibabaNls {

class INlsRequestListener : public HandleBaseOneParamWithReturnVoid<NlsEvent> {
 public:
  INlsRequestListener();
  ~INlsRequestListener();

  virtual void handlerFrame(NlsEvent&) = 0;
  virtual void handlerFrame(std::string errorInfo, int errorCode,
                            NlsEvent::EventType type, std::string taskId);
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_SPEECH_LISTENER_H
