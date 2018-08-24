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

#ifndef NLS_SDK_LOG_H
#define NLS_SDK_LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <ctime>
#include "pthread.h"
#ifdef _ANDRIOD_
#include <android/log.h>
#define LOG_TAG "AliSpeechLib"
#endif

namespace util {

#ifdef _WIN32
    #define _ssnprintf _snprintf
#else
//    #include <unistd.h>
    #define _ssnprintf snprintf
#endif

extern bool zlog_debug;

void sleepTime(int ms);
unsigned long PthreadSelf();

class Log {
public:
    static void setLogEnable(bool enable);

    static std::string UTF8ToGBK(const std::string &strUTF8);
    static std::string GBKToUTF8(const std::string &strGBK);

    static FILE *_output;
    static int _logLevel;
    static pthread_mutex_t mtxOutput;

#if defined(__ANDROID__) || defined(__linux__)
    static int code_convert(char *from_charset,
                            char *to_charset,
                            char *inbuf,
                            size_t inlen,
                            char *outbuf,
                            size_t outlen);
#endif
};

#ifdef _ANDRIOD_
    #define LOG_VERBOSE(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, ##__VA_ARGS__)
    #define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG, ##__VA_ARGS__)
    #define LOG_INFO(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG, ##__VA_ARGS__)
    #define LOG_WARN(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG, ##__VA_ARGS__)
    #define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG, ##__VA_ARGS__)
    #define LOG_EXCEPTION(...) __android_log_print(ANDROID_LOG_FATAL,LOG_TAG, ##__VA_ARGS__)
#else
    #define LOG_PRINT_COMMON(level, ...) { \
                                     char log_str[1024] = {0}; \
                                     char res[11264] = {0}; \
                                     _ssnprintf(log_str, 1024, __VA_ARGS__); \
                                     time_t tt = time(NULL); \
                                     struct tm* ptm = localtime(&tt); \
                                     _ssnprintf(res, 11264, "%4d-%02d-%02d %02d:%02d:%02d AliSpeech_C++SDK(%s)[%lu]: %s:%d %s",\
                                                            (int)ptm->tm_year + 1900, \
                                                            (int)ptm->tm_mon + 1, \
                                                            (int)ptm->tm_mday, \
                                                            (int)ptm->tm_hour, \
                                                            (int)ptm->tm_min, \
                                                            (int)ptm->tm_sec, \
                                                            level, \
                                                            PthreadSelf(),\
                                                            __FUNCTION__, \
                                                            __LINE__, \
                                                            log_str); \
                                     pthread_mutex_lock(&Log::mtxOutput); \
                                     fprintf(Log::_output, "%s\n", res); \
                                     pthread_mutex_unlock(&Log::mtxOutput); \
                                     }

    #define LOG_VERBOSE(...) if (zlog_debug) { LOG_PRINT_COMMON("VERBOSE", __VA_ARGS__) }
    #define LOG_DEBUG(...) if (Log::_logLevel >= 4) { LOG_PRINT_COMMON("DEBUG",__VA_ARGS__) }
    #define LOG_INFO(...) if (Log::_logLevel >= 3) { LOG_PRINT_COMMON("INFO",__VA_ARGS__) }
    #define LOG_WARN(...) if (Log::_logLevel >= 2) { LOG_PRINT_COMMON("WARN",__VA_ARGS__) }
    #define LOG_ERROR(...) if (Log::_logLevel >= 1) { LOG_PRINT_COMMON("ERROR",__VA_ARGS__) }
    #define LOG_EXCEPTION(...) LOG_PRINT_COMMON("!!!!EXCEPTION",__VA_ARGS__)
#endif
}

#endif //NLS_SDK_LOG_H
