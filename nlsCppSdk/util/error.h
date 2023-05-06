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

#ifndef NLS_SDK_ERROR_H
#define NLS_SDK_ERROR_H

#include <string.h>

namespace AlibabaNls {

namespace utility {

#define ERROR_MESSAGE_LEN 512

class BaseError {
public:
    BaseError() {
        _errorCode = 0;
        memset(_errorMessage, 0x0, ERROR_MESSAGE_LEN);
    };
    virtual ~BaseError() {};

    virtual inline void setErrorCode(int code) {_errorCode = code;};
    virtual inline void setErrorMessage(const char* message, size_t length) {
        memset(_errorMessage, 0x0, ERROR_MESSAGE_LEN);
        memcpy(_errorMessage, message, length);
    };

    virtual inline int getErrorCode() {return _errorCode;};
    virtual inline const char* getErrorMessage() {return _errorMessage;};

private:

    unsigned long _errorCode;
    char _errorMessage[ERROR_MESSAGE_LEN];
};

}

}

#endif /*NLS_SDK_ERROR_H*/
