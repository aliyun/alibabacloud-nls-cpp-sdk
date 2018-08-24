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

#include "log.h"
#include "exception.h"

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__ANDROID__) || defined(__linux__)
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#ifndef _ANDRIOD_
#include <iconv.h>
#endif

#include <string.h>

#endif

using std::string;

namespace util {

bool zlog_debug = true;
pthread_mutex_t Log::mtxOutput = PTHREAD_MUTEX_INITIALIZER;
FILE *Log::_output = stdout;
int Log::_logLevel = 1;

void Log::setLogEnable(bool enable) {
    zlog_debug = enable;
}

string Log::UTF8ToGBK(const string &strUTF8) {

#if defined(__ANDROID__) || defined(__linux__)

    const char *msg = strUTF8.c_str();
    size_t inputLen = strUTF8.length();
    size_t outputLen = inputLen * 20;

    char *outbuf = new char[outputLen + 1];
    memset(outbuf, 0x0, outputLen + 1);

    char *inbuf = new char[inputLen + 1];
    memset(inbuf, 0x0, inputLen + 1);
    strncpy(inbuf, msg, inputLen);

    int res = Log::code_convert((char *)"UTF-8", (char *)"GBK", inbuf, inputLen, outbuf, outputLen);
    if (res == -1) {
        throw ExceptionWithString("convert to utf8 error", errno);
    }

    string strTemp(outbuf);

    delete [] outbuf;
    outbuf = NULL;
    delete [] inbuf;
    inbuf = NULL;

    return strTemp;

#elif defined (_WIN32)

    int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
    unsigned short * wszGBK = new unsigned short[len + 1];
    memset(wszGBK, 0, len * 2 + 2);

    MultiByteToWideChar(CP_UTF8, 0, (char*)strUTF8.c_str(), -1, (wchar_t*)wszGBK, len);

    len = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszGBK, -1, NULL, 0, NULL, NULL);

    char *szGBK = new char[len + 1];
    memset(szGBK, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszGBK, -1, szGBK, len, NULL, NULL);

    string strTemp(szGBK);
    delete [] szGBK;
    delete [] wszGBK;

    return strTemp;

#else

    return strUTF8;

#endif

}

string Log::GBKToUTF8(const string &strGBK) {

#if defined(__ANDROID__) || defined(__linux__)

    string strOutUTF8 = "";

    size_t outputLen = strGBK.length() * 20;
    char *outbuf = new char[outputLen + 1];
    memset(outbuf, 0x0, outputLen + 1);

    size_t inputLen = strGBK.length();
    char *inbuf = new char[inputLen + 1];
    memset(inbuf, 0x0, inputLen + 1);
    strncpy(inbuf, strGBK.c_str(), inputLen);

    int res = Log::code_convert((char *)"GBK", (char *)"UTF-8", inbuf, inputLen, outbuf, outputLen);
    if (res == -1) {
        LOG_ERROR("convert to utf8 error, error code is %d", errno);
    }

    strOutUTF8 = string(outbuf);

    delete [] inbuf;
    inbuf = NULL;

    delete [] outbuf;
    outbuf = NULL;

    return strOutUTF8;

#elif defined (_WIN32)

    string strOutUTF8 = "";
    WCHAR * str1;

    int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);

    str1 = new WCHAR[n];

    MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);

    n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);

    char * str2 = new char[n];
    WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
    strOutUTF8 = str2;

    delete [] str1;
    str1 = NULL;
    delete [] str2;
    str2 = NULL;

    return strOutUTF8;

#else

    return strGBK;

#endif

}

#if defined(__ANDROID__) || defined(__linux__)

int Log::code_convert(char *from_charset, char *to_charset, char *inbuf, size_t inlen, char *outbuf, size_t outlen) {

#if defined(_ANDRIOD_)
    outbuf = inbuf;
#else
    iconv_t cd;
    int rc;
    char **pin = &inbuf;
    char **pout = &outbuf;

    cd = iconv_open(to_charset, from_charset);
    if (cd == 0) {
        return -1;
    }

    memset(outbuf, 0, outlen);
    if (iconv(cd, pin, &inlen, pout, &outlen) == -1) {
        return -1;
    }
    iconv_close(cd);
#endif

    return 0;

}

#endif

void sleepTime(int ms) {
#if defined (_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

unsigned long PthreadSelf() {

#if defined(__ANDROID__) || defined(__linux__)
    return pthread_self();
#elif defined (_WIN32)
	return GetCurrentThreadId();
#else
    return pthread_self()->__sig;
#endif

}

}


