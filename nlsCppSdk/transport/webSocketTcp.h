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

#ifndef NLS_SDK_WEBSOCKET_TCP_H
#define NLS_SDK_WEBSOCKET_TCP_H

#include <cstring>
#include <vector>
#include <string>
#include "socket.h"
#include "dataStruct.h"

#if !defined( __APPLE__ )
#include "openssl/ssl.h"
#else
#include "iosSsl.h"
#endif

namespace AlibabaNls {
namespace transport {

class WebSocketTcp : public Socket {

#if defined(_MSC_VER)
    static pthread_mutex_t _sslMutex;
#endif

public:
    static WebSocketTcp* connectTo(const util::WebSocketAddress& addr, int timeOut, std::string token);
    WebSocketTcp(util::SmartHandle<SOCKET> sockfd, int timeOut, const util::WebSocketAddress& addr, std::string token);
    ~WebSocketTcp();

    int ws_read(void *buf, size_t num);
    int ws_write(const void *buf, size_t num);

    template<class Iterator>
    int sendData(util::WsheaderType::OpcodeType type, int message_size, Iterator message_begin, Iterator message_end);

    template<class Iterator>
    int sendBinaryData(int message_size, Iterator message_begin, Iterator message_end);

    template<class Iterator>
    int sendTextData(int message_size, Iterator message_begin, Iterator message_end);

    int RecvDataBySize(std::vector<unsigned char>& buffer, int size);
    int RecvFullWebSocketFrame(std::vector<unsigned char>& frame, util::WsheaderType& ws, util::WebsocketFrame& receivedData, int& errorCode);
    void DecodeHeaderSizeWebSocketFrame(std::vector<unsigned char> buffer, util::WsheaderType& ws);
    void DecodeHeaderBodyWebSocketFrame(std::vector<unsigned char> buffer, util::WsheaderType& ws);
    int DecodeFrameBodyWebSocketFrame(std::vector<unsigned char> buffer, util::WsheaderType& ws, util::WebsocketFrame& receivedData);

    void CloseSsl();

private:
    bool ConnectToHttp(const util::WebSocketAddress addr, std::string token);
    bool InitializeSslContext();
    int send(uint8_t* data, int len);
    int getTargetLen(std::string line, const char* begin, const char* end);

    bool _useMask;
    bool _useSSL;

#if !defined( __APPLE__ )
    SSL * _ssl;
    SSL_CTX * _sslCtx;
#else
    sslCustomParam_t _iosSslParam;
    IosSslConnect* _iosSslHandle;
#endif

    bool _blockSigpipe;
};

template<class Iterator>
int WebSocketTcp::sendBinaryData(int message_size, Iterator message_begin, Iterator message_end) {
    return sendData(util::WsheaderType::BINARY_FRAME, message_size, message_begin, message_end);
}

template<class Iterator>
int WebSocketTcp::sendTextData(int message_size, Iterator message_begin, Iterator message_end) {
    return sendData(util::WsheaderType::TEXT_FRAME, message_size, message_begin, message_end);
}

template<class Iterator>
int WebSocketTcp::sendData(util::WsheaderType::OpcodeType type,
                           int message_size,
                           Iterator message_begin,
                           Iterator message_end) {

    const uint8_t masking_key[4] = { 0x12, 0x34, 0x56, 0x78 };
    const int headlen = 2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + (_useMask ? 4 : 0);
//    uint8_t header[headlen];

    uint8_t* header = new uint8_t[headlen];
    memset(header, 0, sizeof(uint8_t)*headlen);
    header[0] = 0x80 | type;

    if (message_size < 126) {
        header[1] = (message_size & 0xff) | (_useMask ? 0x80 : 0);
        if (_useMask) {
            header[2] = masking_key[0];
            header[3] = masking_key[1];
            header[4] = masking_key[2];
            header[5] = masking_key[3];
        }
    } else if (message_size < 65536) {
        header[1] = 126 | (_useMask ? 0x80 : 0);
        header[2] = (message_size >> 8) & 0xff;
        header[3] = (message_size >> 0) & 0xff;
        if (_useMask) {
            header[4] = masking_key[0];
            header[5] = masking_key[1];
            header[6] = masking_key[2];
            header[7] = masking_key[3];
        }
    } else { // TODO: run coverage testing here
        header[1] = 127 | (_useMask ? 0x80 : 0);
        header[2] = ((uint64_t)message_size >> 56) & 0xff;
        header[3] = ((uint64_t)message_size >> 48) & 0xff;
        header[4] = ((uint64_t)message_size >> 40) & 0xff;
        header[5] = ((uint64_t)message_size >> 32) & 0xff;
		header[6] = ((uint64_t)message_size >> 24) & 0xff;
		header[7] = ((uint64_t)message_size >> 16) & 0xff;
		header[8] = ((uint64_t)message_size >> 8) & 0xff;
		header[9] = ((uint64_t)message_size >> 0) & 0xff;
        if (_useMask) {
            header[10] = masking_key[0];
            header[11] = masking_key[1];
            header[12] = masking_key[2];
            header[13] = masking_key[3];
        }
    }

    // N.B. - txbuf will keep growing until it can be transmitted over the socket:
    uint8_t* txbuf = new uint8_t[headlen + message_size];
    memset(txbuf, 0, sizeof(uint8_t) * (headlen + message_size));
    memcpy(txbuf, header, headlen);
    memcpy(txbuf + headlen, &(*message_begin), message_size);

    if (_useMask) {
        for (size_t i = 0; i != message_size; ++i) {
            *(txbuf + headlen + message_size - message_size + i) ^= masking_key[i & 0x3];
        }
    }

    int ret = send(txbuf, (int)(headlen + message_size));
    delete[] txbuf;
    txbuf = NULL;

    delete[] header;
    header = NULL;
    
    if (ret<= 0) {
        return ret;
    }

    return ret - headlen;
}

}
}

#endif //NLS_SDK_WEBSOCKET_TCP_H
