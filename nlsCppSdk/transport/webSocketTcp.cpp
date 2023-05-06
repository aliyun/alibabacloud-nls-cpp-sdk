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

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#endif

#include <stdlib.h>
#include "utility.h"
#include "log.h"
#include "webSocketTcp.h"

using std::string;
//using std::min;

namespace AlibabaNls {
//namespace transport {

using namespace utility;

#define HTTP_PORT 80
#define HTTP_STATUS_CODE "HTTP/1.1 "
#define HTTP_STATUS_CODE_END " "
#define HTTP_HEADER_END_STRING "\r\n\r\n"
#define HTTP_CHUNK_END_STRING "\r\n0\r\n\r\n"
#define HTTP_CONTENT_LENGTH "Content-Length: "
#define HTTP_CONTENT_LENGTH_END "\r\n"
#define HTTP_ "Transfer-Encoding: chunked\r\n"

WebSocketTcp::WebSocketTcp() {
    _httpCode = 0;
    _httpLength = 0;
    _rStatus = WsHeadSize;
	LOG_INFO("create WebSocket.");
}

WebSocketTcp::~WebSocketTcp() {
	LOG_INFO("Destroy WebSocket.");
}

int WebSocketTcp::requestPackage(urlAddress * url, char* buffer, string httpHeader) {
    char hostBuff[256] = {0};

    if (url->_port == HTTP_PORT) {
        _ssnprintf(hostBuff, 256, "Host: %s\r\n", url->_host);
    } else {
        _ssnprintf(hostBuff, 256, "Host: %s:%d\r\n", url->_host, url->_port);
    }

	int contentSize = 0;
	if (httpHeader.empty()) {
		contentSize = _ssnprintf(buffer, BUFFER_SIZE, "GET /%s HTTP/1.1\r\n%s%s%s%s%s%s: %s\r\n%s",
								 url->_path,
								 hostBuff,
								 "Upgrade: websocket\r\n",
								 "Connection: Upgrade\r\n",
								 "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n",
								 "Sec-WebSocket-Version: 13\r\n",
								 "X-NLS-Token",
								 url->_token,
								 "\r\n");

	} else {
		contentSize = _ssnprintf(buffer, BUFFER_SIZE, "GET /%s HTTP/1.1\r\n%s%s%s%s%s%s: %s\r\n%s%s",
								 url->_path,
								 hostBuff,
								 "Upgrade: websocket\r\n",
								 "Connection: Upgrade\r\n",
								 "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n",
								 "Sec-WebSocket-Version: 13\r\n",
								 "X-NLS-Token",
								 url->_token,
								 httpHeader.c_str(),
								 "\r\n");
	}

	if (contentSize <= 0) {
		LOG_ERROR("send http head to server failed.");
		return -1;
	}

	LOG_INFO("Http Request:%s", buffer);

    return contentSize;
}

/*
HTTP/1.1 403 Forbidden^M
Date: Mon, 18 Feb 2019 08:38:46 GMT^M
Content-Length: 64^M
Connection: keep-alive^M
Server: Tengine^M
^M
Meta:ACCESS_DENIED:The token '12345ffdsfdfvdfcdfcdc' is invalid!.
*/

/*
HTTP/1.1 101 Switching Protocols^M
Date: Mon, 18 Feb 2019 08:39:51 GMT^M
Connection: upgrade^M
Server: Tengine^M
upgrade: websocket^M
sec-websocket-accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=^M
^M
*/

int WebSocketTcp::getTargetLen(string line, const char* begin, const char* end) {
    size_t seek = line.find(begin);

    size_t position = line.find(end, seek + strlen(begin));
    if (position == string::npos) {
        return 0;
    }
    LOG_DEBUG("Position: %d %d %s", position, strlen(begin), line.c_str()+position);

    string tmpCode(line.substr(seek + strlen(begin), position - strlen(begin)));
    LOG_DEBUG("tmpCode: %s", tmpCode.c_str());

    return atoi(tmpCode.c_str());

}

const char* WebSocketTcp::getFailedMsg() {
	return _errorMsg.c_str();
}

int WebSocketTcp::responsePackage(const char * content, size_t length) {

    LOG_DEBUG("Http response:%s", content);

    if (!strstr(content, HTTP_HEADER_END_STRING)) {
        return length;
    }

    string tmpLine = content;
    if (_httpCode == 0) {
        _httpCode = getTargetLen(tmpLine, "HTTP/1.1 ", " ");
        if (_httpCode == 0) {
            LOG_ERROR("Got bad status connecting to %s: %s", content);
            return -1;
        }
    }

    if (_httpCode == 101) {
        return 0;
    } else {
        if (_httpLength == 0) {
            _httpLength = getTargetLen(tmpLine, "Content-Length: ", "\r\n");
        }

        size_t endLen = strlen(HTTP_HEADER_END_STRING);
        size_t position = tmpLine.find(HTTP_HEADER_END_STRING);
        if (position != string::npos) {
            if (_httpLength == 0) {
                LOG_ERROR("Failed: %s", content);
				_errorMsg = content;
                return -1;
            } else {
                if (_httpLength == (length - (position + endLen))) {
                    const char* errMsg = (content + position + endLen);
					_errorMsg = errMsg;
                    LOG_ERROR("Position: %d %s", (int)(length - (position + endLen)), errMsg);
                    return -1;
                }
            }
        }
    }

    return -1;
}

int WebSocketTcp::receiveFullWebSocketFrame(uint8_t* frame,
											size_t frameSize,
											WebSocketHeaderType* wsType,
											WebSocketFrame* resultDate) {

//    LOG_DEBUG("begin receiveFullWebSocketFrame.");
    switch(_rStatus) {
        case WsHeadSize:
//            LOG_DEBUG("WsHeadSize.");
            if (decodeHeaderSizeWebSocketFrame(frame, frameSize, wsType) == -1) {
//                LOG_DEBUG("Parse WsHeadSize.");
                return -1;
            }
            _rStatus = WsHeadBody;
        case WsHeadBody:
//            LOG_DEBUG("WsHeadBody.");
            if (decodeHeaderBodyWebSocketFrame(frame, frameSize, wsType) == -1) {
//                LOG_DEBUG("Parse WsHeadBody.");
                return -1;
            }
            _rStatus = WsContentBody;
        case WsContentBody:
//            LOG_DEBUG("WsContentBody.");
            if (decodeFrameBodyWebSocketFrame(frame, frameSize, wsType, resultDate) == -1) {
 //               LOG_DEBUG("Parse WsContentBody.");
                return -1;
            }
            _rStatus = WsHeadSize;
            return 0;
        default:
            LOG_DEBUG("Default None.");
            break;
    }

//    LOG_DEBUG("receiveFullWebSocketFrame done.");
    return 0;
}

int WebSocketTcp::decodeHeaderSizeWebSocketFrame(uint8_t * buffer, size_t length, WebSocketHeaderType* wsType) {
	if (length < 2) {
		return -1; /* Need at least 2 */
	}

	const uint8_t *data = buffer; // peek, but don't consume
	wsType->fin = (data[0] & 0x80) == 0x80;
	wsType->opCode = (WebSocketHeaderType::OpCodeType) (data[0] & 0x0f);
	wsType->mask = (data[1] & 0x80) == 0x80;
	wsType->N0 = (data[1] & 0x7f);
	wsType->headerSize = 2 + (wsType->N0 == 126 ? 2 : 0) + (wsType->N0 == 127 ? 8 : 0) + (wsType->mask ? 4 : 0);

//    LOG_DEBUG("wsType->headerSize: %d", wsType->headerSize);

	return 0;
}

int WebSocketTcp::decodeHeaderBodyWebSocketFrame(uint8_t* buffer, size_t length, WebSocketHeaderType* wsType) {

    if (wsType->headerSize >= length) {
        return -1;
    }

	if (wsType->N0 < 126) {
		wsType->N = wsType->N0;
	} else if (wsType->N0 == 126) {
		wsType->N = 0;
		wsType->N |= ((uint64_t)(*(buffer + 2))) << 8;
		wsType->N |= ((uint64_t)(*(buffer + 3))) << 0;
	} else if (wsType->N0 == 127) {
		wsType->N = 0;
		wsType->N |= ((uint64_t)(*(buffer + 2))) << 56;
		wsType->N |= ((uint64_t)(*(buffer + 3))) << 48;
		wsType->N |= ((uint64_t)(*(buffer + 4))) << 40;
		wsType->N |= ((uint64_t)(*(buffer + 5))) << 32;
		wsType->N |= ((uint64_t)(*(buffer + 6))) << 24;
		wsType->N |= ((uint64_t)(*(buffer + 7))) << 16;
		wsType->N |= ((uint64_t)(*(buffer + 8))) << 8;
		wsType->N |= ((uint64_t)(*(buffer + 9))) << 0;
	}

	if (wsType->mask) {
		wsType->masKingKey[0] = ((uint8_t)(*(buffer + 0))) << 0;
		wsType->masKingKey[1] = ((uint8_t)(*(buffer + 1))) << 0;
		wsType->masKingKey[2] = ((uint8_t)(*(buffer + 2))) << 0;
		wsType->masKingKey[3] = ((uint8_t)(*(buffer + 3))) << 0;
	} else {
		wsType->masKingKey[0] = 0;
		wsType->masKingKey[1] = 0;
		wsType->masKingKey[2] = 0;
		wsType->masKingKey[3] = 0;
	}

//    LOG_DEBUG("wsType->N: %d", wsType->N);

    return 0;
}

int WebSocketTcp::decodeFrameBodyWebSocketFrame(uint8_t * buffer,
												size_t length,
												WebSocketHeaderType* wsType,
												WebSocketFrame* receivedData) {

    if ((wsType->N + wsType->headerSize) > length) {
//        LOG_DEBUG("Size: %d %u %zu", wsType->N, wsType->headerSize, length);
        return -1;
    }

//	LOG_DEBUG("Size: %d %d %d", wsType->N, wsType->headerSize, length);

	if (wsType->opCode == WebSocketHeaderType::TEXT_FRAME ||
		wsType->opCode == WebSocketHeaderType::BINARY_FRAME ||
		wsType->opCode == WebSocketHeaderType::CONTINUATION) {
		if (wsType->mask) {
			for (size_t i = 0; i != wsType->N; ++i) {
				*(buffer + i + wsType->headerSize) ^= wsType->masKingKey[i & 0x3];
			}
		}

		if (receivedData->data == NULL) {
			receivedData->type = wsType->opCode;
		}

		receivedData->data = (buffer + wsType->headerSize);
		receivedData->length = (size_t)wsType->N;
	} else if (wsType->opCode == WebSocketHeaderType::PING) {
		return -1;
	} else if (wsType->opCode == WebSocketHeaderType::CLOSE) {
		StatusCode code;
		code.frame[0] = *(buffer + 2);
		code.frame[1] = *(buffer + 3);
		int recode = ntohs(code.status);
		if (receivedData->data == NULL) {
			receivedData->type = wsType->opCode;
			receivedData->closeCode = recode;
		}
		receivedData->data = (buffer + wsType->headerSize + 2);
		receivedData->length = (size_t)wsType->N;
	}

    if (wsType->opCode == WebSocketHeaderType::TEXT_FRAME) {
        LOG_DEBUG("Receive TEXT Data: %zu | %s", receivedData->length, (char *)receivedData->data);
    } else {
        LOG_DEBUG("Decoder Receive Data: %zu ", receivedData->length);
    };

//	LOG_DEBUG("receivedData->type: %d | %d", receivedData->type, wsType->opCode);

	return 0;
}

int WebSocketTcp::binaryFrame(const uint8_t * buffer,
                                  size_t length,
                                  uint8_t** frame,
                                  size_t * frameSize) {
	return framePackage(WebSocketHeaderType::BINARY_FRAME, buffer, length, frame, frameSize);
}

int WebSocketTcp::textFrame(const uint8_t * buffer,
                                size_t length,
                                uint8_t** frame,
                                size_t * frameSize) {
	return framePackage(WebSocketHeaderType::TEXT_FRAME, buffer, length, frame, frameSize);
}

int WebSocketTcp::framePackage(WebSocketHeaderType::OpCodeType codeType,
                                   const uint8_t * buffer,
                                   size_t length,
                                   uint8_t ** frame,
                                   size_t * frameSize) {

    bool useMask = true;
	const uint8_t masKingKey[4] = { 0x12, 0x34, 0x56, 0x78 };
	const int headlen = 2 + (length >= 126 ? 2 : 0) + (length >= 65536 ? 6 : 0) + (useMask ? 4 : 0);

	uint8_t* header = (uint8_t*)calloc(headlen, sizeof(uint8_t));//new uint8_t[headlen];
//	memset(header, 0, sizeof(uint8_t)*headlen);
	header[0] = 0x80 | codeType;

	if (length < 126) {
		header[1] = (length & 0xff) | (useMask ? 0x80 : 0);
		if (useMask) {
			header[2] = masKingKey[0];
			header[3] = masKingKey[1];
			header[4] = masKingKey[2];
			header[5] = masKingKey[3];
		}
	} else if (length < 65536) {
		header[1] = 126 | (useMask ? 0x80 : 0);
		header[2] = (length >> 8) & 0xff;
		header[3] = (length >> 0) & 0xff;
		if (useMask) {
			header[4] = masKingKey[0];
			header[5] = masKingKey[1];
			header[6] = masKingKey[2];
			header[7] = masKingKey[3];
		}
	} else { // TODO: run coverage testing here
		header[1] = 127 | (useMask ? 0x80 : 0);
		header[2] = ((uint64_t)length >> 56) & 0xff;
		header[3] = ((uint64_t)length >> 48) & 0xff;
		header[4] = ((uint64_t)length >> 40) & 0xff;
		header[5] = ((uint64_t)length >> 32) & 0xff;
		header[6] = ((uint64_t)length >> 24) & 0xff;
		header[7] = ((uint64_t)length >> 16) & 0xff;
		header[8] = ((uint64_t)length >> 8) & 0xff;
		header[9] = ((uint64_t)length >> 0) & 0xff;
		if (useMask) {
			header[10] = masKingKey[0];
			header[11] = masKingKey[1];
			header[12] = masKingKey[2];
			header[13] = masKingKey[3];
		}
	}

	// N.B. - tmpBuffer will keep growing until it can be transmitted over the socket:
//	uint8_t * tmpBuffer = new uint8_t[headlen + length];
//	memset(tmpBuffer, 0, sizeof(uint8_t) * (headlen + length));
//	memcpy(tmpBuffer, header, headlen);
//	memcpy(tmpBuffer + headlen, (uint8_t*)buffer, length);

    *frameSize = (headlen + length);
    *frame = (uint8_t *)calloc(*frameSize, sizeof(uint8_t));
    memset(*frame, 0, *frameSize);
    memcpy(*frame, header, headlen);
    memcpy(*frame + headlen, (uint8_t*)buffer, length);

//    LOG_DEBUG("framePackage Receive Data: %d ", *frameSize);

	if (useMask) {
		for (size_t i = 0; i != length; ++i) {
			*(*frame + headlen + length - length + i) ^= masKingKey[i & 0x3];
		}
	}

    free(header);
    header = NULL;

    return 0;

//	int ret = webSocketSend(tmpBuffer, (int)(headlen + length));
//	delete[] tmpBuffer;
//	tmpBuffer = NULL;

//	delete[] header;
//	header = NULL;

//	if (ret<= 0) {
//		return ret;
//	}
//
//	return ret - headlen;
}

//}
}
