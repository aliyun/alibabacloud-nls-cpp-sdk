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
#include <Security/SecureTransport.h>
#include <Security/SecPolicy.h>
#include <Security/SecItem.h>
#include <stdint.h>

namespace AlibabaNls {

namespace transport {

#define ERROR_MESSAGE_LEN 256

typedef int handle_t;

struct SslCustomParam {
    handle_t socketFd;
};
typedef struct SslCustomParam sslCustomParam_t;

class IosSslConnect {

    static int sslWaitRead(handle_t socketFd);
    static int sslWaitWrite(handle_t socketFd);

    static OSStatus sslReadCallBack(SSLConnectionRef ctxHandle, void* data, size_t* dataLen);
    static OSStatus sslWriteCallBack(SSLConnectionRef ctxHandle, const void* data, size_t* dataLen);

public:
    IosSslConnect();
    ~IosSslConnect();

    void* iosSslHandshake(sslCustomParam_t* customParam, const char* hostname);
    int iosSslWrite(SSLContextRef ctx, const void* buf, size_t len);
    ssize_t iosSslRead(SSLContextRef ctx, void* buf, size_t len);
    void iosSslClose();

    SSLContextRef getCtxHandle();
    char* getErrorMsg();

private:
    SSLContextRef _ctxHandle;
    char _errorMsg[ERROR_MESSAGE_LEN];
};

}

}