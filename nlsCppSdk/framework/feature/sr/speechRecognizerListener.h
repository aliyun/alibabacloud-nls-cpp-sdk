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

#ifndef NLS_SDK_SPEECH_RECOGNIZER_LISTENER_H
#define NLS_SDK_SPEECH_RECOGNIZER_LISTENER_H

#if defined(_WIN32)
#pragma warning( push )
#pragma warning ( disable : 4251 )
#endif

#include "iNlsRequestListener.h"

namespace AlibabaNls {

class SpeechRecognizerCallback;

class SpeechRecognizerListener : public INlsRequestListener {
public:

SpeechRecognizerListener(SpeechRecognizerCallback* cb);

~SpeechRecognizerListener();

virtual void handlerFrame(NlsEvent);

private:
SpeechRecognizerCallback* _callback;

};

}

#if defined(_WIN32)
#pragma warning( pop )
#endif

#endif //NLS_SDK_SPEECH_RECOGNIZER_LISTENER_H
