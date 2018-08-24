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

#ifndef NLS_SDK_SMARTHANDLE_H
#define NLS_SDK_SMARTHANDLE_H

#include "errorHandlingUtility.h"
//#include "util/log.h"

#if defined(__GNUC__)
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR (-1)
#define WSAENOTSOCK (0)
#define closesocket close
#define WSAGetLastError() 0
#endif

#if defined(_WIN32)
#include <winsock2.h>
#endif

namespace util {

template <typename HandleT>
struct HandleReleaser;

template <>
struct HandleReleaser < SOCKET > {
    void operator()(SOCKET sockfd) const {
        if (closesocket(sockfd) == SOCKET_ERROR) {
            int error = (WSAGetLastError());
            if (error != WSAENOTSOCK) {
                LOG_DEBUG("error during socket closing: %d", error);
            }
        }
    }
};

template <typename HandleT>
struct HandleVerifier;

template <>
struct HandleVerifier < SOCKET > {
    bool operator()(SOCKET hsocket) const {
        return hsocket != INVALID_SOCKET;
    }
};

template <typename HandleT, typename Releaser>
class HandleOwner {
public:
    HandleOwner(HandleT handle)
        : handle_(handle) {

    }

    ~HandleOwner() {
        //Releaser()(handle_);
    }

    HandleT GetHandle() {
        return handle_;
    }

    void ResetHandle() {
        handle_ = INVALID_SOCKET;
    }

    HandleT handle_;

public:
    HandleOwner(HandleOwner const& _handle){ handle_ = _handle.handle_; };
    HandleOwner& operator=(HandleOwner const&){}
};

template <typename HandleT, typename Releaser = HandleReleaser<HandleT>, typename Verifier = HandleVerifier<HandleT> >
class SmartHandle : public  HandleOwner<HandleT, Releaser> {
    typedef HandleOwner<HandleT, Releaser> owner_t;
public:
    SmartHandle() {}
    explicit SmartHandle(HandleT handle):owner_t(handle) {
        ENSURE_WIN32(Verifier()(handle));
    }

    HandleT get() {
        return this->GetHandle();
    }

    void Reset() {
        this->ResetHandle();
    }

};

}

#endif //NLS_SDK_SMARTHANDLE_H
