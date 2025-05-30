/*
 * Copyright 2021 Alibaba Group Holding Limited
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
#include <stdio.h>

#include <string.h>
#include <ctime>
#include <iostream>

#if defined(__ANDRIOD__)
#include <android/log.h>
#elif defined(_MSC_VER) || defined(__linux__)
#include "log4cpp/Appender.hh"
#include "log4cpp/Category.hh"
#include "log4cpp/FileAppender.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/Priority.hh"
#include "log4cpp/RollingFileAppender.hh"
#endif

#include "event.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {
namespace utility {

using std::cout;
using std::endl;
using std::string;

#define LOG_BUFFER_SIZE 2048
#define LOG_BUFFER_PLUS_SIZE 2560
#define LOG_FILES_NUMBER 20
#define LOG_FILE_BASE_SIZE 1024 * 1024
#define LOG_TAG "AliSpeechLib"

#define LOG_FORMAT_STRING(a, l, f, b)                                         \
  do {                                                                        \
    char tmpBuffer[LOG_BUFFER_SIZE] = {0};                                    \
    va_list arg;                                                              \
    va_start(arg, f);                                                         \
    vsnprintf(tmpBuffer, LOG_BUFFER_SIZE - 1, f, arg);                        \
    va_end(arg);                                                              \
    _ssnprintf(b, LOG_BUFFER_SIZE, "[ID:0x%lx][%s:%d]%s", pthreadSelfId(), a, \
               l, tmpBuffer);                                                 \
  } while (0)

#define LOG_WASH(in, str)                                        \
  do {                                                           \
    std::string delim = "%";                                     \
    std::vector<std::string> str_vector;                         \
    std::string tmp_str(in);                                     \
    int pos1 = 0;                                                \
    int pos2 = tmp_str.find(delim);                              \
    int len = delim.length();                                    \
    while (pos2 != string::npos) {                               \
      str_vector.push_back(tmp_str.substr(pos1, pos2 - pos1));   \
      pos1 = pos2 + len;                                         \
      pos2 = tmp_str.find(delim, pos1);                          \
    }                                                            \
    if (pos1 != tmp_str.length()) {                              \
      str_vector.push_back(tmp_str.substr(pos1));                \
    }                                                            \
    std::vector<std::string>::iterator iter;                     \
    for (iter = str_vector.begin(); iter != str_vector.end();) { \
      str += *iter;                                              \
      if (++iter != str_vector.end()) {                          \
        str += "%%";                                             \
      }                                                          \
    }                                                            \
  } while (0)

#define LOG_PRINT_COMMON(level, message)                                       \
  do {                                                                         \
    time_t tt = time(NULL);                                                    \
    struct tm* ptm = localtime(&tt);                                           \
    fprintf(stdout, "%4d-%02d-%02d %02d:%02d:%02d %s(%s): %s\n",               \
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday, \
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec, LOG_TAG,    \
            level, message);                                                   \
  } while (0)

#define LOG_PRINT_CALLBACK(level, message, callback)                         \
  do {                                                                       \
    if (callback) {                                                          \
      time_t ttc = time(NULL);                                               \
      struct tm* ptmc = localtime(&ttc);                                     \
      char timestamp[128] = {0};                                             \
      char log_mesg[LOG_BUFFER_PLUS_SIZE] = {0};                             \
      snprintf(timestamp, 128, "%4d-%02d-%02d %02d:%02d:%02d",               \
               (int)ptmc->tm_year + 1900, (int)ptmc->tm_mon + 1,             \
               (int)ptmc->tm_mday, (int)ptmc->tm_hour, (int)ptmc->tm_min,    \
               (int)ptmc->tm_sec);                                           \
      snprintf(log_mesg, LOG_BUFFER_PLUS_SIZE, "[%s] %s", LOG_TAG, message); \
      callback(timestamp, level, log_mesg);                                  \
    }                                                                        \
  } while (0)

#if defined(_MSC_VER)
HANDLE NlsLog::_mtxLog = CreateMutex(NULL, FALSE, NULL);
#else
pthread_mutex_t NlsLog::_mtxLog = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsLog* NlsLog::_logInstance = NULL;

NlsLog::NlsLog()
    : _callback(NULL),
      _logLevel(1),
      _isStdout(true),
      _isConfig(false),
      _logFileName(""),
      _logLibeventFileName("") {}

NlsLog::~NlsLog() {
#if (!defined(__ANDRIOD__)) && (!defined(__APPLE__))
#if defined(_MSC_VER) || defined(__linux__)
  if (!_isStdout && _isConfig) {
    log4cpp::Category::shutdown();
  }
#endif
#endif

  _isStdout = true;
  _isConfig = false;
  _callback = NULL;
}

unsigned long NlsLog::pthreadSelfId() {
#if defined(_MSC_VER)
  return GetCurrentThreadId();
#elif defined(__APPLE__)
  return pthread_self()->__sig;
#else
  return pthread_self();
#endif
}

NlsLog* NlsLog::getInstance() {
  if (_logInstance == NULL) {
    _logInstance = new NlsLog();
  }
  return _logInstance;
}

void NlsLog::destroyLogInstance() {
  if (_logInstance) {
    delete _logInstance;
    _logInstance = NULL;
  }
}

#if (!defined(__ANDRIOD__)) && (!defined(__APPLE__))
#if defined(_MSC_VER) || defined(__linux__)
static log4cpp::Category& getCategory() {
  log4cpp::Category& _category =
      log4cpp::Category::getRoot().getInstance("alibabaNlsLog");
  return _category;
}
#endif
#endif

void NlsLog::logConfig(const char* name, int level, size_t fileSize,
                       size_t fileNum, LogCallbackMethod callback) {
  if (name) {
    cout << "Begin LogConfig: " << _isConfig << " , " << name << " , " << level
         << " , " << fileSize << endl;
  } else {
    cout << "Begin LogConfig: " << _isConfig << " , " << level << " , "
         << fileSize << endl;
  }

  if (fileNum < 1) {
    fileNum = LOG_FILES_NUMBER;
  }

  _callback = callback;

  MUTEX_LOCK(_mtxLog);

  if (!_isConfig) {
#if (!defined(__ANDRIOD__)) && (!defined(__APPLE__))
    if (name && (fileSize > 0)) {
      log4cpp::PatternLayout* layout;
      layout = new log4cpp::PatternLayout();
      layout->setConversionPattern("%d: %p %c%x: %m%n");

      _logFileName = name;
      _logLibeventFileName = name;
      _logFileName += ".log";
      _logLibeventFileName += "_libevent.log";

      log4cpp::RollingFileAppender* rollfileAppender;
      rollfileAppender = new log4cpp::RollingFileAppender(
          name, _logFileName, fileSize * LOG_FILE_BASE_SIZE, fileNum);
      rollfileAppender->setLayout(layout);

      switch (level) {
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

  MUTEX_UNLOCK(_mtxLog);
  cout << "LogConfig Done." << endl;
  return;
}

void NlsLog::logVerbose(const char* function, int line, const char* format,
                        ...) {
  if (!format || !_isConfig) {
    return;
  }

  char message[LOG_BUFFER_PLUS_SIZE] = {0};
  std::string str_in = "";
  LOG_FORMAT_STRING(function, line, format, message);
  LOG_WASH(message, str_in);

#if defined(__ANDRIOD__)
  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s", str_in.c_str());
#elif defined(_MSC_VER) || defined(__linux__)
  if (!_isStdout) {
    getCategory().debug(str_in.c_str());
  } else {
    LOG_PRINT_COMMON("VERBOSE", str_in.c_str());
  }
#else
  LOG_PRINT_COMMON("VERBOSE", str_in.c_str());
#endif
  LOG_PRINT_CALLBACK(AlibabaNls::LogDebug, str_in.c_str(), _callback);
}

void NlsLog::logDebug(const char* function, int line, const char* format, ...) {
  if (!format || !_isConfig) {
    return;
  }

  if (_logLevel >= 4) {
    char message[LOG_BUFFER_PLUS_SIZE] = {0};
    std::string str_in = "";
    LOG_FORMAT_STRING(function, line, format, message);
    LOG_WASH(message, str_in);

#if defined(__ANDRIOD__)
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s", str_in.c_str());
#elif defined(_MSC_VER) || defined(__linux__)
    if (!_isStdout) {
      getCategory().debug(str_in.c_str());
    } else {
      LOG_PRINT_COMMON("DEBUG", str_in.c_str());
    }
#else
    LOG_PRINT_COMMON("DEBUG", str_in.c_str());
#endif
    LOG_PRINT_CALLBACK(AlibabaNls::LogDebug, str_in.c_str(), _callback);
  }
}

void NlsLog::logInfo(const char* function, int line, const char* format, ...) {
  if (!format || !_isConfig) {
    return;
  }

  if (_logLevel >= 3) {
    char message[LOG_BUFFER_PLUS_SIZE] = {0};
    std::string str_in = "";
    LOG_FORMAT_STRING(function, line, format, message);
    LOG_WASH(message, str_in);

#if defined(__ANDRIOD__)
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s", str_in.c_str());
#elif defined(_MSC_VER) || defined(__linux__)
    if (!_isStdout) {
      getCategory().info(str_in.c_str());
    } else {
      LOG_PRINT_COMMON("INFO", str_in.c_str());
    }
#else
    LOG_PRINT_COMMON("INFO", str_in.c_str());
#endif
    LOG_PRINT_CALLBACK(AlibabaNls::LogInfo, str_in.c_str(), _callback);
  }
}

void NlsLog::logWarn(const char* function, int line, const char* format, ...) {
  if (!format || !_isConfig) {
    return;
  }

  if (_logLevel >= 2) {
    char message[LOG_BUFFER_PLUS_SIZE] = {0};
    std::string str_in = "";
    LOG_FORMAT_STRING(function, line, format, message);
    LOG_WASH(message, str_in);

#if defined(__ANDRIOD__)
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s", str_in.c_str());
#elif defined(_MSC_VER) || defined(__linux__)
    if (!_isStdout) {
      getCategory().warn(str_in.c_str());
    } else {
      LOG_PRINT_COMMON("WARN", str_in.c_str());
    }
#else
    LOG_PRINT_COMMON("WARN", str_in.c_str());
#endif
    LOG_PRINT_CALLBACK(AlibabaNls::LogWarning, str_in.c_str(), _callback);
  }
}

void NlsLog::logError(const char* function, int line, const char* format, ...) {
  if (!format || !_isConfig) {
    return;
  }

  if (_logLevel >= 1) {
    char message[LOG_BUFFER_PLUS_SIZE] = {0};
    std::string str_in = "";
    LOG_FORMAT_STRING(function, line, format, message);
    LOG_WASH(message, str_in);

#if defined(__ANDRIOD__)
    __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s", str_in.c_str());
#elif defined(_MSC_VER) || defined(__linux__)
    if (!_isStdout) {
      getCategory().error(str_in.c_str());
    } else {
      LOG_PRINT_COMMON("ERROR", str_in.c_str());
    }
#else
    LOG_PRINT_COMMON("ERROR", str_in.c_str());
#endif
    LOG_PRINT_CALLBACK(AlibabaNls::LogError, str_in.c_str(), _callback);
  }
}

// FATAL
void NlsLog::logException(const char* function, int line, const char* format,
                          ...) {
  if (!format || !_isConfig) {
    return;
  }

  char message[LOG_BUFFER_PLUS_SIZE] = {0};
  std::string str_in = "";
  LOG_FORMAT_STRING(function, line, format, message);
  LOG_WASH(message, str_in);

#if defined(__ANDRIOD__)
  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s", str_in.c_str());
#elif defined(_MSC_VER) || defined(__linux__)
  if (!_isStdout) {
    getCategory().fatal(str_in.c_str());
  } else {
    LOG_PRINT_COMMON("EXCEPTION", str_in.c_str());
  }
#else
  LOG_PRINT_COMMON("EXCEPTION", str_in.c_str());
#endif
  LOG_PRINT_CALLBACK(AlibabaNls::LogError, str_in.c_str(), _callback);
}

void NlsLog::dumpEvents(void* evbase) {
#ifdef ENABLE_NLS_DEBUG
  if (!_logLibeventFileName.empty()) {
    FILE* f = fopen(_logLibeventFileName.c_str(), "a+");
    if (f) {
      char log_mesg[256] = {0};
      snprintf(log_mesg, 256, "Dump events about evbase %p:\n", evbase);
      fwrite(log_mesg, strnlen(log_mesg, 256), 1, f);
      struct event_base* base = (struct event_base*)evbase;
      event_base_dump_events(base, f);
      fclose(f);
    }
  }
#endif
}

}  // namespace utility
}  // namespace AlibabaNls
