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

#include "wstr2str.h"
#include <stdlib.h>

using std::string;
using std::wstring;

namespace util {

string wstr2str(const wstring &wstr) {
    size_t len = wstr.length();
    if (len == 0) {
        return "";
    }

    string str(len, '\0');

    for (int i = 0; i < len; i++) {
        str[i] = (char) wstr[i];
    }
    return str;
}

wstring str2wstr(const string &str) {
    size_t len = str.length();
    wstring wstr(len, L'\0');
    if (len == 0) {
        return L"";
    }

    for (int i = 0; i < len; i++) {
        wstr[i] = (wchar_t) str[i];
    }
    return wstr;
}

}
