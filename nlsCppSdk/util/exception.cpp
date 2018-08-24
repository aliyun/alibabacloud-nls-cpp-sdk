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

#include "exception.h"
#include "wstr2str.h"

using std::wstring;
using std::string;

namespace util {

wstring ExceptionWithString::GetMiniDumpFilePath() {
    return wstr;
}

const char *ExceptionWithString::what() {
    return _errormsg.c_str();
}

int ExceptionWithString::getErrorcode() {
    return this->_errcode;
}

ExceptionWithString::ExceptionWithString(const string &msg, int errorcode) : _errcode(errorcode),
                                                                             _errormsg(msg) {

}

ExceptionWithString::ExceptionWithString(wstring const &msg, int errorcode, wstring miniDumpFileFullPath) : _errormsg(wstr2str(msg)),
                                                                                                            _errcode(errorcode),
                                                                                                            wstr((miniDumpFileFullPath)) {

}

}
