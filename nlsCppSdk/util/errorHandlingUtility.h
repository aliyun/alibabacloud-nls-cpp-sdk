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

#ifndef NLS_SDK_ERROR_HANDLING_UTILITY_H
#define NLS_SDK_ERROR_HANDLING_UTILITY_H

#include "ensureUtility.h"
#include "log.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace util {

#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)

// ENSURE
#define ENSURE(expr) if ((expr));else MakeEnsureThrowWithoutDump(WIDEN(#expr))

// ENSURE_WIN32
#if defined(_WIN32)
inline std::wstring Win32ErrorMessage(DWORD dwError) {
	LPVOID pText = 0;
    return L"Unknown Error";
}
#endif

#if defined(_WIN32)
#define ENSURE_WIN32(exp) ENSURE(exp)
#define ENSURE_SUCCEEDED(hr) \
    if(SUCCEEDED(hr)) ; \
    else ENSURE(SUCCEEDED(hr))(Win32ErrorMessage(hr))
#elif defined(__GNUC__)
#define ENSURE_WIN32(exp)
#define ENSURE_SUCCEEDED(hr) ENSURE((hr)>=0)
#endif

// LOGGING
#define LOGGING(msg)  LOG_DEBUG(msg)

// exception
template<typename T>
void MuteAllExceptions(void(T::*action)(), T *ptr, std::string msg = "MuteAllExceptions!") {
    try {
        (ptr->*action)();
    }
    catch (...) {
        LOG_DEBUG("Exception Catched: %s\n", msg.c_str());
    }
}

}

#endif //NLS_SDK_ERROR_HANDLING_UTILITY_H
