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

#include <pthread.h>

namespace AlibabaNls {
namespace utility {

class NlsLog {

public:
  static NlsLog* _logInstance;
  static void destroyLogInstance();
  void logConfig(const char* name, int level, size_t fileSize);

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

  static pthread_mutex_t _mtxLog;

  int _logLevel;
  bool _isStdout;
  bool _isConfig;
};

}  // namespace utility

#define LOG_VERBOSE(...)   utility::NlsLog::_logInstance->logVerbose(__FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_DEBUG(...)     utility::NlsLog::_logInstance->logDebug(__FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_INFO(...)      utility::NlsLog::_logInstance->logInfo(__FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_WARN(...)      utility::NlsLog::_logInstance->logWarn(__FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_ERROR(...)     utility::NlsLog::_logInstance->logError(__FUNCTION__, __LINE__, __VA_ARGS__);
#define LOG_EXCEPTION(...) utility::NlsLog::_logInstance->logException(__FUNCTION__, __LINE__, __VA_ARGS__);

}  // namespace AlibabaNls

#endif //NLS_SDK_LOG_H
