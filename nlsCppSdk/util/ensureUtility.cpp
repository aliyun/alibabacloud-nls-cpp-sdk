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

#include "ensureUtility.h"
#include <sstream>

using std::wstring;
using std::wstringstream;

namespace AlibabaNls {
namespace util {

#ifdef _WIN32
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "dbghelp.lib")
#endif

wstring FormatMessage(wstring const &expr,
                      TContextVarMap const &contextVars,
                      wstring const &file, long line) {

    wstringstream ss;
    ss << expr << L" [" << file << L", " << line << L"]\n";

    if (!contextVars.empty()) {
        TContextVarMap::const_iterator p = contextVars.begin();
        for (; p != contextVars.end(); p++) {
            ss << L"    " << p->first << L" = " << p->second << L"\n";
        }
    }

    return ss.str();
}

void ThrowWithoutDumpBehavior::operator()(std::string msg, int errocde) {
    throw ExceptionWithString(msg, errocde);
}

}
}
