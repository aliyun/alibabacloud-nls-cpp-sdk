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

#ifndef NLS_SDK_WEBSOCKET_AGENT_H
#define NLS_SDK_WEBSOCKET_AGENT_H

#include <vector>
#include <string>
#include "asyncBase.h"
#include "util/dataStruct.h"
#include "webSocketTcp.h"
#include "webSocketFrameHandleBase.h"

namespace transport{
	namespace engine {

		class webSocketAgent : public AsyncBase {
		public:
			webSocketAgent(transport::WebSocketTcp *socket);
			~webSocketAgent();

			bool start();
			void stop();
			void close();
			void cancle();

			virtual void workloop();
            int sendBinary(const unsigned char* data, int len);
			int sendText(std::string msg);
			virtual int onErrorCatched(std::string data);
			void setDataHandler(HandleBaseOneParamWithReturnVoid<util::WebsocketFrame> *ptr);

			int setSocketTimeOut(int timeOut);

		private:
			HandleBaseOneParamWithReturnVoid<util::WebsocketFrame> *_handler;

			pthread_mutex_t m_cancelMutex;
			bool canceled;

			transport::WebSocketTcp *_socket;
		};
	}
}

#endif //NLS_SDK_WEBSOCKET_AGENT_H
