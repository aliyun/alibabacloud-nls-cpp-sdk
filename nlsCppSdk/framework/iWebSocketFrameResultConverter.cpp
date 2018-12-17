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

#include "iWebSocketFrameResultConverter.h"
#include <sstream>
#include "json/json.h"
#include "log.h"

using std::string;
using std::vector;
using std::ostringstream;

using namespace Json;

namespace AlibabaNls {

using namespace util;

IWebSocketFrameResultConverter::IWebSocketFrameResultConverter(string format, string taskId)
	:_outputFormat(format), _taskId(taskId) {

}

IWebSocketFrameResultConverter::~IWebSocketFrameResultConverter() {

}

NlsEvent* IWebSocketFrameResultConverter::convertResult(WebsocketFrame& frame) {
	NlsEvent* nlsres = NULL;
	if (frame.type == WsheaderType::BINARY_FRAME && frame.data.size() >= 4) {
		//int index = (frame.data[0] << 24 & 0xff000000) | (frame.data[1] << 16 & 0xff0000)
		//	| (frame.data[2] << 8 & 0xff00) | (frame.data[3] & 0xff);
		vector<unsigned char> data = vector<unsigned char>(frame.data.begin(), frame.data.end());
		nlsres = new NlsEvent(data, 0, NlsEvent::Binary, _taskId);
	} else if (frame.type == WsheaderType::TEXT_FRAME) {
		string resp(frame.data.begin(), frame.data.end());

		LOG_INFO("Response: %s",resp.c_str());

		if ("GBK" == _outputFormat) {
			resp = Log::UTF8ToGBK(resp);
		}
		nlsres = new NlsEvent(resp);

		int ret = nlsres->parseJsonMsg();
		if (ret < 0) {
			delete nlsres;
			nlsres = NULL;
			throw ExceptionWithString("JSON: Json parse failed.", 10000007);
		}
	} else {
        throw ExceptionWithString("WEBSOCKET: unkown head type.", 10000008);
	}

	return nlsres;
}

}
