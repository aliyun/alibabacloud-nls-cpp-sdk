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

#define INPUT_PARAM_STRING_CHECK(x) \
  if (x == NULL) {                  \
    return -(InvalidInputParam);    \
  };
#define INPUT_PARAM_JSON_STRING_CHECK(x)                  \
  do {                                                    \
    if (x != NULL) {                                      \
      Json::Reader json_check_reader;                     \
      Json::Value json_check_root(Json::objectValue);     \
      if (!json_check_reader.parse(x, json_check_root)) { \
        return -(JsonParseFailed);                        \
      }                                                   \
    }                                                     \
  } while (0)
#define INPUT_REQUEST_CHECK(x)              \
  do {                                      \
    if (x == NULL) {                        \
      LOG_ERROR("Input request is empty."); \
      return -(RequestEmpty);               \
    }                                       \
  } while (0)
#define INPUT_REQUEST_PARAM_CHECK(x)              \
  do {                                            \
    if (x == NULL) {                              \
      LOG_ERROR("Input request param is empty."); \
      return -(InvalidRequest);                   \
    }                                             \
  } while (0)
#define REQUEST_CHECK(x, y)                                     \
  do {                                                          \
    if (x == NULL) {                                            \
      LOG_ERROR("The request of this node(%p) is nullptr.", y); \
      return -(RequestEmpty);                                   \
    }                                                           \
  } while (0)
#define EXIT_CANCEL_CHECK(x, y)                   \
  do {                                            \
    if (x == ExitCancel) {                        \
      LOG_WARN("Node(%p) has been canceled.", y); \
      return -(InvalidExitStatus);                \
    }                                             \
  } while (0)
#define EVENT_CLIENT_CHECK(x)                                               \
  do {                                                                      \
    if (x == NULL) {                                                        \
      LOG_ERROR(                                                            \
          "NlsEventNetWork has destroyed, please invoke startWorkThread() " \
          "first.");                                                        \
      return -(EventClientEmpty);                                           \
    }                                                                       \
  } while (0)
#ifdef _MSC_VER
#define SET_EVENT(a, b) \
  do {                  \
    a = false;          \
    SetEvent(b);        \
  } while (0)
#else
#define SEND_COND_SIGNAL(a, b, c) \
  do {                            \
    pthread_mutex_lock(&a);       \
    c = false;                    \
    pthread_cond_signal(&b);      \
    pthread_mutex_unlock(&a);     \
  } while (0)
#endif

#ifdef _MSC_VER
#define MUTEX_LOCK(a)                 \
  do {                                \
    WaitForSingleObject(a, INFINITE); \
  } while (0)
#else
#ifdef ENABLE_NLS_DEBUG_3
#define MUTEX_LOCK(a)                    \
  do {                                   \
    LOG_DEBUG("lock with %p.", &a);      \
    pthread_mutex_lock(&a);              \
    LOG_DEBUG("lock with %p done.", &a); \
  } while (0)
#else
#define MUTEX_LOCK(a)       \
  do {                      \
    pthread_mutex_lock(&a); \
  } while (0)
#endif
#endif

#ifdef _MSC_VER
#define MUTEX_LOCK_WITH_TAG(a, h)     \
  do {                                \
    WaitForSingleObject(a, INFINITE); \
  } while (0)
#else
#ifdef ENABLE_NLS_DEBUG_3
#define MUTEX_LOCK_WITH_TAG(a, h)                       \
  do {                                                  \
    LOG_DEBUG("Handler(%p) lock with %p.", h, &a);      \
    pthread_mutex_lock(&a);                             \
    LOG_DEBUG("Handler(%p) lock with %p done.", h, &a); \
  } while (0)
#else
#define MUTEX_LOCK_WITH_TAG(a, h) \
  do {                            \
    pthread_mutex_lock(&a);       \
  } while (0)
#endif
#endif

#ifdef _MSC_VER
#define MUTEX_TRY_LOCK(a, ms, r)      \
  do {                                \
    WaitForSingleObject(a, INFINITE); \
    r = true;                         \
    break;                            \
  } while (0)
#else
#ifdef ENABLE_NLS_DEBUG_3
#define MUTEX_TRY_LOCK(a, ms, r)                  \
  do {                                            \
    LOG_DEBUG("trylock with %p.", &a);            \
    int count = ms / 5;                           \
    if (count == 0) count = 1;                    \
    while (1) {                                   \
      if (count-- > 0) {                          \
        int lock_ret = pthread_mutex_trylock(&a); \
        if (lock_ret == 0) {                      \
          r = true;                               \
          break;                                  \
        } else {                                  \
          usleep(5 * 1000);                       \
        }                                         \
      } else {                                    \
        r = false;                                \
        break;                                    \
      }                                           \
    }                                             \
    LOG_DEBUG("trylock with %p done.", &a);       \
  } while (0)
#else
#define MUTEX_TRY_LOCK(a, ms, r)                  \
  do {                                            \
    int count = ms / 5;                           \
    if (count == 0) count = 1;                    \
    while (1) {                                   \
      if (count-- > 0) {                          \
        int lock_ret = pthread_mutex_trylock(&a); \
        if (lock_ret == 0) {                      \
          r = true;                               \
          break;                                  \
        } else {                                  \
          usleep(5 * 1000);                       \
        }                                         \
      } else {                                    \
        r = false;                                \
        break;                                    \
      }                                           \
    }                                             \
  } while (0)
#endif
#endif

#ifdef _MSC_VER
#define MUTEX_TRY_LOCK_WITH_TAG(a, ms, r, h) \
  do {                                       \
    WaitForSingleObject(a, INFINITE);        \
    r = true;                                \
    break;                                   \
  } while (0)
#else
#ifdef ENABLE_NLS_DEBUG_3
#define MUTEX_TRY_LOCK_WITH_TAG(a, ms, r, h)            \
  do {                                                  \
    LOG_DEBUG("Handler(%p) trylock with %p.", h, &a);   \
    int count = ms / 5;                                 \
    if (count == 0) count = 1;                          \
    while (1) {                                         \
      if (count-- > 0) {                                \
        int lock_ret = pthread_mutex_trylock(&a);       \
        if (lock_ret == 0) {                            \
          r = true;                                     \
          break;                                        \
        } else {                                        \
          usleep(5 * 1000);                             \
        }                                               \
      } else {                                          \
        r = false;                                      \
        break;                                          \
      }                                                 \
    }                                                   \
    LOG_DEBUG("Handler(%p) trylock with %p %s.", h, &a, \
              r ? "success" : "failed");                \
  } while (0)
#else
#define MUTEX_TRY_LOCK_WITH_TAG(a, ms, r, h)      \
  do {                                            \
    int count = ms / 5;                           \
    if (count == 0) count = 1;                    \
    while (1) {                                   \
      if (count-- > 0) {                          \
        int lock_ret = pthread_mutex_trylock(&a); \
        if (lock_ret == 0) {                      \
          r = true;                               \
          break;                                  \
        } else {                                  \
          usleep(5 * 1000);                       \
        }                                         \
      } else {                                    \
        r = false;                                \
        break;                                    \
      }                                           \
    }                                             \
  } while (0)
#endif
#endif

#ifdef _MSC_VER
#define MUTEX_UNLOCK(a) \
  do {                  \
    ReleaseMutex(a);    \
  } while (0)
#else
#ifdef ENABLE_NLS_DEBUG_3
#define MUTEX_UNLOCK(a)                    \
  do {                                     \
    LOG_DEBUG("unlock with %p.", &a);      \
    pthread_mutex_unlock(&a);              \
    LOG_DEBUG("unlock with %p done.", &a); \
  } while (0)
#else
#define MUTEX_UNLOCK(a)       \
  do {                        \
    pthread_mutex_unlock(&a); \
  } while (0)
#endif
#endif

#ifdef _MSC_VER
#define MUTEX_UNLOCK_WITH_TAG(a, h) \
  do {                              \
    ReleaseMutex(a);                \
  } while (0)
#else
#ifdef ENABLE_NLS_DEBUG_3
#define MUTEX_UNLOCK_WITH_TAG(a, h)                       \
  do {                                                    \
    LOG_DEBUG("Handler(%p) unlock with %p.", h, &a);      \
    pthread_mutex_unlock(&a);                             \
    LOG_DEBUG("Handler(%p) unlock with %p done.", h, &a); \
  } while (0)
#else
#define MUTEX_UNLOCK_WITH_TAG(a, h) \
  do {                              \
    pthread_mutex_unlock(&a);       \
  } while (0)
#endif
#endif

int getLastErrorCode();

}  // namespace utility
}  // namespace AlibabaNls

#endif  // NLS_SDK_UTILITY_H
