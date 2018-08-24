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

#include <sstream>
#include "iWebSocketFrameResultConverter.h"
#include "json/json.h"
#include "util/log.h"

using std::string;
using std::vector;
using std::ostringstream;

using namespace util;
using namespace Json;

IWebSocketFrameResultConverter::IWebSocketFrameResultConverter(string format) :_outputFormat(format) {

}

IWebSocketFrameResultConverter::~IWebSocketFrameResultConverter() {

}

NlsEvent* IWebSocketFrameResultConverter::convertResult(WebsocketFrame& frame) {
	NlsEvent* nlsres = NULL;
	if (frame.type == WsheaderType::BINARY_FRAME && frame.data.size() >= 4) {
		int index = (frame.data[0] << 24 & 0xff000000) | (frame.data[1] << 16 & 0xff0000)
			| (frame.data[2] << 8 & 0xff00) | (frame.data[3] & 0xff);
		vector<unsigned char> data = vector<unsigned char>(frame.data.begin(), frame.data.end());
		nlsres = new NlsEvent(data, 0, NlsEvent::Binary);
	} else if (frame.type == WsheaderType::TEXT_FRAME) {
		Json::Reader reader;
		Json::Value head,root;
		string resp(frame.data.begin(), frame.data.end());
		LOG_INFO("%s",resp.c_str());
		string temp = resp;
		NlsEvent::EventType nameType;
		if (this->_outputFormat == "GBK") {
			temp = Log::UTF8ToGBK(resp);
		}

		try {
            if (!reader.parse(resp, root)) {
				throw ExceptionWithString("Json reader fail", 10000031);
			}

			if (!root["header"].isNull()) {
				head = root["header"];
				if (!head["name"].isNull()) {
					string name = head["name"].asCString();
					if (name == "TaskFailed") {
						nameType = NlsEvent::TaskFailed;
					} else if (name == "RecognitionStarted") {
						nameType = NlsEvent::RecognitionStarted;
					} else if (name == "RecognitionCompleted") {
						nameType = NlsEvent::RecognitionCompleted;
					} else if (name == "RecognitionResultChanged") {
						nameType = NlsEvent::RecognitionResultChanged;
					} else if (name == "TranscriptionStarted") {
						nameType = NlsEvent::TranscriptionStarted;
					} else if (name == "SentenceBegin") {
						nameType = NlsEvent::SentenceBegin;
					} else if (name == "TranscriptionResultChanged") {
						nameType = NlsEvent::TranscriptionResultChanged;
					} else if (name == "SentenceEnd") {
                        nameType = NlsEvent::SentenceEnd;
                    } else if (name == "TranscriptionCompleted") {
                        nameType = NlsEvent::TranscriptionCompleted;
                    } else if (name == "SynthesisStarted") {
                        nameType = NlsEvent::SynthesisStarted;
                    }  else if (name == "SynthesisCompleted") {
						nameType = NlsEvent::SynthesisCompleted;
					}  else {
						LOG_ERROR("%s",resp.c_str());
						throw ExceptionWithString("name of Json invalid", 10000029);
					}
				} else {
					throw ExceptionWithString("Json invalid", 10000030);
				}
			} else {
				throw ExceptionWithString("Json invalid", 10000030);
			}

			if (!head["status"].isNull()) {
				int status_code = head["status"].asInt();
				nlsres = new NlsEvent(temp, status_code, nameType);
			} else {
				throw ExceptionWithString("status of Json invalid", 10000032);
			}
		} catch (ExceptionWithString& e) {
			throw e;
		} catch (...) {
			ostringstream os;
			os << "line : " << __LINE__
				<< __FUNCTION__ << " "
				<< "Json convert fail";
			throw ExceptionWithString(os.str(), 10000033);
		}
	}
	return nlsres;
}
