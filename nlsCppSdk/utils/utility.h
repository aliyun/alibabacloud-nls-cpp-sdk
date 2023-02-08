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

#ifndef NLS_SDK_UTILITY_H
#define NLS_SDK_UTILITY_H

namespace AlibabaNls {
namespace utility {

#ifdef _MSC_VER
#define _ssnprintf _snprintf
#else
#define _ssnprintf snprintf
#endif


#define INPUT_PARAM_STRING_CHECK(x) if (x == NULL) {return -(InvalidInputParam);};
#define INPUT_REQUEST_CHECK(x) \
  do { \
    if (x == NULL) { \
      LOG_ERROR("Input request is empty."); \
      return -(RequestEmpty); \
    } \
  } while (0)
#define REQUEST_CHECK(x, y) \
  do { \
    if (x == NULL) { \
      LOG_ERROR("The request of this node(%p) is nullptr.", y); \
      return -(RequestEmpty); \
    } \
  } while (0)
#define EXIT_CANCEL_CHECK(x, y) \
  do { \
    if (x == ExitCancel) { \
      LOG_WARN("Node(%p) has been canceled.", y); \
      return -(InvalidExitStatus); \
    } \
  } while (0)
#define EVENT_CLIENT_CHECK(x) \
  do { \
    if (x == NULL) { \
      LOG_ERROR("NlsEventNetWork has destroyed, please invoke startWorkThread() first."); \
      return -(EventClientEmpty); \
    } \
  } while (0)
#ifdef _MSC_VER
#define SET_EVENT(a, b) \
  do { \
    a = false; \
    SetEvent(b); \
  } while (0)
#else
#define SEND_COND_SIGNAL(a, b, c) \
  do { \
    c = false; \
    pthread_mutex_lock(&a); \
    pthread_cond_signal(&b); \
    pthread_mutex_unlock(&a); \
  } while (0)
#endif

int getLastErrorCode();

}  // namespace utility
}  // namespace AlibabaNls

#endif // NLS_SDK_UTILITY_H
