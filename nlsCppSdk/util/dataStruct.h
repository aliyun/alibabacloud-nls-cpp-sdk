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

#ifndef NLS_SDK_DATA_STRUCT_H
#define NLS_SDK_DATA_STRUCT_H

#include <string>
#include <stdint.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include "exception.h"

namespace util {
#if defined(_WIN32)
#ifndef snprintf
#define snprintf _snprintf_s
#endif
#endif

class WebSocketAddress {
public:
    char type[10];
    char path[128];
    char host[128];
    int port;

    WebSocketAddress() {

	memset(type, 0x0, 10);
	memset(path, 0x0, 128);
	memset(host, 0x0, 128);
        port = 80;
    }

    static WebSocketAddress urlConvert2WebSocketAddress(std::string url) {
        WebSocketAddress wsa;

	if (url.empty()) {
		throw ExceptionWithString("ERROR: The url is empty", 10000019);
	}

        if (sscanf(url.c_str(), "%[^:/]://%[^:/]:%d/%s", wsa.type, wsa.host, &wsa.port, wsa.path) == 4) {

        } else if (sscanf(url.c_str(), "%[^:/]://%[^:/]/%s", wsa.type, wsa.host, wsa.path) == 3) {
            wsa.port = (strcmp(wsa.type, "wss") == 0 || strcmp(wsa.type, "https") == 0) ? 443 : 80;
        } else if (sscanf(url.c_str(), "%[^:/]://%[^:/]:%d", wsa.type, wsa.host, &wsa.port) == 3) {
            wsa.path[0] = '\0';
        } else if (sscanf(url.c_str(), "%[^:/]://%[^:/]", wsa.type, wsa.host) == 2) {
            wsa.port = (strcmp(wsa.type, "wss") == 0 || strcmp(wsa.type, "https") == 0) ? 443 : 80;
            wsa.path[0] = '\0';
        } else {
            throw ExceptionWithString("ERROR: Could not parse WebSocket url: " + url, 10000018);
        }
        return wsa;
    }
};

struct WsheaderType {
    unsigned header_size;
    bool fin;
    bool mask;
    enum OpcodeType {
        CONTINUATION = 0x0,
        TEXT_FRAME = 0x1,
        BINARY_FRAME = 0x2,
        CLOSE = 8,
        PING = 9,
        PONG = 0xa,
    } opcode;
    int N0;
    uint64_t N;
    uint8_t masking_key[4];
};

union StatusCode {
    unsigned short code;
    char pdata[2];
};

typedef enum ResponseDataType {
    TEXT = 0,
    BINARY,
    ERRORAUTH
} ResponseDataType;

struct WebsocketFrame {
    WsheaderType::OpcodeType type;
    std::vector<uint8_t> data;
    int closecode;
};

enum RequestMode {
    SR = 0,
    ST,
    WWV,
    SY,
    TGA,
    VPR,
    VPM
};

enum ResponseMode {
    Streaming = 0,
    Normal
};

void int2ByteArray(int32_t *data, int len, uint8_t *result, bool isBigEndian);
char *base64Encode(const char *input, int length, bool with_new_line);

}

#endif
