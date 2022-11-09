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

#include <cstring>
#include <string>
#include <stdint.h>

namespace AlibabaNls {

#define BUFFER_SIZE 2048  //1024
#define READ_BUFFER_SIZE 20480

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
  bool fin;
  bool mask;
  enum OpCodeType {
    CONTINUATION = 0x0,
    TEXT_FRAME = 0x1,
    BINARY_FRAME = 0x2,
    CLOSE = 8,
    PING = 9,
    PONG = 0xa,
  } opCode;
  int N0;
  uint64_t N;
  uint8_t masKingKey[4];
};

struct WebSocketFrame {
  WebSocketHeaderType::OpCodeType type;
  uint8_t * data;
  size_t length;
  int closeCode;
};

#define HOST_SIZE 256
#define TOKEN_SIZE 64

struct urlAddress {
  char _host[HOST_SIZE];
  int _port;
  char _type[10];
  char _path[HOST_SIZE];
  char _token[TOKEN_SIZE];
  bool _isSsl;
  char _address[HOST_SIZE];
  bool _directIp;
  bool _enableSysGetAddr;
};

class WebSocketTcp {
 public:
  WebSocketTcp();
  ~WebSocketTcp();

  int requestPackage(urlAddress * url, char* buffer, std::string httpHeader);
  int responsePackage(const char * content, size_t length);

  int framePackage(WebSocketHeaderType::OpCodeType type,
                   const uint8_t * buffer, size_t length,
                   uint8_t** frame, size_t * frameSize);
  int binaryFrame(const uint8_t * buffer, size_t length,
                  uint8_t** frame, size_t * frameSize);
  int textFrame(const uint8_t * buffer, size_t length,
                uint8_t** frame, size_t * frameSize);

  int receiveFullWebSocketFrame(uint8_t * frame, size_t frameSize,
                                WebSocketHeaderType* ws, WebSocketFrame* rData);
  int decodeHeaderSizeWebSocketFrame(uint8_t * buffer, size_t length,
                                     WebSocketHeaderType* wsType);
  int decodeHeaderBodyWebSocketFrame(uint8_t * buffer, size_t length,
                                     WebSocketHeaderType* wsType);
  int decodeFrameBodyWebSocketFrame(uint8_t * buffer, size_t length,
                                    WebSocketHeaderType* ws,
                                    WebSocketFrame* receivedData);

  const char* getFailedMsg();

 private:
  size_t _httpCode;
  size_t _httpLength;

  WebSocketReceiveStatus _rStatus;
  std::string _errorMsg;
  std::string _secWsKey;

  int getTargetLen(std::string line, const char* begin, const char* end);
  const char* getSecWsKey();
  const char* getRequestForLog(char *buf_in, std::string *buf_out);
};

}  // namespace AlibabaNls


#endif // NLS_SDK_WEBSOCKET_TCP_H
