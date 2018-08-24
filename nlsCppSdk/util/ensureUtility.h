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

#ifndef NLS_SDK_ENSURE_UTILITY_H
#define NLS_SDK_ENSURE_UTILITY_H

#include <string>
#include <map>
#include "exception.h"
#include "wstr2str.h"

namespace util {

typedef std::map<std::wstring, std::wstring> TContextVarMap;

// format message
std::wstring FormatMessage(
        std::wstring const &expr,
        TContextVarMap const &contextVars,
        std::wstring const &file,
        long line);

// Ensure_Guard
template<typename TBehavior>
class Ensure_Guard {
public:
    explicit Ensure_Guard(std::wstring msg) : _msg(wstr2str(msg)) {}

    Ensure_Guard(int err) : errcode(err) {}

    ~Ensure_Guard() {}

    void operator()() const {
        TBehavior()(_msg, 10010);
    }

    void operator()(int err) const {
        TBehavior()(_msg, err);
    }

private:
    std::string _msg;
    int errcode;
};

#pragma warning(push)
#pragma warning(disable: 4355)
#pragma warning(pop)

struct ThrowWithoutDumpBehavior {
    void operator()(std::string msg, int errocde);
};

inline Ensure_Guard<ThrowWithoutDumpBehavior> MakeEnsureThrowWithoutDump(std::wstring expr) {
    return Ensure_Guard<ThrowWithoutDumpBehavior>(expr);
}

}

#endif //NLS_SDK_ENSURE_UTILITY_H
