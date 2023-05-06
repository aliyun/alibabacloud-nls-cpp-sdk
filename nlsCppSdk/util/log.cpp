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

#include <stdarg.h>
#include <iostream>
#include <ctime>

#if defined(__ANDRIOD__)
#include <android/log.h>
#elif defined(_WIN32) || defined(__linux__)
#include "log4cpp/Category.hh"
#include "log4cpp/Appender.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/Priority.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/RollingFileAppender.hh"
#endif

#if defined(__ANDRIOD__)
#include <android/log.h>
#endif

#include "log.h"
#include "utility.h"

namespace AlibabaNls {

namespace utility {

using std::string;
using std::cout;
using std::endl;

#define LOG_BUFFER_SIZE 2048
#define LOG_FILES_NUMBER 5
#define LOG_FILE_BASE_SIZE 1024*1024
#define LOG_TAG "AliSpeechLib"

#define LOG_FORMAT_STRING(a, l, f, b)  char tmpBuffer[LOG_BUFFER_SIZE] = {0}; \
                                va_list arg; \
                                va_start(arg, f); \
                                vsnprintf(tmpBuffer, LOG_BUFFER_SIZE, f, arg); \
                                va_end(arg); \
                                _ssnprintf(b, LOG_BUFFER_SIZE, "[ID:%lu][%s:%d]%s", pthreadSelfId(), a, l, tmpBuffer);

#define LOG_PRINT_COMMON(level, message) { \
                                            time_t tt = time(NULL); \
                                            struct tm* ptm = localtime(&tt); \
                                            fprintf(stdout, "%4d-%02d-%02d %02d:%02d:%02d %s(%s): %s\n",\
                                            (int)ptm->tm_year + 1900, \
                                            (int)ptm->tm_mon + 1, \
                                            (int)ptm->tm_mday, \
                                            (int)ptm->tm_hour, \
                                            (int)ptm->tm_min, \
                                            (int)ptm->tm_sec, \
                                            LOG_TAG, \
                                            level, \
                                            message); \
                                        }

//#define CHECK_LOG_OUTPUT(isFlag) if(!isFlag) {return;}

#if defined(_WIN32)
HANDLE NlsLog::_mtxLog = CreateMutex(NULL, FALSE, NULL);
#else
pthread_mutex_t NlsLog::_mtxLog = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsLog* NlsLog::_logInstance = new NlsLog();

void NlsLog::destroyLogInstance() {
    delete _logInstance;
    _logInstance = NULL;
}

NlsLog::NlsLog() {
    _logLevel = 1;
    _isStdout = true;
    _isConfig = false;
}

NlsLog::~NlsLog() {
    _isStdout = true;
    _isConfig = false;

#if (!defined(__ANDRIOD__)) && (!defined(__APPLE__))
#if defined(_WIN32) || defined(__linux__)
    if (!_isStdout && _isConfig) {
        log4cpp::Category::shutdown();
    }
#endif
#endif
}

unsigned long NlsLog::pthreadSelfId() {
#if defined (_WIN32)
    return GetCurrentThreadId();
#elif defined(__APPLE__)
    return pthread_self()->__sig;
#else
    return pthread_self();
#endif
}

#if (!defined(__ANDRIOD__)) && (!defined(__APPLE__))
#if defined(_WIN32) || defined(__linux__)
static log4cpp::Category& getCategory() {
    log4cpp::Category& _category = log4cpp::Category::getRoot().getInstance("alibabaNlsLog");
    return _category;
}
#endif
#endif

void NlsLog::logConfig(const char* name, int level, size_t fileSize) {

    if (name) {
        cout << "Begin LogConfig: " << _isConfig << " , " << name << " , " << level << " , " << fileSize << endl;
    } else {
        cout << "Begin LogConfig: " << _isConfig << " , " << level << " , " << fileSize << endl;
    }
    
#if defined(_WIN32)
    WaitForSingleObject(_mtxLog, INFINITE);
#else
    pthread_mutex_lock(&_mtxLog);
#endif

    if (!_isConfig) {
#if (!defined(__ANDRIOD__)) && (!defined(__APPLE__))
        if (name && (fileSize > 0)) {
            log4cpp::PatternLayout* layout;
            layout = new log4cpp::PatternLayout();
            layout->setConversionPattern("%d: %p %c%x: %m%n");

            string logFileName = name;
            logFileName += ".log";
//            cout << "Nls log name: " << logFileName << " ." << endl;
            log4cpp::RollingFileAppender* rollfileAppender;
            rollfileAppender = new log4cpp::RollingFileAppender(name, logFileName, fileSize * LOG_FILE_BASE_SIZE, LOG_FILES_NUMBER);
            rollfileAppender->setLayout(layout);

            switch(level) {
                case 1:
                    log4cpp::Category::getRoot().setPriority(log4cpp::Priority::ERROR);
                    break;
                case 2:
                    log4cpp::Category::getRoot().setPriority(log4cpp::Priority::WARN);
                    break;
                case 3:
                    log4cpp::Category::getRoot().setPriority(log4cpp::Priority::INFO);
                    break;
                case 4:
                    log4cpp::Category::getRoot().setPriority(log4cpp::Priority::DEBUG);
                    break;
                default:
                    log4cpp::Category::getRoot().setPriority(log4cpp::Priority::ERROR);
                    break;
            }

            getCategory().addAppender(rollfileAppender);
            _isStdout = false;
        } else {
            _isStdout = true;
        }
#endif
        _logLevel = level;
        _isConfig = true;
    }

#if defined(_WIN32)
    ReleaseMutex(_mtxLog);
#else
    pthread_mutex_unlock(&_mtxLog);
#endif
    cout << "LogConfig Done." << endl;
    return ;
}

void NlsLog::logVerbose(const char* function, int line, const char *format, ...) {
    if (!format || !_isConfig) {
        return ;
    }

    char message[LOG_BUFFER_SIZE] = {0};
    LOG_FORMAT_STRING(function, line, format, message);

#if defined (__ANDRIOD__)
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, message);
#elif defined(_WIN32) || defined(__linux__)
    if (!_isStdout) {
        getCategory().emerg(message);
    } else {
        LOG_PRINT_COMMON("VERBOSE", message);
    }
#else
    LOG_PRINT_COMMON("VERBOSE", message);
#endif
}

void NlsLog::logDebug(const char* function, int line, const char *format, ...) {

    if (!format || !_isConfig) {
        return ;
    }

    char message[LOG_BUFFER_SIZE] = {0};
    LOG_FORMAT_STRING(function, line, format, message);

    if (_logLevel >= 4) {
#if defined (__ANDRIOD__)
        __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, message);
#elif defined(_WIN32) || defined(__linux__)
        if (!_isStdout) {
            getCategory().debug(message);
        } else {
            LOG_PRINT_COMMON("DEBUG", message);
        }
#else
        LOG_PRINT_COMMON("DEBUG", message);
#endif
    }
}

void NlsLog::logInfo(const char* function, int line, const char * format, ...) {
    if (!format || !_isConfig) {
        return ;
    }

    char message[LOG_BUFFER_SIZE] = {0};
    LOG_FORMAT_STRING(function, line, format, message);

    if (_logLevel >= 3) {
#if defined (__ANDRIOD__)
        __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, message);
#elif defined(_WIN32) || defined(__linux__)
        if (!_isStdout) {
            getCategory().info(message);
        } else {
            LOG_PRINT_COMMON("INFO", message);
        }
#else
        LOG_PRINT_COMMON("INFO", message);
#endif
    }
}

void NlsLog::logWarn(const char* function, int line, const char * format, ...) {
    if (!format || !_isConfig) {
        return ;
    }

    char message[LOG_BUFFER_SIZE] = {0};
    LOG_FORMAT_STRING(function, line, format, message);

    if (NlsLog::_logLevel >= 2) {
#if defined (__ANDRIOD__)
        __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, message);
#elif defined(_WIN32) || defined(__linux__)
        if (!_isStdout) {
            getCategory().warn(message);
        } else {
            LOG_PRINT_COMMON("WARN", message);
        }
#else
        LOG_PRINT_COMMON("WARN", message);
#endif
    }
}

void NlsLog::logError(const char* function, int line, const char * format, ...) {
    if (!format || !_isConfig) {
        return ;
    }

    char message[LOG_BUFFER_SIZE] = {0};
    LOG_FORMAT_STRING(function, line, format, message);

    if (NlsLog::_logLevel >= 1) {
#if defined (__ANDRIOD__)
        __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, message);
#elif defined(_WIN32) || defined(__linux__)
        if (!_isStdout) {
            getCategory().error(message);
        } else {
            LOG_PRINT_COMMON("ERROR", message);
        }
#else
        LOG_PRINT_COMMON("ERROR", message);
#endif
    }
}

//FATAL
void NlsLog::logException(const char* function, int line, const char *format, ...) {
    if (!format || !_isConfig) {
        return ;
    }

    char message[LOG_BUFFER_SIZE] = {0};
    LOG_FORMAT_STRING(function, line, format, message);

#if defined (__ANDRIOD__)
            __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, message);
#elif defined(_WIN32) || defined(__linux__)
    if (!_isStdout) {
        getCategory().fatal(message);
    } else {
        LOG_PRINT_COMMON("EXCEPTION", message);
    }
#else
    LOG_PRINT_COMMON("EXCEPTION", message);
#endif
}

}

}
