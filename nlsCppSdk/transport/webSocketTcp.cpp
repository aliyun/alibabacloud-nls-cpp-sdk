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

#if defined(_MSC_VER)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#endif

#include <stdlib.h>

#include <fstream>
#include <sstream>

#include "nlog.h"
#include "nlsGlobal.h"
#include "text_utils.h"
#include "utility.h"
#include "webSocketTcp.h"

namespace AlibabaNls {

#define HTTP_STATUS_CODE        "HTTP/1.1 "
#define HTTP_STATUS_CODE_END    " "
#define HTTP_HEADER_END_STRING  "\r\n\r\n"
#define HTTP_CONTENT_LENGTH     "Content-Length: "
#define HTTP_CONTENT_LENGTH_END "\r\n"
#define SEC_WS_VER              "13"

//#define OPU_DEBUG

WebSocketTcp::WebSocketTcp()
    : _httpCode(0), _httpLength(0), _rStatus(WsHeadSize) {
  LOG_DEBUG("Create WebSocketTcp:%p.", this);
}

WebSocketTcp::~WebSocketTcp() {
  LOG_DEBUG("WS(%p) Destroy WebSocketTcp done.", this);
}

int WebSocketTcp::parseUrlAddress(struct urlAddress& url, const char* address) {
  if (sscanf(address, "%10[^:/]://%256[^:/]:%d/%255s", url._type, url._host,
             &url._port, url._path) == 4) {
    if (strcmp(url._type, "wss") == 0 || strcmp(url._type, "https") == 0) {
      url._isSsl = true;
    }
  } else if (sscanf(address, "%10[^:/]://%256[^:/]/%255s", url._type, url._host,
                    url._path) == 3) {
    if (strcmp(url._type, "wss") == 0 || strcmp(url._type, "https") == 0) {
      url._port = 443;
      url._isSsl = true;
    } else {
      url._port = 80;
    }
  } else if (sscanf(address, "%10[^:/]://%256[^:/]:%d", url._type, url._host,
                    &url._port) == 3) {
    url._path[0] = '\0';
  } else if (sscanf(address, "%10[^:/]://%256[^:/]", url._type, url._host) ==
             2) {
    if (strcmp(url._type, "wss") == 0 || strcmp(url._type, "https") == 0) {
      url._port = 443;
      url._isSsl = true;
    } else {
      url._port = 80;
    }
    url._path[0] = '\0';
  } else {
    return -(ParseUrlFailed);
  }
  return Success;
}

bool WebSocketTcp::urlWithAccess(const char* address) {
  struct urlAddress url;
  if (WebSocketTcp::parseUrlAddress(url, address) == Success) {
    for (int i = 0; i < strnlen(url._path, PathSize); i++) {
      if (url._path[i] == '?') {
        return true;
      }
    }
  }
  return false;
}

int WebSocketTcp::requestPackage(urlAddress* url, char* buffer,
                                 std::string httpHeader) {
  char hostBuff[256] = {0};

  if (url->_port == HttpPort) {
    _ssnprintf(hostBuff, 256, "Host: %s\r\n", url->_host);
  } else {
    _ssnprintf(hostBuff, 256, "Host: %s:%d\r\n", url->_host, url->_port);
  }

  int contentSize = 0;
  const int token_bytes = strnlen(url->_token, TokenSize);
  const int ws_key_bytes = strnlen(getSecWsKey(), 128) - 18;
  if (httpHeader.empty()) {
    contentSize = _ssnprintf(
        buffer, BufferSize, "GET /%s HTTP/1.1\r\n%s%s%s%s%s%s%s%s%s: %s\r\n%s",
        url->_path, hostBuff, "Upgrade: websocket\r\n",
        "Connection: Upgrade\r\n", getSecWsKey(), HTTP_CONTENT_LENGTH_END,
        "Sec-WebSocket-Version: ", SEC_WS_VER, HTTP_CONTENT_LENGTH_END,
        "X-NLS-Token", url->_token, HTTP_CONTENT_LENGTH_END);
  } else {
    contentSize = _ssnprintf(
        buffer, BufferSize,
        "GET /%s HTTP/1.1\r\n%s%s%s%s%s%s%s%s%s: %s\r\n%s%s", url->_path,
        hostBuff, "Upgrade: websocket\r\n", "Connection: Upgrade\r\n",
        getSecWsKey(), HTTP_CONTENT_LENGTH_END,
        "Sec-WebSocket-Version: ", SEC_WS_VER, HTTP_CONTENT_LENGTH_END,
        "X-NLS-Token", url->_token, httpHeader.c_str(),
        HTTP_CONTENT_LENGTH_END);
  }

  if (contentSize <= 0) {
    LOG_ERROR("WS(%p) send http head to server failed.", this);
    return -(WsRequestPackageEmpty);
  }

  const int default_step = 4;
  const int token_step =
      token_bytes < default_step ? token_bytes : default_step;
  const int ws_key_step =
      ws_key_bytes < default_step ? ws_key_step : default_step;
  if (token_step > 0 && ws_key_step > 0) {
    std::string key_buf_str;
    std::string token_buf_str;
    key_buf_str = utility::TextUtils::securityDisposalForLog(
        buffer, &key_buf_str, "Sec-WebSocket-Key:", ws_key_step, 'X');
    token_buf_str = utility::TextUtils::securityDisposalForLog(
        (char*)key_buf_str.c_str(), &token_buf_str, "X-NLS-Token:", token_step,
        'Y');
    LOG_DEBUG("WS(%p) Http Request:\n%s", this, token_buf_str.c_str());
  }
  return contentSize;
}

/*
 * HTTP/1.1 403 Forbidden
 * Date: Mon, 18 Feb 2019 08:38:46 GMT
 * Content-Length: 64
 * Connection: keep-alive
 * Server: Tengine
 *
 * Meta:ACCESS_DENIED:The token '12345ffdsfdfvdfcdfcdc' is invalid!.
 */

/*
 * HTTP/1.1 101 Switching Protocols
 * Date: Mon, 18 Feb 2019 08:39:51 GMT
 * Connection: upgrade
 * Server: Tengine
 * upgrade: websocket
 * sec-websocket-accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=
 */

int WebSocketTcp::getTargetLen(std::string line, const char* begin,
                               const char* end) {
  size_t seek = line.find(begin);

  size_t position = line.find(end, seek + strlen(begin));
  if (position == std::string::npos) {
    return 0;
  }
  LOG_DEBUG("WS(%p) Position: %d %d %s", this, position, strlen(begin),
            line.c_str() + position);

  std::string tmpCode(
      line.substr(seek + strlen(begin), position - strlen(begin)));

  return atoi(tmpCode.c_str());
}

const char* WebSocketTcp::getFailedMsg() { return _errorMsg.c_str(); }

const char* WebSocketTcp::getSecWsKey() {
  char buffer[128] = {0};
  char tmp[64] = {0};
  char key_array[10] = {'x', 'D', 'H', '3', 'L', 'M', 'J', '1', 'b', 'J'};
  char key2[] = {"BhXDw=="};
  char key1[] = {"EzLkh9G"};
  char name3[] = {"Key"};
  char name2[] = {"WebSocket"};
  char name1[] = {"Sec"};
  int i = 0;

  for (i = 0; i < 10; i++) {
    tmp[i] = key_array[i * 7 % 10];
  }

  _ssnprintf(buffer, 128, "%s-%s-%s: %s%s%s", name1, name2, name3, tmp, key1,
             key2);
  _secWsKey = buffer;

  return _secWsKey.c_str();
}

int WebSocketTcp::responsePackage(const char* content, size_t length) {
  LOG_DEBUG("WS(%p) Http response:%s", this, content);

  if (!strstr(content, HTTP_HEADER_END_STRING)) {
    return length;
  }

  std::string tmpLine = content;
  if (_httpCode == 0) {
    _httpCode = getTargetLen(tmpLine, HTTP_STATUS_CODE, HTTP_STATUS_CODE_END);
    if (_httpCode == 0) {
      LOG_ERROR("WS(%p) Got bad status connecting to %s", this, content);
      return -(HttpGotBadStatus);
    }
  }

  if (_httpCode == 101) {
    return 0;
  } else {
    if (_httpLength == 0) {
      _httpLength =
          getTargetLen(tmpLine, HTTP_CONTENT_LENGTH, HTTP_CONTENT_LENGTH_END);
    }

    size_t endLen = strlen(HTTP_HEADER_END_STRING);
    size_t position = tmpLine.find(HTTP_HEADER_END_STRING);
    if (position != std::string::npos) {
      if (_httpLength == 0) {
        LOG_ERROR("WS(%p) Failed: %s", this, content);
        _errorMsg = content;
        return -(WsResponsePackageFailed);
      } else {
        if (_httpLength == (length - (position + endLen))) {
          const char* errMsg = (content + position + endLen);
          _errorMsg = errMsg;
          LOG_ERROR("WS(%p) Position: %d %s", this,
                    (int)(length - (position + endLen)), errMsg);
          return -(WsResponsePackageFailed);
        }
      }
    }
  }

  return -(WsResponsePackageFailed);
}

int WebSocketTcp::receiveFullWebSocketFrame(uint8_t* frame, size_t frameSize,
                                            WebSocketHeaderType* wsType,
                                            WebSocketFrame* resultDate) {
  int retCode = Success;
  // LOG_DEBUG("begin receiveFullWebSocketFrame.");
  switch (_rStatus) {
    case WsHeadSize:
      // LOG_DEBUG("WsHeadSize.");
      retCode = decodeHeaderSizeWebSocketFrame(frame, frameSize, wsType);
      // LOG_DEBUG("WS(%p) Parse decodeHeaderSizeWebSocketFrame
      // wsType->opCode:%d.", this, wsType->opCode);
      if (retCode < 0) {
        LOG_ERROR("WS(%p) Parse WsHeadSize Failed, retCode:%d.", this, retCode);
        return retCode;
      }
      _rStatus = WsHeadBody;
    case WsHeadBody:
      // LOG_DEBUG("WsHeadBody.");
      retCode = decodeHeaderBodyWebSocketFrame(frame, frameSize, wsType);
      // LOG_DEBUG("WS(%p) Parse decodeHeaderBodyWebSocketFrame
      // wsType->opCode:%d.", this, wsType->opCode);
      if (retCode < 0) {
        if (wsType->opCode == WebSocketHeaderType::PONG) {
          LOG_DEBUG("This WS(%p) is PONG, please ignore this warn.", this);
          retCode = Success;
        } else {
          LOG_ERROR(
              "WS(%p) Parse WsHeadBody Failed, retCode:%d, "
              "wsType->headerSize:%d, frameSize:%d.",
              this, retCode, wsType->headerSize, frameSize);
          return retCode;
        }
      }
      _rStatus = WsContentBody;
    case WsContentBody:
      // LOG_DEBUG("WsContentBody.");
      retCode =
          decodeFrameBodyWebSocketFrame(frame, frameSize, wsType, resultDate);
      // LOG_DEBUG("WS(%p) Parse decodeFrameBodyWebSocketFrame wsType->opCode:%d
      // with retCode:%d.", this, wsType->opCode, retCode);
      if (retCode < 0) {
        // LOG_ERROR("WS(%p) Parse WsContentBody Failed, retCode:%d.", this,
        // retCode);
        return retCode;
      }
      _rStatus = WsHeadSize;
      return Success;
    default:
      LOG_WARN("WS(%p) Default None. _rStatus:%d.", this, _rStatus);
      break;
  }

  // LOG_DEBUG("receiveFullWebSocketFrame done.");
  return Success;
}

/**
 * @brief: 从Websocket帧中解析出HeaderSize
 * @return: 成功则返回收到的字节数, 失败则返回负值.
 */
int WebSocketTcp::decodeHeaderSizeWebSocketFrame(uint8_t* buffer, size_t length,
                                                 WebSocketHeaderType* wsType) {
  if (length < 2) {
    return -(InvalidWsFrameHeaderSize); /* Need at least 2 */
  }

  const uint8_t* data = buffer;  // peek, but don't consume
  wsType->fin = (data[0] & 0x80) == 0x80;
  wsType->opCode = (WebSocketHeaderType::OpCodeType)(data[0] & 0x0f);
  wsType->mask = (data[1] & 0x80) == 0x80;
  wsType->N0 = (data[1] & 0x7f);
  wsType->headerSize = 2 + (wsType->N0 == 126 ? 2 : 0) +
                       (wsType->N0 == 127 ? 8 : 0) + (wsType->mask ? 4 : 0);

  //  LOG_DEBUG("wsType->headerSize: %d, opCode: %d, fin: %d",
  //      wsType->headerSize, wsType->opCode, wsType->fin);

  return Success;
}

/**
 * @brief: 从Websocket帧中解析出HeaderBody
 * @return: 成功则返回收到的字节数, 失败则返回负值.
 */
int WebSocketTcp::decodeHeaderBodyWebSocketFrame(uint8_t* buffer, size_t length,
                                                 WebSocketHeaderType* wsType) {
  if (wsType->headerSize >= length) {
    return -(InvalidWsFrameHeaderBody);
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

  // LOG_DEBUG("wsType->N: %d", wsType->N);
  return Success;
}

/**
 * @brief: 从Websocket帧中解析出FrameBody
 * @return: 成功则返回收到的字节数, 失败则返回负值.
 */
int WebSocketTcp::decodeFrameBodyWebSocketFrame(uint8_t* buffer, size_t length,
                                                WebSocketHeaderType* wsType,
                                                WebSocketFrame* receivedData) {
  if (wsType->opCode == WebSocketHeaderType::PONG) {
    return Success;
  }

  if ((wsType->N + wsType->headerSize) > length) {
    // LOG_ERROR("Size: %d %u %zu", wsType->N, wsType->headerSize, length);
    return -(InvalidWsFrameBody);
  }

  // LOG_DEBUG("Size: %d %d %d", wsType->N, wsType->headerSize, length);

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
    return -(InvalidWsFrameBody);
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
    // pass
  } else {
    // LOG_DEBUG("Decoder Receive Data: %zu ", receivedData->length);
  };

  LOG_DEBUG("WS(%p) Decoder received data opCode:%d dataType:%d dataLength:%d.",
            this, wsType->opCode, receivedData->type, receivedData->length);
  return Success;
}

int WebSocketTcp::binaryFrame(const uint8_t* buffer, size_t length,
                              uint8_t** frame, size_t* frameSize) {
  return framePackage(WebSocketHeaderType::BINARY_FRAME, buffer, length, frame,
                      frameSize);
}

int WebSocketTcp::textFrame(const uint8_t* buffer, size_t length,
                            uint8_t** frame, size_t* frameSize) {
  return framePackage(WebSocketHeaderType::TEXT_FRAME, buffer, length, frame,
                      frameSize);
}

int WebSocketTcp::pingFrame(uint8_t** frame, size_t* frameSize) {
  return framePackage(WebSocketHeaderType::PING, NULL, 0, frame, frameSize);
}

int WebSocketTcp::framePackage(WebSocketHeaderType::OpCodeType codeType,
                               const uint8_t* buffer, size_t length,
                               uint8_t** frame, size_t* frameSize) {
  bool useMask = true;
  const uint8_t masKingKey[4] = {0x12, 0x34, 0x56, 0x78};
  const int headlen = 2 + (length >= 126 ? 2 : 0) + (length >= 65536 ? 6 : 0) +
                      (useMask ? 4 : 0);

  uint8_t* header =
      (uint8_t*)calloc(headlen, sizeof(uint8_t));  // new uint8_t[headlen];
  if (header == NULL) {
    LOG_ERROR("WS(%p) calloc header failed.", this);
    return -(MallocFailed);
  }

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
  } else {  // TODO: run coverage testing here
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

  // N.B. - tmpBuffer will keep growing until it can be transmitted over the
  // socket: uint8_t * tmpBuffer = new uint8_t[headlen + length];
  // memset(tmpBuffer, 0, sizeof(uint8_t) * (headlen + length));
  // memcpy(tmpBuffer, header, headlen);
  // memcpy(tmpBuffer + headlen, (uint8_t*)buffer, length);

  *frameSize = (headlen + length);
  *frame = (uint8_t*)calloc(*frameSize, sizeof(uint8_t));
  if (*frame == NULL) {
    LOG_ERROR("WS(%p) calloc frame failed.", this);
    free(header);
    return -(MallocFailed);
  }
  memcpy(*frame, header, headlen);
  if (buffer && length > 0) {
    memcpy(*frame + headlen, (uint8_t*)buffer, length);
  }

#ifdef OPU_DEBUG
  std::ofstream ofs;
  ofs.open("./out.opus", std::ios::out | std::ios::app | std::ios::binary);
  if (ofs.is_open() && buffer) {
    ofs.write((const char*)buffer, length);
    ofs.flush();
    ofs.close();
  }
#endif

  // LOG_DEBUG("framePackage Receive Data: %d ", *frameSize);

  if (useMask) {
    for (size_t i = 0; i != length; ++i) {
      *(*frame + headlen + length - length + i) ^= masKingKey[i & 0x3];
    }
  }

  free(header);
  header = NULL;

  return Success;
}

}  // namespace AlibabaNls
