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

#ifndef NLS_SDK_WEBSOCKET_TCP_H
#define NLS_SDK_WEBSOCKET_TCP_H

#include <stdint.h>

#include <cstring>
#include <string>

namespace AlibabaNls {

enum WebSocketConstValue {
  HttpPort = 80,
  TypeSize = 10,
  HostSize = 256,
  PathSize = 512,
  TokenSize = 512,
  BufferSize = 2048,  // 1024
  ReadBufferSize = 30720,
};

union StatusCode {
  unsigned short status;
  char frame[2];
};

enum WebSocketReceiveStatus {
  WsHeadSize = 0,
  WsHeadBody,
  WsContentBody,
  WsWsFailed
};

struct WebSocketHeaderType {
  unsigned headerSize;
  bool fin; /* 0bit */
  bool mask;
  enum OpCodeType {
    CONTINUATION = 0x0,
    TEXT_FRAME = 0x1,
    BINARY_FRAME = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xa,
  } opCode;   /* 4-7bit */
  int N0;     /* store payload len */
  uint64_t N; /* payload len bytes */
  uint8_t masKingKey[4];
};

struct WebSocketFrame {
  WebSocketHeaderType::OpCodeType type;
  uint8_t* data;
  size_t length;
  int closeCode;
};

struct urlAddress {
  char _host[HostSize];
  int _port;
  char _type[TypeSize];
  char _path[PathSize];
  char _token[TokenSize];
  bool _isSsl;
  char _address[HostSize];
  bool _directIp;
  bool _enableSysGetAddr;
};

class WebSocketTcp {
 public:
  WebSocketTcp();
  ~WebSocketTcp();

  void setConnectNode(void* node) { _nodeHandle = node; }
  void* getConnectNode() { return _nodeHandle; }

  int requestPackage(urlAddress* url, char* buffer, std::string httpHeader);
  int responsePackage(const char* content, size_t length);

  int framePackage(WebSocketHeaderType::OpCodeType type, const uint8_t* buffer,
                   size_t length, uint8_t** frame, size_t* frameSize);
  int binaryFrame(const uint8_t* buffer, size_t length, uint8_t** frame,
                  size_t* frameSize);
  int textFrame(const uint8_t* buffer, size_t length, uint8_t** frame,
                size_t* frameSize);
  int pingFrame(uint8_t** frame, size_t* frameSize);

  int receiveFullWebSocketFrame(uint8_t* frame, size_t frameSize,
                                WebSocketHeaderType* ws, WebSocketFrame* rData);
  int decodeHeaderSizeWebSocketFrame(uint8_t* buffer, size_t length,
                                     WebSocketHeaderType* wsType);
  int decodeHeaderBodyWebSocketFrame(uint8_t* buffer, size_t length,
                                     WebSocketHeaderType* wsType);
  int decodeFrameBodyWebSocketFrame(uint8_t* buffer, size_t length,
                                    WebSocketHeaderType* ws,
                                    WebSocketFrame* receivedData);

  const char* getFailedMsg();

  static int parseUrlAddress(struct urlAddress& url, const char* address);
  static bool urlWithAccess(const char* address);

 private:
  size_t _httpCode;
  size_t _httpLength;

  WebSocketReceiveStatus _rStatus;
  std::string _errorMsg;
  std::string _secWsKey;

  void* _nodeHandle;

  int getTargetLen(std::string line, const char* begin, const char* end);
  const char* getSecWsKey();
  const char* getRequestForLog(char* buf_in, std::string* buf_out);
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_WEBSOCKET_TCP_H
