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

#include "openssl/err.h"
#include "webSocketTcp.h"
#include <string>
#include <algorithm>
#include "pthread.h"
#include "util/log.h"

#include <signal.h>

using std::string;
using std::vector;
using std::min;
using namespace util;

namespace transport {

const char* TOKEN = "X-NLS-Token";
pthread_mutex_t WebSocketTcp::_sslMutex = PTHREAD_MUTEX_INITIALIZER;

WebSocketTcp::WebSocketTcp(SmartHandle<SOCKET> sockfd, int timeOut, const WebSocketAddress& addr, string token) : Socket(sockfd, timeOut),
                                                                                                                  _useMask(true),
                                                                                                                  _useSSL(false) {
	_ssl = NULL;
	_sslCtx = NULL;

	if (strcmp(addr.type, "wss") == 0 && addr.port == 443) {
		SOCKET fd = sockfd.GetHandle();
		SSL_load_error_strings();
		const SSL_METHOD * sslMethod = SSLv23_client_method();
		if (sslMethod == NULL) {
			throw ExceptionWithString("SSLv23_client_method fail", 10000015);
		}

		_sslCtx = SSL_CTX_new(sslMethod);
		if (_sslCtx == NULL) {
			throw ExceptionWithString("SSL_CTX_new fail", 10000016);
		}

		_ssl = SSL_new(_sslCtx);
		if (_ssl == NULL) {
			throw ExceptionWithString("SSL_new fail", 10000017);
		}

		int nRet = SSL_set_fd(_ssl, fd);
		if (nRet == 0) {
			throw ExceptionWithString("SSL_set_fd fail", 10000027);
		}

		pthread_mutex_lock(&_sslMutex);
		nRet = SSL_connect(_ssl);
		pthread_mutex_unlock(&_sslMutex);

		if (-1 == nRet) {
			int code = 0;
			int err = 0;
			int counter = 10;
			do {
				sleepTime(50);
				err = SSL_get_error(_ssl, nRet);
				code = ERR_get_error();

				pthread_mutex_lock(&_sslMutex);
				nRet = SSL_connect(_ssl);
				pthread_mutex_unlock(&_sslMutex);
			} while (counter-- && (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE));

            if (err == 2) {
				throw ExceptionWithString("ssl connect fail", err);
			}
		}
		_useSSL = true;
	}

	if (ConnectToHttp(addr,token) == false) {
		throw ExceptionWithString("ConnectToHttp fail", 10000005);
	}

    _blockSigpipe = false;

}

int WebSocketTcp::ws_read(void *buf, size_t num) {
	int ret = 0;
	ret = _useSSL == true? SSL_read(_ssl, buf, num) : Recv((unsigned char*)buf, num);

	return ret;
}

int WebSocketTcp::ws_write(const void *buf, size_t num) {
	int ret = 0;
	ret = _useSSL == true ? SSL_write(_ssl, buf, num) : Send((unsigned char*)buf, num);

	return ret;
}

WebSocketTcp::~WebSocketTcp() {
	if (_useSSL) {
		SSL_free(_ssl);
		SSL_CTX_free(_sslCtx);
	}
}

WebSocketTcp* WebSocketTcp::connectTo(const WebSocketAddress& addr, int timeOut, string token) {
	SocketFuncs::Startup();
	SmartHandle<SOCKET> sockfd(socket(AF_INET, SOCK_STREAM, 0));
	string ip;
	bool res = InetAddress::GetInetAddressByHostname(addr.host, ip);
	if (res == false) {
		throw ExceptionWithString("GetInetAddressByHostname fail", 10000028);
	}

	InetAddress ina(ip, addr.port);
	SocketFuncs::connectTo(sockfd.get(), ina);
	WebSocketTcp* tcpSock = new WebSocketTcp(sockfd, timeOut, addr, token);

    return tcpSock;
}

bool WebSocketTcp::ConnectToHttp(const WebSocketAddress addr, string token) {
	char line[256] = {0};
	int status = 0;
	int i = 0;

	snprintf(line, 256, "GET /%s HTTP/1.1\r\n", addr.path);
	ws_write(line, strlen(line));
	if (addr.port == 80) {
		snprintf(line, 256, "Host: %s\r\n", addr.host);
		ws_write(line, strlen(line));
	} else {
		snprintf(line, 256, "Host: %s:%d\r\n", addr.host, addr.port);
		ws_write(line, strlen(line));
	}

	snprintf(line, 256, "Upgrade: websocket\r\n");
	ws_write(line, strlen(line));
	snprintf(line, 256, "Connection: Upgrade\r\n");
	ws_write(line, strlen(line));
	snprintf(line, 256, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n");
	ws_write(line, strlen(line));
	snprintf(line, 256, "Sec-WebSocket-Version: 13\r\n");
	ws_write(line, strlen(line));
	snprintf(line, 256, "%s: %s\r\n", TOKEN, token.c_str());
	ws_write(line, strlen(line));
	snprintf(line, 256, "\r\n");
	ws_write(line, strlen(line));
	LOG_DEBUG("send http head to server");

	memset(line, 0, 256);
	for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i) {
		int ret = ws_read(line + i, 1);
		if (ret <= 0) {
            LOG_DEBUG("handshake: read failed, %d.", ret);
            return false;
        }
	}

	line[i] = 0;
	LOG_DEBUG("http: %s", line);

    if (i == 255) {
		LOG_ERROR("Got invalid status line connecting to: %s", addr.host);
		return false;
	}

	if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101) {
		LOG_WARN("Got bad status connecting to %s: %s", addr.host, line);
		throw ExceptionWithString("Got bad status", status);
	}
	LOG_DEBUG("receive http status response from server");

	while (true) {
		memset(line, 0, 256);
		for (i = 0; i < 2 || (i < 255 && line[i - 2] != '\r' && line[i - 1] != '\n'); ++i) {
			int ret = ws_read(line + i, 1);
			if (ret <= 0) {
                LOG_DEBUG("handshake: read failed, %d.", ret);
				return false;
			}
		}

		LOG_DEBUG("http: %s", line);
		if (line[0] == '\r' && line[1] == '\n') {
			break; 
		}		
	}
	LOG_DEBUG("receive http head response from server");

	return true;
}

int WebSocketTcp::send(uint8_t* data, int len) {

    int nRet = 0;

#if defined(__ANDROID__) || defined(__linux__)
    sigset_t signal_mask;

    if (false == _blockSigpipe) {
        if (-1 == sigemptyset(&signal_mask)) {
            LOG_ERROR("sigemptyset failed.");
        }

        if (-1 == sigaddset(&signal_mask, SIGPIPE)) {
            LOG_ERROR("sigaddset failed.");
        }

        if(pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
            LOG_ERROR("pthread_sigmask failed.");
        }

        _blockSigpipe = true;
    }
#endif

    if (_useSSL == true) {
        nRet = SSL_write(_ssl, data, len);
        if (nRet <= 0) {
            return -1;
        }
    } else {
        nRet = Send(data, len);
        if (nRet < 0) {
            return -1;
        }
    }

	return nRet;
}

int WebSocketTcp::RecvDataBySize(vector<unsigned char>& buffer, int size) {
	int len = 0;
	int ret = 0;

    while (len < size) {
		int temp = ws_read(&buffer[len], min(size, size - len));
		if (temp <= 0) {
            ret = -1;
            break;
		}

//		if (temp <= 0) {
//			throw ExceptionWithString("WebTcp socket has been closed gracefully!", Socket::getLastErrorCode());
//		}
		len += temp;
	}

    return ret;
}

int WebSocketTcp::RecvFullWebSocketFrame(vector<unsigned char>& frame,
                                         WsheaderType& ws,
                                         WebsocketFrame& receivedData,
                                         int& errorCode) {
	//recv header to get header size;
	frame.resize(frame.size() + 2);
	if ( -1 == RecvDataBySize(frame, 2)) {
        errorCode = getLastErrorCode();
        return -1;
    }

	DecodeHeaderSizeWebSocketFrame(frame, ws);
	//recv header body to get body size
	vector<unsigned char> Headerbody(ws.header_size - 2, 0);
    if ( -1 == RecvDataBySize(Headerbody, ws.header_size - 2)) {
        errorCode = getLastErrorCode();
        return -1;
    }

	frame.insert(frame.end(), Headerbody.begin(), Headerbody.end());
	DecodeHeaderBodyWebSocketFrame(frame, ws);

	//recv body
	vector<unsigned char> body(ws.N, 0);
    if ( -1 == RecvDataBySize(body, ws.N)) {
        errorCode = getLastErrorCode();
        return -1;
    }

	frame.insert(frame.end(), body.begin(), body.end());
	if (DecodeFrameBodyWebSocketFrame(frame, ws, receivedData) == -1) {
		errorCode = 10000004;
		return -1;
	}

    return 0;
}

void WebSocketTcp::DecodeHeaderSizeWebSocketFrame(vector<unsigned char> rxbuf, WsheaderType& ws) {
	if (rxbuf.size() < 2) {
		return; /* Need at least 2 */
	}

	const uint8_t *data = &rxbuf[0]; // peek, but don't consume
	ws.fin = (data[0] & 0x80) == 0x80;
	ws.opcode = (WsheaderType::OpcodeType) (data[0] & 0x0f);
	ws.mask = (data[1] & 0x80) == 0x80;
	ws.N0 = (data[1] & 0x7f);
	ws.header_size = 2 + (ws.N0 == 126 ? 2 : 0) + (ws.N0 == 127 ? 8 : 0) + (ws.mask ? 4 : 0);
}

void WebSocketTcp::DecodeHeaderBodyWebSocketFrame(vector<unsigned char> data, WsheaderType& ws) {
	int i = 0;
	if (ws.N0 < 126) {
		ws.N = ws.N0;
		i = 2;
	} else if (ws.N0 == 126) {
		ws.N = 0;
		ws.N |= ((uint64_t)data[2]) << 8;
		ws.N |= ((uint64_t)data[3]) << 0;
		i = 4;
	} else if (ws.N0 == 127) {
		ws.N = 0;
		ws.N |= ((uint64_t)data[2]) << 56;
		ws.N |= ((uint64_t)data[3]) << 48;
		ws.N |= ((uint64_t)data[4]) << 40;
		ws.N |= ((uint64_t)data[5]) << 32;
		ws.N |= ((uint64_t)data[6]) << 24;
		ws.N |= ((uint64_t)data[7]) << 16;
		ws.N |= ((uint64_t)data[8]) << 8;
		ws.N |= ((uint64_t)data[9]) << 0;
		i = 10;
	}

	if (ws.mask) {
		ws.masking_key[0] = ((uint8_t)data[i + 0]) << 0;
		ws.masking_key[1] = ((uint8_t)data[i + 1]) << 0;
		ws.masking_key[2] = ((uint8_t)data[i + 2]) << 0;
		ws.masking_key[3] = ((uint8_t)data[i + 3]) << 0;
	} else {
		ws.masking_key[0] = 0;
		ws.masking_key[1] = 0;
		ws.masking_key[2] = 0;
		ws.masking_key[3] = 0;
	}
}

int WebSocketTcp::DecodeFrameBodyWebSocketFrame(vector<unsigned char> rxbuf, WsheaderType& ws, WebsocketFrame& receivedData) {
	if (ws.opcode == WsheaderType::TEXT_FRAME ||
        ws.opcode == WsheaderType::BINARY_FRAME ||
        ws.opcode == WsheaderType::CONTINUATION) {
		if (ws.mask) {
			for (size_t i = 0; i != ws.N; ++i) {
				rxbuf[i + ws.header_size] ^= ws.masking_key[i & 0x3];
			}
		}

		if (receivedData.data.empty()) {
			receivedData.type = ws.opcode;
		}

		receivedData.data.insert(receivedData.data.end(),
                                 rxbuf.begin() + ws.header_size,
                                 rxbuf.begin() + ws.header_size + (size_t)ws.N);
	} else if (ws.opcode == WsheaderType::PING) {
//		throw ExceptionWithString("PING no implementaion",10000004);
		return -1;
	} else if (ws.opcode == WsheaderType::CLOSE) {
		StatusCode code;
		code.pdata[0] = rxbuf[2];
		code.pdata[1] = rxbuf[3];
		int recode = ntohs(code.code);
		if (receivedData.data.empty()) {
			receivedData.type = ws.opcode;
			receivedData.closecode = recode;
		}
		receivedData.data.insert(receivedData.data.end(),
                                 rxbuf.begin() + ws.header_size + 2,
                                 rxbuf.begin() + ws.header_size + (size_t)ws.N);
	}

	return 0;
}

bool WebSocketTcp::InitializeSslContext() {
	return true;
}

void WebSocketTcp::CloseSsl() {
    if (_useSSL) {
        SSL_shutdown(_ssl);
    }
}

}
