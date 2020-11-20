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

#ifndef NLS_SDK_IOS_SSL_H
#define NLS_SDK_IOS_SSL_H

#include <string>
#include <Security/SecureTransport.h>
#include <Security/SecPolicy.h>
#include <Security/SecItem.h>
#include <stdint.h>
#include <pthread.h>

namespace AlibabaNls {

typedef int handle_t;

struct SslCustomParam {
    handle_t socketFd;
};
typedef struct SslCustomParam sslCustomParam_t;

class SslConnect {
    static int sslWaitRead(handle_t socketFd);
    static int sslWaitWrite(handle_t socketFd);

    static OSStatus sslReadCallBack(SSLConnectionRef ctxHandle, void* data, size_t* dataLen);
    static OSStatus sslWriteCallBack(SSLConnectionRef ctxHandle, const void* data, size_t* dataLen);

public:
    SslConnect();
    ~SslConnect();

    pthread_mutex_t  _mtxSsl;

    int sslHandshake(int socketFd, const char* hostname);
    ssize_t sslWrite(const uint8_t * buffer, size_t len);
    ssize_t sslRead(uint8_t * buffer, size_t len);
    void sslClose();

    const char* getFailedMsg();

private:
    SSLContextRef _ctxHandle;
    sslCustomParam_t _iosSslParam;

    std::string _errorMsg;
};

}

#endif //NLS_SDK_IOS_SSL_H
