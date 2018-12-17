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

#if !defined(_WIN32)
#include <netdb.h>
#endif

#if !defined( __APPLE__ )
#include "openssl/err.h"
#else
//Security ssl
#endif

#include "webSocketTcp.h"
#include <signal.h>
#include <string>
#include <algorithm>
#include <fcntl.h>
#include <stdlib.h>
#include "log.h"

using std::string;
using std::vector;
using std::min;

namespace AlibabaNls {
namespace transport {

using namespace util;

#define LINE_LENGTH 512
#define HTTP_END_STRING "\r\n\r\n"

const char* TOKEN = "X-NLS-Token";

#if defined(_MSC_VER)
pthread_mutex_t WebSocketTcp::_sslMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

WebSocketTcp::WebSocketTcp(SmartHandle<SOCKET> sockfd, int timeOut, const WebSocketAddress& addr, string token) : Socket(sockfd, timeOut),
                                                                                                                  _useMask(true),
                                                                                                                  _useSSL(false) {

	if (strcmp(addr.type, "wss") == 0 && addr.port == 443) {
#if !defined( __APPLE__ )
        _ssl = NULL;
	    _sslCtx = NULL;

		SOCKET fd = sockfd.GetHandle();

		const SSL_METHOD * sslMethod = SSLv23_client_method();
		if (sslMethod == NULL) {
			throw ExceptionWithString("SSL: couldn't create a method!", 10000001);
		}

		_sslCtx = SSL_CTX_new(sslMethod);
		if (_sslCtx == NULL) {
			throw ExceptionWithString("SSL: couldn't create a context!", 10000001);
		}

		_ssl = SSL_new(_sslCtx);
		if (_ssl == NULL) {
			throw ExceptionWithString("SSL: couldn't create a context (handle)!", 10000001);
		}

		int nRet = SSL_set_fd(_ssl, (int)fd);
		if (nRet == 0) {
			throw ExceptionWithString("SSL: couldn't create a fd!", 10000001);
		}

#if defined(_MSC_VER)
		pthread_mutex_lock(&_sslMutex);
#endif

		nRet = SSL_connect(_ssl);

#if defined(_MSC_VER)
		pthread_mutex_unlock(&_sslMutex);
#endif

		if (-1 == nRet) {
			//int code = 0;
			int err = 0;
			int counter = 10;
			do {
				sleepTime(50);
				err = SSL_get_error(_ssl, nRet);
				//code = ERR_get_error();

#if defined(_MSC_VER)
				pthread_mutex_lock(&_sslMutex);
#endif

				nRet = SSL_connect(_ssl);

#if defined(_MSC_VER)
				pthread_mutex_unlock(&_sslMutex);
#endif
			} while (counter-- && (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE));

            if (err != SSL_ERROR_NONE) {
				char errorBuffer[128] = {0};
				ERR_error_string(err, errorBuffer);
				throw ExceptionWithString(errorBuffer, 10000002);
			}
		}
#else
        //Security ssl
        _iosSslHandle = NULL;
        _iosSslParam.socketFd = sockfd.GetHandle();

        LOG_DEBUG("connect socketFd: %d",  _iosSslParam.socketFd);

        _iosSslHandle  = new IosSslConnect();

        if (NULL == _iosSslHandle->iosSslHandshake(&_iosSslParam, addr.host) ) {
            throw ExceptionWithString(_iosSslHandle->getErrorMsg(), 10000001);
        }
#endif
        _useSSL = true;
    }

/*
    struct timeval tv;
    gettimeofday(&tv,NULL);
*/

	if (ConnectToHttp(addr, token) == false) {
		throw ExceptionWithString("HTTP: connect failed.", 100000013);
	}

/*
    struct timeval tv1;
    gettimeofday(&tv1,NULL);
    LOG_DEBUG("ConnectToTime: %llu, %llu, %llu.",
    tv.tv_sec*1000 + tv.tv_usec/1000,
    tv1.tv_sec*1000 + tv1.tv_usec/1000,
    (tv1.tv_sec*1000 + tv1.tv_usec/1000) - (tv.tv_sec*1000 + tv.tv_usec/1000));
*/

    _blockSigpipe = false;
}

int WebSocketTcp::ws_read(void *buf, size_t num) {
	int ret = 0;

#if defined( __APPLE__ )
    ret = _useSSL == true? _iosSslHandle->iosSslRead(_iosSslHandle->getCtxHandle(), buf, num) : Recv((unsigned char*)buf, (int)num);
#else
    ret = _useSSL == true? SSL_read(_ssl, buf, (int)num) : Recv((unsigned char*)buf, (int)num);
#endif

	return ret;
}

int WebSocketTcp::ws_write(const void *buf, size_t num) {
	int ret = 0;

#if defined( __APPLE__ )
	ret = _useSSL == true ? _iosSslHandle->iosSslWrite(_iosSslHandle->getCtxHandle(), buf, num) : Send((unsigned char*)buf, (int)num);
#else
    ret = _useSSL == true ? SSL_write(_ssl, buf, (int)num) : Send((unsigned char*)buf, (int)num);
#endif

	return ret;
}

WebSocketTcp::~WebSocketTcp() {
	if (_useSSL) {
#if defined( __APPLE__ )
        delete _iosSslHandle;
        _iosSslHandle = NULL;
#else
        SSL_free(_ssl);
		SSL_CTX_free(_sslCtx);
#endif
	}
}

WebSocketTcp* WebSocketTcp::connectTo(const WebSocketAddress& addr, int timeOut, string token) {
	SocketFuncs::Startup();
	string ip;
    int aiFamily;
    string errorMsg;
	bool res = InetAddress::GetInetAddressByHostname(addr.host, ip, aiFamily, errorMsg);
	if (res == false) {
		throw ExceptionWithString(errorMsg, 10000003);
	}

    SmartHandle<SOCKET> sockfd(socket(aiFamily, SOCK_STREAM, 0));

	InetAddress ina(ip, aiFamily, addr.port);

	fd_set fdr, fdw;
	struct timeval timeout;
	int ret;

#if defined(_WIN32)
	unsigned long ul = 1;
	ret = ioctlsocket(sockfd.get(), FIONBIO, (unsigned long*)&ul);
	if (ret == SOCKET_ERROR) {
		throw ExceptionWithString("ioctlsocket FIONBIO failed.", 10000015);
	}
#else
	int flags;
	flags = fcntl(sockfd.get(), F_GETFL, 0);
	if (flags < 0) {
		throw ExceptionWithString("fcntl F_GETFL failed.", 10000015);
	}
	flags |= O_NONBLOCK;
	if (fcntl(sockfd.get(), F_SETFL, flags) < 0) {
		throw ExceptionWithString("fcntl F_SETFL failed.", 10000015);
	}
#endif

	ret = SocketFuncs::connectTo(sockfd.get(), ina);
  	if (ret != 0) {

#if !defined(_WIN32)
    if (Socket::getLastErrorCode() == EINPROGRESS) {
#endif
		LOG_DEBUG("connect in progress");

		FD_ZERO(&fdr);
      	FD_ZERO(&fdw);
      	FD_SET(sockfd.get(), &fdr);
      	FD_SET(sockfd.get(), &fdw);
      	timeout.tv_sec = 3;
      	timeout.tv_usec = 0;

      	ret = select(((int)sockfd.get()) + 1, &fdr, &fdw, NULL, &timeout);
      	if (ret < 0) {
        	throw ExceptionWithString("connect failed.", 10000015);
      	} else if (ret == 0) {
        	throw ExceptionWithString("connect timeout.", 10000015);
      	} else if (ret == 2) {
        	throw ExceptionWithString("connect error.", 10000015);
      	} else if (ret == 1 && FD_ISSET(sockfd.get(), &fdw)) {
        	LOG_DEBUG("connect done.");
      	} else {
        	LOG_ERROR("connect return value is %d.", ret);
        	throw ExceptionWithString("select failed.", 10000015);
      	}

#if !defined(_WIN32)
    } else {
		LOG_ERROR("select error %s.", gai_strerror(Socket::getLastErrorCode()));
		throw ExceptionWithString("select error.", 10000015);
    }
#endif
  }

#if defined(_WIN32)
	ul = 0;
	ret = ioctlsocket(sockfd.get(), FIONBIO, (unsigned long*)&ul);
	if (ret == SOCKET_ERROR) {
		throw ExceptionWithString("ioctlsocket FIONBIO failed.", 10000015);
	}
#else
	flags = fcntl(sockfd.get(), F_GETFL, 0);
	if (flags < 0) {
		throw ExceptionWithString("fcntl get failed", 10000015);
	}
	flags &= ~O_NONBLOCK;
	if (fcntl(sockfd.get(), F_SETFL, flags) < 0) {
		throw ExceptionWithString("fcntl set failed", 10000015);
	}
#endif

	WebSocketTcp* tcpSock = new WebSocketTcp(sockfd, timeOut, addr, token);

    return tcpSock;
}

int WebSocketTcp::getTargetLen(string line, const char* begin, const char* end) {
	size_t seek = line.find(begin);

	size_t position = line.find(end, seek + strlen(begin));
	if (position == string::npos) {
		return -1;
	}
//	LOG_DEBUG("Position: %d %d %s", position, strlen(begin), line.c_str()+position);

	string tmpCode(line.substr(seek + strlen(begin), position - strlen(begin)));
//	LOG_DEBUG("tmpCode: %s", tmpCode.c_str());

	return atoi(tmpCode.c_str());
}

bool WebSocketTcp::ConnectToHttp(const WebSocketAddress addr, string token) {
	char line[LINE_LENGTH] = {0};
	int statusCode = -1;
	int contentLen = -1;
	int recvLen = 0;
	bool retValue = false;
	char hostBuff[256] = {0};

	if (addr.port == 80) {
		snprintf(hostBuff, 256, "Host: %s\r\n", addr.host);
	} else {
		snprintf(hostBuff, 256, "Host: %s:%d\r\n", addr.host, addr.port);
	}

	snprintf(line, LINE_LENGTH, "GET /%s HTTP/1.1\r\n%s%s%s%s%s%s: %s\r\n%s",
			 addr.path,
			 hostBuff,
			 "Upgrade: websocket\r\n",
			 "Connection: Upgrade\r\n",
			 "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n",
			 "Sec-WebSocket-Version: 13\r\n",
			 TOKEN,
			 token.c_str(),
			 "\r\n");
	ws_write(line, strlen(line));
	LOG_DEBUG("send http head to server:%s", line);

	memset(line, 0, LINE_LENGTH);
	int ret = 0;
	for (recvLen = 0; recvLen < (LINE_LENGTH - 1); ) {
		ret = ws_read(line + recvLen, (LINE_LENGTH - recvLen));
		if (ret <= 0) {
            LOG_ERROR("handshake: read failed, %d.", ret);
			retValue = false;
			break;
        }
		recvLen += ret;

		if (recvLen < 2) {
			continue;
		}

//		LOG_DEBUG("handshake: %s.", line);

		string tmpLine = line;
		if (statusCode == -1) {
			statusCode = getTargetLen(tmpLine, "HTTP/1.1 ", " ");
			if (statusCode == -1) {
				LOG_ERROR("Got bad status connecting to %s: %s", addr.host, line);
				throw ExceptionWithString("HTTP: Got bad status.", statusCode);
			}
		}

		if (statusCode == 101) {
			retValue = true;
			break;
		} else {
			if (contentLen == -1) {
				contentLen = getTargetLen(line, "Content-Length: ", "\r\n");
			}

			size_t endLen = strlen(HTTP_END_STRING);
			size_t position = tmpLine.find(HTTP_END_STRING);
			if (position != string::npos) {
				if (contentLen == -1) {
					char* errorMsg = line;
					throw ExceptionWithString(errorMsg, statusCode);
				} else {
					if (contentLen == (recvLen - (position + endLen))) {
						char* errorMsg = (line + position + endLen);
						LOG_ERROR("Position: %d %s", (int)(recvLen - (position + endLen)), errorMsg);
						throw ExceptionWithString(errorMsg, statusCode);
					}
				}
			}
		}
	}
	line[recvLen] = 0;

	LOG_DEBUG("http: %s", line);

	LOG_DEBUG("receive http head response from server");

	return retValue;
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
#if defined( __APPLE__ )
        nRet = _iosSslHandle->iosSslWrite(_iosSslHandle->getCtxHandle(), data, len);
        if (nRet < 0) {
            return -1;
        }
#else
        nRet = SSL_write(_ssl, data, len);
        if (nRet <= 0) {
            return -1;
        }
#endif
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
	vector<unsigned char> body((int)ws.N, 0);
    if ( -1 == RecvDataBySize(body, (int)ws.N)) {
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
#if defined( __APPLE__ )
        _iosSslHandle->iosSslClose();
#else
        SSL_shutdown(_ssl);
#endif
    }
}

}
}
