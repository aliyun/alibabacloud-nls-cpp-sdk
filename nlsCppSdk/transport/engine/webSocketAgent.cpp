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
#include "webSocketAgent.h"
#include "util/log.h"

using std::string;
using std::vector;
using std::ostringstream;
using namespace util;

namespace transport{
	namespace engine {

		webSocketAgent::webSocketAgent(WebSocketTcp *socket) : AsyncBase("webSocketAgent"),
															   _socket(socket) {
			this->_handler = NULL;

			pthread_mutex_init(&m_cancelMutex, NULL);
			canceled = false;
		}

		webSocketAgent::~webSocketAgent() {

            pthread_mutex_destroy(&m_cancelMutex);

			if (_socket != NULL) {
				delete _socket;
				_socket = NULL;
			}
		}

		void webSocketAgent::workloop() {
			WebsocketFrame receivedData;
            vector<unsigned char> frame(0);
            WsheaderType ws;
            int ret = -1;
            int errorCode = 0;
            string msg;

#ifndef _ANDRIOD_
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
//            pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
#endif

			while (canLoopContinue()) {

                frame.clear();
                memset(&ws, 0, sizeof(WsheaderType));

                ret = _socket->RecvFullWebSocketFrame(frame, ws, receivedData, errorCode);

                pthread_mutex_lock(&m_cancelMutex);

                if (!canceled) {
                    if (-1 == ret) {
                        receivedData.type = WsheaderType::CLOSE;
                        receivedData.closecode = -1;

                        std::ostringstream os;
                        os << "Socket recv failed, errorCode: " << errorCode << std::endl;
                        msg = os.str();
                        receivedData.data.insert(receivedData.data.begin(), msg.begin(), msg.end());

                        if (this->_handler) {
                            this->_handler->handlerFrame(receivedData);
                        }
                    } else {
                        if (ws.fin) {
                            if (this->_handler) {
                                this->_handler->handlerFrame(receivedData);
                            }
                        }
                    }
                }

                receivedData.data.clear();

                pthread_mutex_unlock(&m_cancelMutex);
            }
		}

        int webSocketAgent::sendBinary(const unsigned char* data, int len) {
            int ret = _socket->sendBinaryData(len, data, data + len);
			if (ret <= 0) {
				return ret;
			}
			return ret;
		}

		void webSocketAgent::stop() {
		}

		bool webSocketAgent::start() {
			startAsyncBase();
			return true;
		}

		int webSocketAgent::onErrorCatched(string msg) {

            if (!canceled) {
                WebsocketFrame receivedData;

                receivedData.type = WsheaderType::CLOSE;
                receivedData.closecode = -1;
                receivedData.data.insert(receivedData.data.begin(), msg.begin(), msg.end());

                if (this->_handler) {
                    this->_handler->handlerFrame(receivedData);
                }
            }

            return 0;
		}

		void webSocketAgent::setDataHandler(HandleBaseOneParamWithReturnVoid<WebsocketFrame> *ptr) {
			this->_handler = ptr;
		}

		void webSocketAgent::close() {
			stopAsyncBase();

			if (_socket) {
				_socket->CloseSsl();
				_socket->Shutdown();
			}
		}

		void webSocketAgent::cancle() {

            stopAsyncBase();

            if (_socket) {
                _socket->CloseSsl();
                _socket->Shutdown();
            }

			pthread_mutex_lock(&m_cancelMutex);

			canceled = true;

            cancleAsyncBase();

			pthread_mutex_unlock(&m_cancelMutex);

            pthread_join(_workingThread, NULL);
		}

        int webSocketAgent::setSocketTimeOut(int timeOut) {
            return _socket->SetSocketRecvTimeOut(timeOut);
        }

		int webSocketAgent::sendText(string req) {
			return _socket->sendTextData(req.size(), req.begin(), req.end());
        }
	}
}
