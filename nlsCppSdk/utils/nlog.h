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

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace AlibabaNls {
namespace utility {

class NlsLog {

public:
  static NlsLog* _logInstance;
  static NlsLog* getInstance();
  static void destroyLogInstance();
  void logConfig(const char* name, int level, size_t fileSize, size_t fileNum);

  void logVerbose(const char* function, int line, const char * format, ...);
  void logDebug(const char* function, int line, const char * format, ...);
  void logInfo(const char* function, int line, const char * format, ...);
  void logWarn(const char* function, int line, const char * format, ...);
  void logError(const char* function, int line, const char * format, ...);
  void logException(const char* function, int line, const char * format, ...);

private:
  NlsLog();
  ~NlsLog();

  unsigned long pthreadSelfId();

#if defined(_MSC_VER)
  static HANDLE _mtxLog;
#else
  static pthread_mutex_t _mtxLog;
#endif

  int _logLevel;
  bool _isStdout;
  bool _isConfig;
};

}  // namespace utility

#define LOG_VERBOSE(...)   do { \
  if (utility::NlsLog::_logInstance) { \
    utility::NlsLog::_logInstance->logVerbose(__FUNCTION__, __LINE__, __VA_ARGS__); \
  } } while(0);

#define LOG_DEBUG(...)     do { \
  if (utility::NlsLog::_logInstance) { \
    utility::NlsLog::_logInstance->logDebug(__FUNCTION__, __LINE__, __VA_ARGS__); \
  } } while(0);

#define LOG_INFO(...)      do { \
  if (utility::NlsLog::_logInstance) { \
    utility::NlsLog::_logInstance->logInfo(__FUNCTION__, __LINE__, __VA_ARGS__); \
  } } while(0);

#define LOG_WARN(...)      do { \
  if (utility::NlsLog::_logInstance) { \
    utility::NlsLog::_logInstance->logWarn(__FUNCTION__, __LINE__, __VA_ARGS__); \
  } } while(0);

#define LOG_ERROR(...)     do { \
  if (utility::NlsLog::_logInstance) { \
    utility::NlsLog::_logInstance->logError(__FUNCTION__, __LINE__, __VA_ARGS__); \
  } } while(0);

#define LOG_EXCEPTION(...) do { \
  if (utility::NlsLog::_logInstance) { \
    utility::NlsLog::_logInstance->logException(__FUNCTION__, __LINE__, __VA_ARGS__); \
  } } while(0);

}  // namespace AlibabaNls

#endif //NLS_SDK_LOG_H
