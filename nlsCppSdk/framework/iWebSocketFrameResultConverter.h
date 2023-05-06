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

#ifndef NLS_SDK_SESSION_SPEECH_RECOGNIZER_H
#define NLS_SDK_SESSION_SPEECH_RECOGNIZER_H

#include "dataStruct.h"
#include "nlsEvent.h"

namespace AlibabaNls {

class IWebSocketFrameResultConverter {
public:
	IWebSocketFrameResultConverter(std::string, std::string taskId);
	virtual ~IWebSocketFrameResultConverter();
	virtual NlsEvent* convertResult(util::WebsocketFrame& frame);
	std::string _outputFormat;
	std::string _taskId;
};

}

#endif
