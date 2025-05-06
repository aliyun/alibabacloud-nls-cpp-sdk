/*
 * Copyright 2025 Alibaba Group Holding Limited
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

#ifdef _MSC_VER
#include <process.h>
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif
#include "connectedPool.h"
#include "flowingSynthesizerRequest.h"
#include "nlog.h"
#include "nlsEventNetWork.h"
#include "nlsRequestParamInfo.h"
#include "speechRecognizerRequest.h"
#include "speechSynthesizerRequest.h"
#include "speechTranscriberRequest.h"
#include "text_utils.h"
#include "utility.h"

namespace AlibabaNls {

ConnectedPool::ConnectedPool(unsigned int maxNumber, unsigned int timeoutMs,
                             unsigned int requestedTimeoutMs)
    : _maxPreconnectedNumber(maxNumber),
      _preconnectedTimeoutMs(timeoutMs),
      _prerequestedTimeoutMs(requestedTimeoutMs),
      _poolWorkBase(NULL),
      _connectPoolEvent(NULL),
      _connectPoolTimerFlag(false),
      _nodeReleaseEvent(NULL) {
  _poolWorkBase = event_base_new();
  if (NULL == _poolWorkBase) {
    LOG_ERROR("ConnectedPool(%p) invoke event_base_new failed.", this);
    exit(1);
  }
  int features = event_base_get_features(_poolWorkBase);
  LOG_INFO("ConnectedPool(%p) create evbase(%p), get features %d", this,
           _poolWorkBase, features);

  _fssRequests.type = TypeStreamInputTts;
  _srRequests.type = TypeAsr;
  _stRequests.type = TypeRealTime;
  _syRequests.type = TypeTts;

  if (NULL == _connectPoolEvent) {
    _connectPoolEvent = evtimer_new(
        _poolWorkBase, ConnectedPool::connectPoolEventCallback, this);
    if (NULL == _connectPoolEvent) {
      LOG_ERROR("ConnectedPool(%p) new event(connectPoolEventCallback) failed.",
                this);
      exit(1);
    }
  }

  if (_nodeReleaseEvent == NULL) {
    _nodeReleaseEvent =
        event_new(_poolWorkBase, -1, EV_READ,
                  ConnectedPool::nodeReleaseEventCallback, this);
    if (NULL == _nodeReleaseEvent) {
      LOG_ERROR("ConnectedPool(%p) new event(nodeReleaseEventCallback) failed.",
                this);
    }
  }

  _connectPoolTimerTv.tv_sec = 1;
  _connectPoolTimerTv.tv_usec = 500000;
  if (_connectPoolEvent) {
    LOG_DEBUG("ConnectedPool(%p) evtimer_add with %d.%ds.", this,
              _connectPoolTimerTv.tv_sec, _connectPoolTimerTv.tv_usec);
    evtimer_add(_connectPoolEvent, &_connectPoolTimerTv);
    _connectPoolTimerFlag = true;
  }

#if defined(_MSC_VER)
  _lock = CreateMutex(NULL, FALSE, NULL);

  _poolWorkThreadHandle =
      (HANDLE)_beginthreadex(NULL, 0, loopConnectedPoolEventCallback,
                             (LPVOID)this, 0, &_poolWorkThreadId);
  CloseHandle(_poolWorkThreadHandle);
#else
  pthread_mutex_init(&_lock, NULL);

  pthread_create(&_poolWorkThreadId, NULL, loopConnectedPoolEventCallback,
                 (void *)this);
#endif
}

ConnectedPool::~ConnectedPool() {
  LOG_DEBUG("ConnectedPool(%p) destructing ...", this);
  int tryCount = 50;
  while (_poolWorkThreadId != 0 && tryCount-- > 0) {
    usleep(1 * 1000);
  }

  if (_fssRequests.work) {
    deletePreNode(&_fssRequests.prestartedRequests);
    deletePreNode(&_fssRequests.preconnectedRequests);
  }
  if (_srRequests.work) {
    deletePreNode(&_srRequests.prestartedRequests);
    deletePreNode(&_srRequests.preconnectedRequests);
  }
  if (_stRequests.work) {
    deletePreNode(&_stRequests.prestartedRequests);
    deletePreNode(&_stRequests.preconnectedRequests);
  }
  if (_syRequests.work) {
    deletePreNode(&_syRequests.prestartedRequests);
    deletePreNode(&_syRequests.preconnectedRequests);
  }

#if defined(_MSC_VER)
  CloseHandle(_lock);
#else
  pthread_mutex_destroy(&_lock);
#endif

  LOG_DEBUG("ConnectedPool(%p) destructing done", this);
}

#if defined(_MSC_VER)
unsigned __stdcall ConnectedPool::loopConnectedPoolEventCallback(LPVOID arg) {
#else
void *ConnectedPool::loopConnectedPoolEventCallback(void *arg) {
#endif

  ConnectedPool *eventParam = static_cast<ConnectedPool *>(arg);

#if defined(__ANDROID__) || defined(__linux__)
  sigset_t signal_mask;

  if (-1 == sigemptyset(&signal_mask)) {
    LOG_ERROR("sigemptyset failed.");
    exit(1);
  }

  if (-1 == sigaddset(&signal_mask, SIGPIPE)) {
    LOG_ERROR("sigaddset failed.");
    exit(1);
  }

  if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
    LOG_ERROR("pthread_sigmask failed.");
    exit(1);
  }

  prctl(PR_SET_NAME, "connectedPoolEventThread");
#endif

  LOG_DEBUG("ConnectedPool(%p) create loopConnectedPoolEventCallback.", arg);

  if (eventParam->_poolWorkBase) {
    LOG_DEBUG("ConnectedPool(%p) event_base_dispatch ...", arg);
    event_base_dispatch(eventParam->_poolWorkBase);
  }
  if (eventParam->_poolWorkBase) {
    LOG_DEBUG("ConnectedPool(%p) event_base_free ...", arg);
    event_base_free(eventParam->_poolWorkBase);
    eventParam->_poolWorkBase = NULL;
  }

  eventParam->_poolWorkThreadId = 0;

  LOG_DEBUG("ConnectedPool(%p) loopConnectedPoolEventCallback exit.", arg);

#if defined(_MSC_VER)
  return Success;
#else
  return NULL;
#endif
}

void ConnectedPool::connectPoolEventCallback(evutil_socket_t socketFd,
                                             short event, void *arg) {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_a, timewait_end = 0;
  ConnectedPool *pool = static_cast<ConnectedPool *>(arg);
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(pool->_lock, pool);
  timewait_a = utility::TextUtils::GetTimestampMs();
#else
  ConnectedPool *pool = static_cast<ConnectedPool *>(arg);
  MUTEX_LOCK_WITH_TAG(pool->_lock, pool);
#endif

  LOG_DEBUG("Pool(%p) connectPoolEventCallback checking every pre-node ...",
            pool);

  int releaseCount = 0;

  if (event == EV_CLOSED) {
  } else {
    // event == EV_TIMEOUT
    if (pool->_fssRequests.work) {
      releaseCount +=
          pool->timeoutPrestartedNode(&pool->_fssRequests.prestartedRequests);
      releaseCount += pool->timeoutPreconnectedNode(
          &pool->_fssRequests.preconnectedRequests);
    }
    if (pool->_srRequests.work) {
      releaseCount +=
          pool->timeoutPrestartedNode(&pool->_srRequests.prestartedRequests);
      releaseCount += pool->timeoutPreconnectedNode(
          &pool->_srRequests.preconnectedRequests);
    }
    if (pool->_stRequests.work) {
      releaseCount +=
          pool->timeoutPrestartedNode(&pool->_stRequests.prestartedRequests);
      releaseCount += pool->timeoutPreconnectedNode(
          &pool->_stRequests.preconnectedRequests);
    }
    if (pool->_syRequests.work) {
      releaseCount +=
          pool->timeoutPrestartedNode(&pool->_syRequests.prestartedRequests);
      // pool->showEveryNode(&pool->_syRequests.prestartedRequests,
      //                     "syPrestarted after timeout");
      releaseCount += pool->timeoutPreconnectedNode(
          &pool->_syRequests.preconnectedRequests);
      // pool->showEveryNode(&pool->_syRequests.preconnectedRequests,
      //                     "syPreconnected after timeout");
    }
  }

  MUTEX_UNLOCK_WITH_TAG(pool->_lock, pool);

  evtimer_add(pool->_connectPoolEvent, &pool->_connectPoolTimerTv);

  if (releaseCount > 0) {
    event_active(pool->_nodeReleaseEvent, EV_READ, 0);
  }

#ifdef ENABLE_NLS_DEBUG_2
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN(
        "Pool(%p) connectPoolEventCallback done with excessive time:%llums, "
        "lock:%llums, work:%llums",
        pool, timewait_end - timewait_start, timewait_a - timewait_start,
        timewait_end - timewait_a);
  } else {
    LOG_DEBUG("Pool(%p) connectPoolEventCallback done", pool);
  }
#else
  LOG_DEBUG("Pool(%p) connectPoolEventCallback done", pool);
#endif
  return;
}

void ConnectedPool::nodeReleaseEventCallback(evutil_socket_t socketFd,
                                             short event, void *arg) {
  ConnectedPool *pool = static_cast<ConnectedPool *>(arg);

  LOG_DEBUG(
      "Pool(%p) nodeReleaseEventCallback ready to delete or preconnect ...",
      pool);

  if (event == EV_READ) {
    if (pool->_fssRequests.work) {
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_fssRequests.prestartedRequests, "fssPrestarted");
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_fssRequests.preconnectedRequests, "fssPreconnected");
    }
    if (pool->_srRequests.work) {
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_srRequests.prestartedRequests, "srPrestarted");
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_srRequests.preconnectedRequests, "srPreconnected");
    }
    if (pool->_stRequests.work) {
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_stRequests.prestartedRequests, "stPrestarted");
      // pool->showEveryNode(&pool->_stRequests.prestartedRequests,
      //                     "stPrestarted");
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_stRequests.preconnectedRequests, "stPreconnected");
      // pool->showEveryNode(&pool->_stRequests.preconnectedRequests,
      //                     "stPreconnected");
    }
    if (pool->_syRequests.work) {
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_syRequests.prestartedRequests, "syPrestarted");
      pool->deleteOrPreconnectNodeShouldReleased(
          &pool->_syRequests.preconnectedRequests, "syPreconnected");
    }
  }

  LOG_DEBUG("Pool(%p) nodeReleaseEventCallback done", pool);
}

int ConnectedPool::startConnectedPool() {
  // uint64_t timewait_start, timewait_end = 0;
  // timewait_start = utility::TextUtils::GetTimestampMs();
  // MUTEX_LOCK(_lock);
  // timewait_end = utility::TextUtils::GetTimestampMs();
  // if (timewait_end - timewait_start > 50) {
  //   LOG_WARN(
  //       "ConnectedPool(%p) startConnectedPool lock with excessive time
  //       %llums.", this, timewait_end - timewait_start);
  // } else {
  //   LOG_DEBUG("ConnectedPool(%p) startConnectedPool ...", this);
  // }
  // MUTEX_UNLOCK(_lock);
  return Success;
}

int ConnectedPool::stopConnectedPool() {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK(_lock);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN(
        "ConnectedPool(%p) stopConnectedPool lock with excessive time %llums.",
        this, timewait_end - timewait_start);
  } else {
    LOG_DEBUG("ConnectedPool(%p) stopConnectedPool ...", this);
  }
#else
  MUTEX_LOCK(_lock);
  LOG_DEBUG("ConnectedPool(%p) stopConnectedPool ...", this);
#endif

  if (_connectPoolEvent != NULL) {
    if (_connectPoolTimerFlag) {
      evtimer_del(_connectPoolEvent);
      _connectPoolTimerFlag = false;
    } else {
      LOG_DEBUG("ConnectedPool(%p) PoolEvent isnot in working ...", this);
    }
    event_free(_connectPoolEvent);
    _connectPoolEvent = NULL;
  }

  if (_nodeReleaseEvent) {
    event_del(_nodeReleaseEvent);
    event_free(_nodeReleaseEvent);
    _nodeReleaseEvent = NULL;
  }

  LOG_INFO("ConnectedPool(%p) break evbase(%p) ...", this, _poolWorkBase);
  event_base_loopbreak(_poolWorkBase);
  return Success;
}

bool ConnectedPool::popPrestartedNode(INlsRequest *request, NlsType type) {
  if (request == NULL) {
    return false;
  }

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN(
        "ConnectedPool(%p) popPrestartedNode Request(%p) lock with excessive "
        "time %llums.",
        this, request, timewait_end - timewait_start);
  } else {
    LOG_DEBUG("ConnectedPool(%p) popPrestartedNode Request(%p) ...", this,
              request);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
  LOG_DEBUG("ConnectedPool(%p) popPrestartedNode Request(%p) ...", this,
            request);
#endif

  int ret = Success;
  // 0. 当前池子没有待工作的节点, 则做准备
  int prestarted = 0;
  int preconnected = 0;
  ret = getNumberOfThisTypeNodes(type, prestarted, preconnected);
  if (prestarted == 0 || preconnected == 0) {
    // 0.1 填充preconnectedRequests和prestartedRequests
    ret = initThisNodesPool(type);
    LOG_DEBUG(
        "ConnectedPool(%p) popPrestartedNode initThisNodesPool done with "
        "request(%p). prestarted is %d, preconnected is %d.",
        this, request, prestarted, preconnected);
    MUTEX_UNLOCK_WITH_TAG(_lock, request);
    return false;
  } else {
    int count = getNumberOfPrestartedNodes(type);
    LOG_DEBUG(
        "ConnectedPool(%p) popPrestartedNode, request(%p), prestarted is %d, "
        "count of PrestartedNodes is %d.",
        this, request, prestarted, count);
    if (prestarted > 0 && count > 0) {
      // 1. 存在started的工作节点, 获取此节点
      if (popOnePrestartedNode(request, type)) {
        MUTEX_UNLOCK_WITH_TAG(_lock, request);
        return true;
      }
    }
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return false;
}

bool ConnectedPool::popPreconnectedNode(INlsRequest *request, NlsType type) {
  if (request == NULL) {
    return false;
  }

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
#endif

  int ret = Success;
  // 0. 当前池子没有待工作的节点, 则做准备
  int prestarted = 0;
  int preconnected = 0;
  ret = getNumberOfThisTypeNodes(type, prestarted, preconnected);
  if (prestarted == 0 || preconnected == 0) {
    // 0.1 填充preconnectedRequests和prestartedRequests
    ret = initThisNodesPool(type);
    MUTEX_UNLOCK_WITH_TAG(_lock, request);
    return false;
  } else {
    if (preconnected > 0 && getNumberOfPreconnectedNodes(type) > 0) {
      // 2. 存在connected的工作节点, 获取此节点
      if (popOnePreconnectedNode(request, type)) {
        MUTEX_UNLOCK_WITH_TAG(_lock, request);
        return true;
      }
    }
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return false;
}

bool ConnectedPool::pushPreconnectedNode(INlsRequest *request, NlsType type,
                                         bool newNode) {
  if (request == NULL) {
    return false;
  }

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
#endif

  int index = request->getConnectNode()->getPoolIndex();

  LOG_DEBUG(
      "ConnectedPool(%p) input %s request(%p) ndoe(%p) "
      "with "
      "index(%d) and "
      "type[(%d)"
      "0:Asr,1:SpeechTranscriber,2:TTS,3:StreamInputTts] ...",
      this, newNode ? "new" : "old", request, request->getConnectNode(), index,
      type);

  int ret = Success;
  // 0. 当前池子没有待工作的节点, 则做准备
  int prestarted = 0;
  int preconnected = 0;
  ret = getNumberOfThisTypeNodes(type, prestarted, preconnected);
  if (prestarted == 0 || preconnected == 0) {
    // 0.1 填充preconnectedRequests和prestartedRequests
    ret = initThisNodesPool(type);
  }

  evutil_socket_t curSocketFd = request->getConnectNode()->getSocketFd();
  SSLconnect *curSslHandle = request->getConnectNode()->getSslHandle();

  struct ConnectedPoolProcess *poolProcess = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  switch (type) {
    case TypeAsr:
      poolProcess = &_srRequests;
      curPool = &_srRequests.preconnectedRequests;
      break;
    case TypeRealTime:
      poolProcess = &_stRequests;
      curPool = &_stRequests.preconnectedRequests;
      break;
    case TypeTts:
      poolProcess = &_syRequests;
      curPool = &_syRequests.preconnectedRequests;
      break;
    case TypeStreamInputTts:
      poolProcess = &_fssRequests;
      curPool = &_fssRequests.preconnectedRequests;
      break;
    default:
      break;
  }

  if (curPool) {
    if (!newNode) {
      /* 查找是否已经存在, 则更新部最近一次操作时间戳.
       * 且push操作表示此SSL已经用完 */
      if (curPool->size() >= index) {
        struct ConnectedNodeProcess &node = (*curPool)[index];
        if (node.status == PreNodeConnected) {
#ifdef ENABLE_NLS_DEBUG_2
          LOG_DEBUG(
              "ConnectedPool(%p) pushPreconnectedNode request(%p) compare to "
              "request(%p) ...",
              this, request, node.request);
#endif
          evutil_socket_t itSocketFd =
              node.request->getConnectNode()->getSocketFd();
          SSLconnect *itSslHandle =
              node.request->getConnectNode()->getSslHandle();

          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            uint64_t oldTimestamp = node.workableTimestamp;
            node.workableTimestamp = request->getConnectNode()
                                         ->getNodeProcess()
                                         ->last_op_timestamp_ms;
            node.canPick = false;
            /* curRequest在finishPushPreNode时再置NULL */
            // node.curRequest = NULL;
            node.curRequestInvalid = true;

            LOG_INFO(
                "ConnectedPool(%p) pushPreconnectedNode input request(%p) "
                "node(%p) "
                "is existent in index(%d/%d), update last operation "
                "timestamp(from %s to %s). SSL handle is "
                "%p and SocketFd is %d.",
                this, request, request->getConnectNode(), index,
                curPool->size(),
                utility::TextUtils::GetTimeFromMs(oldTimestamp).c_str(),
                utility::TextUtils::GetTimeFromMs(node.workableTimestamp)
                    .c_str(),
                node.sslHandle, node.socketFd);
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
          }
        }
      }
    }

    /* 若不存在, 则存储 */
    for (std::vector<struct ConnectedNodeProcess>::iterator it =
             curPool->begin();
         it != curPool->end(); ++it) {
      if (it->status == PreNodeToBeCreated && !it->shouldRelease &&
          it->request == NULL) {
        index = std::distance(curPool->begin(), it);
        it->type = request->getRequestParam()->_mode;
        it->status = PreNodeConnected;
        it->workableTimestamp =
            request->getConnectNode()->getNodeProcess()->last_op_timestamp_ms;
        uint64_t oldTimestamp = it->workableTimestamp;
        it->startTimestamp = it->workableTimestamp;
        if (request->getRequestParam()->_mode == TypeTts) {
          it->ttsVersion = request->getRequestParam()->getVersion();
        }
        it->sdkName = request->getRequestParam()->getSdkName();
        it->startedResponse.clear();
        it->request = request;
        it->socketFd = curSocketFd;
        it->sslHandle = curSslHandle;
        it->canPick = false;
        it->curRequest = NULL;
        it->shouldRelease = false;
        it->shouldPreconnect = false;
        it->curRequestInvalid = false;
        it->isAbnormal = false;
        request->getConnectNode()->setPoolIndex(index);
        poolProcess->work = true;

        uint64_t tokenExpirationTimestamp =
            request->getRequestParam()->_tokenExpirationTime;
        const int redundancyTimeDiffMs = 3600000;  // 1h
        if (tokenExpirationTimestamp > redundancyTimeDiffMs) {
          uint64_t nowTimestamp = utility::TextUtils::GetTimestampMs();
          const uint64_t diffTimeMs = 43200000;  // 12h
          it->tokenExpirationTimestamp =
              (tokenExpirationTimestamp >= nowTimestamp + diffTimeMs)
                  ? nowTimestamp + diffTimeMs
                  : (tokenExpirationTimestamp >=
                             nowTimestamp + redundancyTimeDiffMs
                         ? tokenExpirationTimestamp - redundancyTimeDiffMs
                         : tokenExpirationTimestamp);
        }

        LOG_INFO(
            "ConnectedPool(%p) pushPreconnectedNode input request(%p) node(%p) "
            "done in index(%d/%d), start timestamp(%s), last operation "
            "timestamp(%s), token expiration timestamp(%s). SSL handle "
            "is "
            "%p and SocketFd is %d.",
            this, request, request->getConnectNode(), index, curPool->size(),
            utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
            utility::TextUtils::GetTimeFromMs(it->workableTimestamp).c_str(),
            utility::TextUtils::GetTimeFromMs(it->tokenExpirationTimestamp)
                .c_str(),
            it->sslHandle, it->socketFd);
        MUTEX_UNLOCK_WITH_TAG(_lock, request);
        return true;
      }
    }  // for
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return false;
}

bool ConnectedPool::pushPrestartedNode(INlsRequest *request, NlsType type,
                                       bool newNode) {
  if (request == NULL) {
    return false;
  }

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t a_ms = utility::TextUtils::GetTimestampMs();
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  } else {
    LOG_DEBUG("ConnectedPool(%p) pushPrestartedNode request(%p) ...", this,
              request);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
#endif

  int index = request->getConnectNode()->getPoolIndex();

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t b_ms = utility::TextUtils::GetTimestampMs();
#endif

  LOG_DEBUG(
      "ConnectedPool(%p) input %s request(%p) ndoe(%p) with "
      "index(%d) and "
      "type[(%d)"
      "0:Asr,1:SpeechTranscriber,2:TTS,3:StreamInputTts] ...",
      this, newNode ? "new" : "old", request, request->getConnectNode(), index,
      type);

  int ret = Success;
  // 0. 当前池子没有待工作的节点, 则做准备
  int prestarted = 0;
  int preconnected = 0;
  ret = getNumberOfThisTypeNodes(type, prestarted, preconnected);
  if (prestarted == 0 || preconnected == 0) {
    // 0.1 填充preconnectedRequests和prestartedRequests
    ret = initThisNodesPool(type);
  }

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t c_ms = utility::TextUtils::GetTimestampMs();
#endif

  evutil_socket_t curSocketFd = request->getConnectNode()->getSocketFd();
  SSLconnect *curSslHandle = request->getConnectNode()->getSslHandle();

  struct ConnectedPoolProcess *poolProcess = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  switch (type) {
    case TypeAsr:
      poolProcess = &_srRequests;
      curPool = &_srRequests.prestartedRequests;
      break;
    case TypeRealTime:
      poolProcess = &_stRequests;
      curPool = &_stRequests.prestartedRequests;
      break;
    case TypeTts:
      poolProcess = &_syRequests;
      curPool = &_syRequests.prestartedRequests;
      break;
    case TypeStreamInputTts:
      poolProcess = &_fssRequests;
      curPool = &_fssRequests.prestartedRequests;
      break;
    default:
      break;
  }

  if (curPool) {
    if (!newNode) {
      /* 查找是否已经存在, 则更新部最近一次操作时间戳.
       * 且push操作表示此SSL已经用完 */
      if (curPool->size() >= index) {
        struct ConnectedNodeProcess &node = (*curPool)[index];
        if (node.status == PreNodeStarted) {
#ifdef ENABLE_NLS_DEBUG_2
          LOG_DEBUG(
              "ConnectedPool(%p) pushPrestartedNode request(%p) compare to "
              "request(%p) ...",
              this, request, node.request);
#endif
          evutil_socket_t itSocketFd =
              node.request->getConnectNode()->getSocketFd();
          SSLconnect *itSslHandle =
              node.request->getConnectNode()->getSslHandle();

          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            uint64_t oldTimestamp = node.workableTimestamp;
            node.workableTimestamp = request->getConnectNode()
                                         ->getNodeProcess()
                                         ->last_op_timestamp_ms;
            node.canPick = false;
            /* curRequest在finishPushPreNode时再置NULL */
            // node.curRequest = NULL;
            node.curRequestInvalid = true;

#ifdef ENABLE_NLS_DEBUG_2
            uint64_t d_ms = utility::TextUtils::GetTimestampMs();
            LOG_DEBUG(
                "ConnectedPool(%p) pushPrestartedNode latency request(%p) "
                "lock:%llu init:%llu exist:%llu, index:%d",
                this, request, b_ms - a_ms, c_ms - b_ms, d_ms - c_ms, index);
#endif

            LOG_INFO(
                "ConnectedPool(%p) pushPrestartedNode input request(%p) "
                "node(%p) "
                "is existent in index(%d/%d), update last operation "
                "timestamp(from %s to %s). SSL handle is "
                "%p and SocketFd is %d.",
                this, request, request->getConnectNode(), index,
                curPool->size(),
                utility::TextUtils::GetTimeFromMs(oldTimestamp).c_str(),
                utility::TextUtils::GetTimeFromMs(node.workableTimestamp)
                    .c_str(),
                node.sslHandle, node.socketFd);
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
          }
        }
      }
    }

#ifdef ENABLE_NLS_DEBUG_2
    uint64_t d_ms = utility::TextUtils::GetTimestampMs();
#endif

    /* 若不存在, 则存储 */
    for (std::vector<struct ConnectedNodeProcess>::iterator it =
             curPool->begin();
         it != curPool->end(); ++it) {
      if (it->status == PreNodeToBeCreated && !it->shouldRelease &&
          it->request == NULL) {
        index = std::distance(curPool->begin(), it);
        it->type = request->getRequestParam()->_mode;
        it->status = PreNodeStarted;
        it->workableTimestamp =
            request->getConnectNode()->getNodeProcess()->last_op_timestamp_ms;
        it->startTimestamp = it->workableTimestamp;
        if (request->getRequestParam()->_mode == TypeTts) {
          it->ttsVersion = request->getRequestParam()->getVersion();
        }
        it->sdkName = request->getRequestParam()->getSdkName();
        it->startedResponse.clear();
        it->request = request;
        it->socketFd = curSocketFd;
        it->sslHandle = curSslHandle;
        it->canPick = false;
        it->curRequest = NULL;
        it->shouldRelease = false;
        it->shouldPreconnect = false;
        it->curRequestInvalid = false;
        it->isAbnormal = false;
        request->getConnectNode()->setPoolIndex(index);
        poolProcess->work = true;

        uint64_t tokenExpirationTimestamp =
            request->getRequestParam()->_tokenExpirationTime;
        const int redundancyTimeDiffMs = 3600000;  // 1h
        if (tokenExpirationTimestamp > redundancyTimeDiffMs) {
          uint64_t nowTimestamp = utility::TextUtils::GetTimestampMs();
          const uint64_t diffTimeMs = 43200000;  // 12h
          it->tokenExpirationTimestamp =
              (tokenExpirationTimestamp >= nowTimestamp + diffTimeMs)
                  ? nowTimestamp + diffTimeMs
                  : (tokenExpirationTimestamp >=
                             nowTimestamp + redundancyTimeDiffMs
                         ? tokenExpirationTimestamp - redundancyTimeDiffMs
                         : tokenExpirationTimestamp);
        }

#ifdef ENABLE_NLS_DEBUG_2
        uint64_t e_ms = utility::TextUtils::GetTimestampMs();
        LOG_DEBUG(
            "ConnectedPool(%p) pushPrestartedNode latency request(%p) "
            "lock:%llu init:%llu exist:%llu inexist:%llu",
            this, request, b_ms - a_ms, c_ms - b_ms, d_ms - c_ms, e_ms - d_ms);
#endif

        LOG_INFO(
            "ConnectedPool(%p) pushPrestartedNode input request(%p) node(%p) "
            "done in index(%d/%d), start timestamp(%s), last operation "
            "timestamp(%s), token expiration timestamp(%s). SSL handle "
            "is "
            "%p and SocketFd is %d.",
            this, request, request->getConnectNode(), index, curPool->size(),
            utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
            utility::TextUtils::GetTimeFromMs(it->workableTimestamp).c_str(),
            utility::TextUtils::GetTimeFromMs(it->tokenExpirationTimestamp)
                .c_str(),
            it->sslHandle, it->socketFd);
        MUTEX_UNLOCK_WITH_TAG(_lock, request);
        return true;
      }
    }  // for
  }

#ifdef ENABLE_NLS_DEBUG_2
  uint64_t e_ms = utility::TextUtils::GetTimestampMs();
  LOG_DEBUG(
      "ConnectedPool(%p) pushPrestartedNode latency request(%p) "
      "lock:%llu init:%llu work:%llu",
      this, request, b_ms - a_ms, c_ms - b_ms, e_ms - c_ms);
#endif
  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return false;
}

bool ConnectedPool::pushPrestartedNodeFromPreconnected(INlsRequest *request,
                                                       NlsType type) {
  if (request == NULL) {
    return false;
  }

  MUTEX_LOCK_WITH_TAG(_lock, request);

  int index = request->getConnectNode()->getPoolIndex();
  if (index < 0) {
    LOG_WARN(
        "ConnectedPool(%p) input "
        "request(%p) node(%p) "
        "failed with index:%d.",
        this, request, request->getConnectNode(), index);
    MUTEX_UNLOCK_WITH_TAG(_lock, request);
    return false;
  }

  LOG_DEBUG(
      "ConnectedPool(%p) input request(%p) "
      "ndoe(%p) with "
      "index(%d) and "
      "type[(%d)"
      "0:Asr,1:SpeechTranscriber,2:TTS,3:StreamInputTts] ...",
      this, request, request->getConnectNode(), index, type);

  int ret = Success;
  evutil_socket_t curSocketFd = request->getConnectNode()->getSocketFd();
  SSLconnect *curSslHandle = request->getConnectNode()->getSslHandle();

  struct ConnectedPoolProcess *poolProcess = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool0 = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  switch (type) {
    case TypeAsr:
      poolProcess = &_srRequests;
      curPool0 = &_srRequests.preconnectedRequests;
      curPool = &_srRequests.prestartedRequests;
      break;
    case TypeRealTime:
      poolProcess = &_stRequests;
      curPool0 = &_stRequests.preconnectedRequests;
      curPool = &_stRequests.prestartedRequests;
      break;
    case TypeTts:
      poolProcess = &_syRequests;
      curPool0 = &_syRequests.preconnectedRequests;
      curPool = &_syRequests.prestartedRequests;
      break;
    case TypeStreamInputTts:
      poolProcess = &_fssRequests;
      curPool0 = &_fssRequests.preconnectedRequests;
      curPool = &_fssRequests.prestartedRequests;
      break;
    default:
      break;
  }

  bool result = false;
  if (curPool0 && curPool) {
    /* 查找是否已经存在 */
    if (curPool0->size() >= index) {
      struct ConnectedNodeProcess &node = (*curPool0)[index];
      if (node.status == PreNodeConnected) {
        evutil_socket_t itSocketFd =
            node.request->getConnectNode()->getSocketFd();
        SSLconnect *itSslHandle =
            node.request->getConnectNode()->getSslHandle();

        if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
          for (std::vector<struct ConnectedNodeProcess>::iterator it =
                   curPool->begin();
               it != curPool->end(); ++it) {
            if (it->status == PreNodeToBeCreated && !it->shouldRelease &&
                it->request == NULL) {
              index = std::distance(curPool->begin(), it);
              it->type = node.request->getRequestParam()->_mode;
              it->status = PreNodeStarted;
              it->workableTimestamp = node.request->getConnectNode()
                                          ->getNodeProcess()
                                          ->last_op_timestamp_ms;
              it->startTimestamp = it->workableTimestamp;
              if (node.request->getRequestParam()->_mode == TypeTts) {
                it->ttsVersion = node.request->getRequestParam()->getVersion();
              }
              it->sdkName = request->getRequestParam()->getSdkName();
              it->startedResponse.clear();
              it->request = node.request;
              it->socketFd = curSocketFd;
              it->sslHandle = curSslHandle;
              it->canPick = false;
              it->curRequest = NULL;
              node.request->getConnectNode()->setPoolIndex(index);
              poolProcess->work = true;

              uint64_t tokenExpirationTimestamp =
                  node.request->getRequestParam()->_tokenExpirationTime;
              const int redundancyTimeDiffMs = 3600000;  // 1h
              if (tokenExpirationTimestamp > redundancyTimeDiffMs) {
                uint64_t nowTimestamp = utility::TextUtils::GetTimestampMs();
                const uint64_t diffTimeMs = 43200000;  // 12h
                it->tokenExpirationTimestamp =
                    (tokenExpirationTimestamp >= nowTimestamp + diffTimeMs)
                        ? nowTimestamp + diffTimeMs
                        : (tokenExpirationTimestamp >=
                                   nowTimestamp + redundancyTimeDiffMs
                               ? tokenExpirationTimestamp - redundancyTimeDiffMs
                               : tokenExpirationTimestamp);
              }

              LOG_INFO(
                  "ConnectedPool(%p) pushPrestartedNodeFromPreconnected input "
                  "request(%p) node(%p) "
                  "done in index(%d/%d), start timestamp(%s), last operation "
                  "timestamp(%s), token expiration timestamp(%s). SSL handle "
                  "is "
                  "%p and SocketFd is %d.",
                  this, request, request->getConnectNode(), index,
                  curPool->size(),
                  utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
                  utility::TextUtils::GetTimeFromMs(it->workableTimestamp)
                      .c_str(),
                  utility::TextUtils::GetTimeFromMs(
                      it->tokenExpirationTimestamp)
                      .c_str(),
                  it->sslHandle, it->socketFd);

              result = true;
              break;
            }
          }  // for

          // empty this node
          node.clearNode();
          // node.status = PreNodeToBeCreated;
          // node.startTimestamp = 0;
          // node.workableTimestamp = 0;
          // node.tokenExpirationTimestamp = 0;
          // node.socketFd = INVALID_SOCKET;
          // node.sslHandle = NULL;
          // node.ttsVersion = 0;
          // node.sdkName.clear();
          // node.startedResponse.clear();
          // node.request = NULL;
          // node.curRequest = NULL;
          // node.canPick = false;
          // node.isAbnormal = false;
          // node.shouldRelease = false;
          // node.shouldPreconnect = false;
          // node.curRequestInvalid = false;
        }  // find
      }
    }
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return result;
}

bool ConnectedPool::sslBelongToPool(INlsRequest *request, NlsType type,
                                    bool &oriRequestIsAbnormal,
                                    bool &requestInPool) {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  } else {
    LOG_DEBUG("ConnectedPool(%p) Request(%p) with lock(%p) sslBelongToPool ...",
              this, request, &_lock);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
#endif

  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  evutil_socket_t curSocketFd = request->getConnectNode()->getSocketFd();
  SSLconnect *curSslHandle = request->getConnectNode()->getSslHandle();
  int preSize = getNumberOfPrestartedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.prestartedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.prestartedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.prestartedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.prestartedRequests;
        break;
      default:
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeStarted) {
          evutil_socket_t itSocketFd = it->socketFd;
          SSLconnect *itSslHandle = it->sslHandle;
          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            oriRequestIsAbnormal = it->isAbnormal;
            if (request == it->request) {
              requestInPool = true;
            }
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
          }
        }
      }  // for
    }    // curPool
  }

  preSize = getNumberOfPreconnectedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.preconnectedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.preconnectedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.preconnectedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.preconnectedRequests;
        break;
      default:
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeConnected) {
          evutil_socket_t itSocketFd = it->socketFd;
          SSLconnect *itSslHandle = it->sslHandle;
          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            oriRequestIsAbnormal = it->isAbnormal;
            if (request == it->request) {
              requestInPool = true;
            }
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
          }
        }
      }  // for
    }    // curPool
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return false;
}

void ConnectedPool::curRequestIsAbnormal(INlsRequest *request, NlsType type) {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
#endif

  std::list<int> *curList = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  evutil_socket_t curSocketFd = request->getConnectNode()->getSocketFd();
  SSLconnect *curSslHandle = request->getConnectNode()->getSslHandle();
  int preSize = getNumberOfPrestartedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.prestartedRequests;
        curList = &_srRequests.prestartedIndexList;
        break;
      case TypeRealTime:
        curPool = &_stRequests.prestartedRequests;
        curList = &_stRequests.prestartedIndexList;
        break;
      case TypeTts:
        curPool = &_syRequests.prestartedRequests;
        curList = &_syRequests.prestartedIndexList;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.prestartedRequests;
        curList = &_fssRequests.prestartedIndexList;
        break;
      default:
        curPool = NULL;
        curList = NULL;
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeStarted) {
          evutil_socket_t itSocketFd = it->socketFd;
          SSLconnect *itSslHandle = it->sslHandle;
          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            it->isAbnormal = true;
            int index = std::distance(curPool->begin(), it);
            removeElement(*curList, index);
            LOG_INFO(
                "ConnectedPool(%p) curRequestIsAbnormal Request(%p) SSL(%p) "
                "SocketFd(%d) Index(%d) is abnormal.",
                this, request, curSslHandle, curSocketFd, index);
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return;
          }
        }
      }  // for
    }    // curPool
  }

  preSize = getNumberOfPreconnectedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.preconnectedRequests;
        curList = &_srRequests.preconnectedIndexList;
        break;
      case TypeRealTime:
        curPool = &_stRequests.preconnectedRequests;
        curList = &_stRequests.preconnectedIndexList;
        break;
      case TypeTts:
        curPool = &_syRequests.preconnectedRequests;
        curList = &_syRequests.preconnectedIndexList;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.preconnectedRequests;
        curList = &_fssRequests.preconnectedIndexList;
        break;
      default:
        curPool = NULL;
        curList = NULL;
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeConnected) {
          evutil_socket_t itSocketFd = it->socketFd;
          SSLconnect *itSslHandle = it->sslHandle;
          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            it->isAbnormal = true;
            int index = std::distance(curPool->begin(), it);
            removeElement(*curList, index);
            LOG_INFO(
                "ConnectedPool(%p) curRequestIsAbnormal Request(%p) SSL(%p) "
                "SocketFd(%d) Index(%d) is abnormal.",
                this, request, curSslHandle, curSocketFd, index);
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return;
          }
        }
      }  // for
    }    // curPool
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  LOG_DEBUG("ConnectedPool(%p) Request(%p) curRequestIsAbnormal done", this,
            request);
}

void ConnectedPool::finishPushPreNode(NlsType type, evutil_socket_t curSocketFd,
                                      SSLconnect *curSslHandle, int index,
                                      INlsRequest *request) {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  } else {
    LOG_DEBUG(
        "ConnectedPool(%p) input request(%p) with lock(%p), SSL "
        "handle "
        "is "
        "%p and SocketFd is %d. Index is %d.",
        this, request, &_lock, curSslHandle, curSocketFd, index);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
  LOG_DEBUG(
      "ConnectedPool(%p) input request(%p) with lock(%p), SSL "
      "handle "
      "is "
      "%p and SocketFd is %d. Index is %d.",
      this, request, &_lock, curSslHandle, curSocketFd, index);
#endif

  int findIndex = index >= 0 ? index : 0;
  std::list<int> *curList = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  int preSize = getNumberOfPrestartedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.prestartedRequests;
        curList = &_srRequests.prestartedIndexList;
        break;
      case TypeRealTime:
        curPool = &_stRequests.prestartedRequests;
        curList = &_stRequests.prestartedIndexList;
        break;
      case TypeTts:
        curPool = &_syRequests.prestartedRequests;
        curList = &_syRequests.prestartedIndexList;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.prestartedRequests;
        curList = &_fssRequests.prestartedIndexList;
        break;
      default:
        curPool = NULL;
        curList = NULL;
        break;
    }

#ifdef ENABLE_NLS_DEBUG_2
    uint64_t a_ms = utility::TextUtils::GetTimestampMs();
#endif

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin() + findIndex; it != curPool->end(); ++it) {
        // LOG_DEBUG(
        //     "ConnectedPool(%p) finishPushPreNode index(%d) Request(%p), it
        //     SSL " "handle " "is "
        //     "%p and SocketFd is %d. status is %d. cur SSL handle is %p and "
        //     "SocketFd is %d.",
        //     this, std::distance(curPool->begin(), it), it->request,
        //     it->sslHandle, it->socketFd, it->status, curSslHandle,
        //     curSocketFd);

        if (it->status == PreNodeStarted) {
          evutil_socket_t itSocketFd = it->socketFd;
          SSLconnect *itSslHandle = it->sslHandle;
          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            int curIndex = std::distance(curPool->begin(), it);
#ifdef ENABLE_NLS_DEBUG_2
            uint64_t b_ms = utility::TextUtils::GetTimestampMs();
            LOG_DEBUG(
                "ConnectedPool(%p) finishPushPreNode latency PreNodeStarted "
                "request(%p) "
                "finish:%llu",
                this, it->request, b_ms - a_ms);
#endif
            LOG_INFO(
                "ConnectedPool(%p) request(%p) node(%p) [curRequest(%p)] can "
                "pick. "
                "SSL handle is %p and SocketFd is %d. Index is %d.",
                this, it->request, it->request->getConnectNode(),
                it->curRequest, it->sslHandle, it->socketFd, curIndex);

            it->canPick = true;
            it->curRequest = NULL;
            it->curRequestInvalid = false;
            insertListInOrder(*curList, curIndex);

            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return;
          }
        }
      }  // for
    }    // curPool
  } else {
    // LOG_ERROR("ConnectedPool(%p) Request(%p) cannot get prestarted pool!",
    // this,
    //           request);
  }

  preSize = getNumberOfPreconnectedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.preconnectedRequests;
        curList = &_srRequests.preconnectedIndexList;
        break;
      case TypeRealTime:
        curPool = &_stRequests.preconnectedRequests;
        curList = &_stRequests.preconnectedIndexList;
        break;
      case TypeTts:
        curPool = &_syRequests.preconnectedRequests;
        curList = &_syRequests.preconnectedIndexList;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.preconnectedRequests;
        curList = &_fssRequests.preconnectedIndexList;
        break;
      default:
        curPool = NULL;
        curList = NULL;
        break;
    }

    if (curPool) {
#ifdef ENABLE_NLS_DEBUG_2
      uint64_t c_ms = utility::TextUtils::GetTimestampMs();
#endif
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin() + findIndex; it != curPool->end(); ++it) {
        if (it->status == PreNodeConnected) {
          evutil_socket_t itSocketFd = it->socketFd;
          SSLconnect *itSslHandle = it->sslHandle;
          if (curSocketFd == itSocketFd && curSslHandle == itSslHandle) {
            int curIndex = std::distance(curPool->begin(), it);
            LOG_INFO(
                "ConnectedPool(%p) request(%p) node(%p) [curRequest(%p)] can "
                "pick. "
                "SSL handle is %p and SocketFd is %d. Index is %d.",
                this, it->request, it->request->getConnectNode(),
                it->curRequest, it->sslHandle, it->socketFd, curIndex);

            it->canPick = true;
            it->curRequest = NULL;
            it->curRequestInvalid = false;
            insertListInOrder(*curList, curIndex);

            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return;
          }
        }
      }  // for

#ifdef ENABLE_NLS_DEBUG_2
      uint64_t d_ms = utility::TextUtils::GetTimestampMs();
      LOG_DEBUG(
          "ConnectedPool(%p) finishPushPreNode latency PreNodeConnected "
          "finish:%llu",
          this, d_ms - c_ms);
#endif
    }  // curPool
  } else {
    // LOG_ERROR("ConnectedPool(%p) Request(%p) cannot get preconnected pool!",
    //           this, request);
  }

  LOG_ERROR("ConnectedPool(%p) Request(%p) finishPushPreNode occur exception!",
            this, request);
  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return;
}

bool ConnectedPool::requestInPool(INlsRequest *request, NlsType type) {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
#endif

  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  int preSize = getNumberOfPrestartedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.prestartedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.prestartedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.prestartedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.prestartedRequests;
        break;
      default:
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeStarted) {
          if (request == it->request &&
              request->getConnectNode() == it->request->getConnectNode()) {
            LOG_DEBUG(
                "find prestarted node of request(%p) and node(%p) in "
                "size(%d).",
                request, request->getConnectNode(), preSize);
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
#ifdef ENABLE_NLS_DEBUG_2
          } else {
            LOG_DEBUG(
                "request(%p) is not prestarted request(%p) node(%p) with "
                "index(%d).",
                request, it->request, it->request->getConnectNode(),
                std::distance(curPool->begin(), it));
#endif
          }
        }
      }  // for
    }
  }

  preSize = getNumberOfPreconnectedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.preconnectedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.preconnectedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.preconnectedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.preconnectedRequests;
        break;
      default:
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeConnected) {
          if (request == it->request &&
              request->getConnectNode() == it->request->getConnectNode()) {
            LOG_DEBUG(
                "find preconnected node of request(%p) and node(%p) in "
                "size(%d).",
                request, request->getConnectNode(), preSize);
            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
#ifdef ENABLE_NLS_DEBUG_2
          } else {
            LOG_DEBUG("request(%p) is not preconnected request(%p) node(%p)",
                      request, it->request, it->request->getConnectNode());
#endif
          }
        }
      }  // for
    }
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return false;
}

bool ConnectedPool::deletePreNodeByRequest(INlsRequest *request, NlsType type) {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, request);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) Request(%p) lock with excessive time %llums.",
             this, request, timewait_end - timewait_start);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, request);
#endif

  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  int preSize = getNumberOfPrestartedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.prestartedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.prestartedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.prestartedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.prestartedRequests;
        break;
      default:
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeStarted) {
          if (request == it->request &&
              request->getConnectNode() == it->request->getConnectNode()) {
            LOG_DEBUG(
                "find prestarted node of request(%p) and node(%p) in "
                "size(%d) with SSL handler(%p), and remove from pool.",
                request, request->getConnectNode(), preSize, it->sslHandle);

            // empty this node
            it->clearNode();
            // it->status = PreNodeToBeCreated;
            // it->startTimestamp = 0;
            // it->workableTimestamp = 0;
            // it->socketFd = INVALID_SOCKET;
            // it->sslHandle = NULL;
            // it->ttsVersion = 0;
            // it->startedResponse.clear();
            // it->request = NULL;
            // it->curRequest = NULL;
            // it->canPick = false;

            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
#ifdef ENABLE_NLS_DEBUG_2
          } else {
            LOG_DEBUG(
                "request(%p) is not prestarted request(%p) node(%p) with "
                "index(%d).",
                request, it->request, it->request->getConnectNode(),
                std::distance(curPool->begin(), it));
#endif
          }
        }
      }  // for
    }
  }

  preSize = getNumberOfPreconnectedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.preconnectedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.preconnectedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.preconnectedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.preconnectedRequests;
        break;
      default:
        break;
    }

    if (curPool) {
      bool equalFlag = false;
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeConnected) {
          if (request == it->request &&
              request->getConnectNode() == it->request->getConnectNode()) {
            LOG_DEBUG(
                "find preconnected node of request(%p) and node(%p) in "
                "size(%d) with SSL handler(%p), and remove from pool.",
                request, request->getConnectNode(), preSize, it->sslHandle);

            // empty this node
            it->clearNode();
            // it->status = PreNodeToBeCreated;
            // it->startTimestamp = 0;
            // it->workableTimestamp = 0;
            // it->socketFd = INVALID_SOCKET;
            // it->sslHandle = NULL;
            // it->ttsVersion = 0;
            // it->startedResponse = "";
            // it->request = NULL;
            // it->curRequest = NULL;
            // it->canPick = false;

            MUTEX_UNLOCK_WITH_TAG(_lock, request);
            return true;
#ifdef ENABLE_NLS_DEBUG_2
          } else {
            LOG_DEBUG("request(%p) is not preconnected request(%p) node(%p)",
                      request, it->request, it->request->getConnectNode());
#endif
          }
        }
      }  // for
    }
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, request);
  return false;
}

bool ConnectedPool::deletePreNodeBySSL(SSLconnect *curSslHandle, NlsType type) {
#ifdef ENABLE_NLS_DEBUG_2
  uint64_t timewait_start, timewait_end = 0;
  timewait_start = utility::TextUtils::GetTimestampMs();
  MUTEX_LOCK_WITH_TAG(_lock, curSslHandle);
  timewait_end = utility::TextUtils::GetTimestampMs();
  if (timewait_end - timewait_start > 50) {
    LOG_WARN("ConnectedPool(%p) SSL(%p) lock with excessive time %llums.", this,
             curSslHandle, timewait_end - timewait_start);
  }
#else
  MUTEX_LOCK_WITH_TAG(_lock, curSslHandle);
#endif

  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  int preSize = getNumberOfPrestartedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.prestartedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.prestartedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.prestartedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.prestartedRequests;
        break;
      default:
        break;
    }

    if (curPool) {
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeStarted) {
          if (it->sslHandle == curSslHandle) {
            LOG_DEBUG(
                "find prestarted SSL(%p) in "
                "size(%d), and remove request(%p).",
                curSslHandle, preSize, it->request);

            // empty this node
            if (it->request) {
              delete it->request;
              it->request = NULL;
            }
            it->clearNode();
            // it->status = PreNodeToBeCreated;
            // it->startTimestamp = 0;
            // it->workableTimestamp = 0;
            // it->socketFd = INVALID_SOCKET;
            // it->sslHandle = NULL;
            // it->ttsVersion = 0;
            // it->startedResponse.clear();
            // it->curRequest = NULL;
            // it->canPick = false;
            // it->isAbnormal = false;

            MUTEX_UNLOCK_WITH_TAG(_lock, curSslHandle);
            return true;
          }
        }
      }  // for
    }
  }

  preSize = getNumberOfPreconnectedNodes(type);
  if (preSize > 0) {
    switch (type) {
      case TypeAsr:
        curPool = &_srRequests.preconnectedRequests;
        break;
      case TypeRealTime:
        curPool = &_stRequests.preconnectedRequests;
        break;
      case TypeTts:
        curPool = &_syRequests.preconnectedRequests;
        break;
      case TypeStreamInputTts:
        curPool = &_fssRequests.preconnectedRequests;
        break;
      default:
        curPool = NULL;
        break;
    }

    if (curPool) {
      bool equalFlag = false;
      std::vector<struct ConnectedNodeProcess>::iterator it;
      for (it = curPool->begin(); it != curPool->end(); ++it) {
        if (it->status == PreNodeConnected) {
          if (it->sslHandle == curSslHandle) {
            LOG_DEBUG(
                "find preconnected SSL(%p) in "
                "size(%d), and remove request(%p).",
                curSslHandle, preSize, it->request);

            // empty this node
            if (it->request) {
              delete it->request;
              it->request = NULL;
            }
            it->clearNode();
            // it->status = PreNodeToBeCreated;
            // it->startTimestamp = 0;
            // it->workableTimestamp = 0;
            // it->socketFd = INVALID_SOCKET;
            // it->sslHandle = NULL;
            // it->ttsVersion = 0;
            // it->startedResponse.clear();
            // it->curRequest = NULL;
            // it->canPick = false;
            // it->isAbnormal = false;

            MUTEX_UNLOCK_WITH_TAG(_lock, curSslHandle);
            return true;
          }
        }
      }  // for
    }
  }

  MUTEX_UNLOCK_WITH_TAG(_lock, curSslHandle);
  return false;
}

int ConnectedPool::getNumberOfThisTypeNodes(NlsType type, int &prestarted,
                                            int &preconnected) {
  switch (type) {
    case TypeAsr:
      prestarted = _srRequests.prestartedRequests.size();
      break;
    case TypeRealTime:
      prestarted = _stRequests.prestartedRequests.size();
      break;
    case TypeTts:
      prestarted = _syRequests.prestartedRequests.size();
      break;
    case TypeStreamInputTts:
      prestarted = _fssRequests.prestartedRequests.size();
      break;
    default:
      prestarted = 0;
      break;
  }

  switch (type) {
    case TypeAsr:
      preconnected = _srRequests.preconnectedRequests.size();
      break;
    case TypeRealTime:
      preconnected = _stRequests.preconnectedRequests.size();
      break;
    case TypeTts:
      preconnected = _syRequests.preconnectedRequests.size();
      break;
    case TypeStreamInputTts:
      preconnected = _fssRequests.preconnectedRequests.size();
      break;
    default:
      preconnected = 0;
      break;
  }

  return Success;
}

int ConnectedPool::getNumberOfPreconnectedNodes(NlsType type) {
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  switch (type) {
    case TypeAsr:
      curPool = &_srRequests.preconnectedRequests;
      break;
    case TypeRealTime:
      curPool = &_stRequests.preconnectedRequests;
      break;
    case TypeTts:
      curPool = &_syRequests.preconnectedRequests;
      break;
    case TypeStreamInputTts:
      curPool = &_fssRequests.preconnectedRequests;
      break;
    default:
      break;
  }

  int count = 0;
  if (curPool) {
    size_t size = curPool->size();
    for (std::vector<struct ConnectedNodeProcess>::iterator it =
             curPool->begin();
         it != curPool->end(); ++it) {
      if (it->status == PreNodeConnected) {
        count++;
      }
    }  // for
  }
  return count;
}

int ConnectedPool::getNumberOfPrestartedNodes(NlsType type) {
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  switch (type) {
    case TypeAsr:
      curPool = &_srRequests.prestartedRequests;
      break;
    case TypeRealTime:
      curPool = &_stRequests.prestartedRequests;
      break;
    case TypeTts:
      curPool = &_syRequests.prestartedRequests;
      break;
    case TypeStreamInputTts:
      curPool = &_fssRequests.prestartedRequests;
      break;
    default:
      break;
  }

  int count = 0;
  if (curPool) {
    size_t size = curPool->size();
    for (std::vector<struct ConnectedNodeProcess>::iterator it =
             curPool->begin();
         it != curPool->end(); ++it) {
      if (it->status == PreNodeStarted) {
        count++;
      }
    }  // for
  }
  return count;
}

int ConnectedPool::initThisNodesPool(NlsType type) {
  // LOG_DEBUG("ConnectedPool(%p) initThisNodesPool ...", this);
  std::vector<struct ConnectedNodeProcess> *curPrestartedPool = NULL;
  std::vector<struct ConnectedNodeProcess> *curPreconnectedPool = NULL;
  switch (type) {
    case TypeAsr:
      curPrestartedPool = &_srRequests.prestartedRequests;
      curPreconnectedPool = &_srRequests.preconnectedRequests;
      break;
    case TypeRealTime:
      curPrestartedPool = &_stRequests.prestartedRequests;
      curPreconnectedPool = &_stRequests.preconnectedRequests;
      break;
    case TypeTts:
      curPrestartedPool = &_syRequests.prestartedRequests;
      curPreconnectedPool = &_syRequests.preconnectedRequests;
      break;
    case TypeStreamInputTts:
      curPrestartedPool = &_fssRequests.prestartedRequests;
      curPreconnectedPool = &_fssRequests.preconnectedRequests;
      break;
    default:
      break;
  }

  if (curPrestartedPool) {
    size_t size = curPrestartedPool->size();
    for (size_t i = size; i < _maxPreconnectedNumber; ++i) {
      struct ConnectedNodeProcess tmp;
      tmp.type = type;
      tmp.status = PreNodeToBeCreated;
      curPrestartedPool->push_back(tmp);
    }  // for
  }

  if (curPreconnectedPool) {
    size_t size = curPreconnectedPool->size();
    for (size_t i = size; i < _maxPreconnectedNumber; ++i) {
      struct ConnectedNodeProcess tmp;
      tmp.type = type;
      tmp.status = PreNodeToBeCreated;
      curPreconnectedPool->push_back(tmp);
    }  // for
  }

  return Success;
}

bool ConnectedPool::popOnePreconnectedNode(INlsRequest *request, NlsType type) {
#ifdef ENABLE_NLS_DEBUG_2
  LOG_DEBUG(
      "ConnectedPool(%p) popOnePreconnectedNode request(%p) with type(%d) "
      "...",
      this, request, type);
#endif

  std::list<int> *curList = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  switch (type) {
    case TypeAsr:
      curPool = &_srRequests.preconnectedRequests;
      curList = &_srRequests.prestartedIndexList;
      break;
    case TypeRealTime:
      curPool = &_stRequests.preconnectedRequests;
      curList = &_stRequests.preconnectedIndexList;
      break;
    case TypeTts:
      curPool = &_syRequests.preconnectedRequests;
      curList = &_syRequests.preconnectedIndexList;
      break;
    case TypeStreamInputTts:
      curPool = &_fssRequests.preconnectedRequests;
      curList = &_fssRequests.preconnectedIndexList;
      break;
    default:
      curPool = NULL;
      curList = NULL;
      break;
  }

  if (curPool) {
    int popIndex = popListFront(*curList);
    std::vector<struct ConnectedNodeProcess>::iterator it;
    for (it = curPool->begin() + popIndex; it != curPool->end(); ++it) {
      if (it->status == PreNodeConnected) {
        if (it->canPick) {        /*SSL处于空闲, 可取走SSL*/
          bool equalFlag = false; /* 判断request参数是否相同 */
          INlsRequestParam *paramsInRequest = request->getRequestParam();
          INlsRequestParam *paramsInPool = it->request->getRequestParam();
          if (paramsInRequest && paramsInPool) {
            switch (type) {
              case TypeAsr:
                equalFlag = *paramsInPool == *paramsInRequest &&
                            it->sdkName == paramsInRequest->getSdkName();
                break;
              case TypeRealTime:
                equalFlag = *paramsInPool == *paramsInRequest &&
                            it->sdkName == paramsInRequest->getSdkName();
                break;
              case TypeTts:
                equalFlag = *paramsInPool == *paramsInRequest &&
                            it->sdkName == paramsInRequest->getSdkName() &&
                            it->ttsVersion == paramsInRequest->getVersion();
                break;
              case TypeStreamInputTts:
                equalFlag = *paramsInPool == *paramsInRequest &&
                            it->sdkName == paramsInRequest->getSdkName();
                break;
              default:
                break;
            }
          } else {
            LOG_ERROR(
                "ConnectedPool(%p) input invalid request(%p) params(%p) and "
                "item request(%p) params(%p).",
                this, request, paramsInRequest, it->request, paramsInPool);
          }
          if (equalFlag) { /* request参数相同*/
            int index = std::distance(curPool->begin(), it);
            // fill node
            SSLconnect *oldSSL = request->getConnectNode()->getSslHandle();
            evutil_socket_t oldSocketFd =
                request->getConnectNode()->getSocketFd();
            delete oldSSL;
            evutil_closesocket(oldSocketFd);
            urlAddress *dstUrlAddress =
                request->getConnectNode()->getUrlAddressPointer();
            urlAddress *srcUrlAddress =
                it->request->getConnectNode()->getUrlAddressPointer();
            request->getConnectNode()->setSslHandle(it->sslHandle);
            request->getConnectNode()->setSocketFd(it->socketFd);
            memcpy(dstUrlAddress, srcUrlAddress, sizeof(struct urlAddress));
            it->canPick = false;
            it->curRequest = request;
            it->curRequestInvalid = false;
            request->getConnectNode()->setPoolIndex(index);
            removeElement(*curList, index);

            LOG_INFO(
                "ConnectedPool(%p) popOnePreconnectedNode request(%p) "
                "node(%p) "
                "with "
                "type(%d) index(%d/%d) done, reset SSL handle %p to %p and "
                "reset "
                "SocketFd %d "
                "to %d.",
                this, request, request->getConnectNode(), type,
                std::distance(curPool->begin(), it), curPool->size(), oldSSL,
                request->getConnectNode()->getSslHandle(), oldSocketFd,
                request->getConnectNode()->getSocketFd());
            return true;
          }  // equalFlag
        } else {
          // LOG_DEBUG(
          //     "ConnectedPool(%p) popOnePreconnectedNode request(%p) node(%p)
          //     " "is not ready ...", this, request,
          //     request->getConnectNode());
        }
      }
    }  // for
  }    // curPool
  return false;
}

bool ConnectedPool::popOnePrestartedNode(INlsRequest *request, NlsType type) {
#ifdef ENABLE_NLS_DEBUG_2
  LOG_DEBUG(
      "ConnectedPool(%p) popOnePrestartedNode request(%p) with type(%d) "
      "...",
      this, request, type);
#endif

  std::list<int> *curList = NULL;
  std::vector<struct ConnectedNodeProcess> *curPool = NULL;
  switch (type) {
    case TypeAsr:
      curPool = &_srRequests.prestartedRequests;
      curList = &_srRequests.prestartedIndexList;
      break;
    case TypeRealTime:
      curPool = &_stRequests.prestartedRequests;
      curList = &_stRequests.prestartedIndexList;
      break;
    case TypeTts:
      curPool = &_syRequests.prestartedRequests;
      curList = &_syRequests.prestartedIndexList;
      break;
    case TypeStreamInputTts:
      curPool = &_fssRequests.prestartedRequests;
      curList = &_fssRequests.prestartedIndexList;
      break;
    default:
      curPool = NULL;
      curList = NULL;
      break;
  }

  if (curPool) {
    int popIndex = popListFront(*curList);
    std::vector<struct ConnectedNodeProcess>::iterator it;
    for (it = curPool->begin() + popIndex; it != curPool->end(); ++it) {
      if (it->status == PreNodeStarted) {
        if (it->canPick) {        /*SSL处于空闲, 可取走SSL*/
          bool equalFlag = false; /* 判断request参数是否相同 */
          INlsRequestParam *paramsInPool = it->request->getRequestParam();
          INlsRequestParam *paramsInRequest = request->getRequestParam();
          if (paramsInRequest && paramsInPool) {
            switch (type) {
              case TypeAsr:
                equalFlag =
                    *paramsInPool == *paramsInRequest &&
                    paramsInPool->getSdkName() == paramsInRequest->getSdkName();
                break;
              case TypeRealTime:
                equalFlag =
                    *paramsInPool == *paramsInRequest &&
                    paramsInPool->getSdkName() == paramsInRequest->getSdkName();
                break;
              case TypeTts:
                equalFlag =
                    *paramsInPool == *paramsInRequest &&
                    paramsInPool->getSdkName() ==
                        paramsInRequest->getSdkName() &&
                    paramsInPool->getVersion() == paramsInRequest->getVersion();
                break;
              case TypeStreamInputTts:
                equalFlag =
                    *paramsInPool == *paramsInRequest &&
                    paramsInPool->getSdkName() == paramsInRequest->getSdkName();
                break;
              default:
                break;
            }
          } else {
            LOG_ERROR(
                "ConnectedPool(%p) input invalid request(%p) params(%p) and "
                "item request(%p) params(%p).",
                this, request, paramsInRequest, it->request, paramsInPool);
          }
          if (equalFlag) { /* request参数相同*/
            int index = std::distance(curPool->begin(), it);
            // fill node
            SSLconnect *oldSSL = request->getConnectNode()->getSslHandle();
            evutil_socket_t oldSocketFd =
                request->getConnectNode()->getSocketFd();
            delete oldSSL;
            evutil_closesocket(oldSocketFd);
            urlAddress *dstUrlAddress =
                request->getConnectNode()->getUrlAddressPointer();
            urlAddress *srcUrlAddress =
                it->request->getConnectNode()->getUrlAddressPointer();
            request->getConnectNode()->setSslHandle(it->sslHandle);
            request->getConnectNode()->setSocketFd(it->socketFd);
            memcpy(dstUrlAddress, srcUrlAddress, sizeof(struct urlAddress));
            it->canPick = false;
            it->curRequest = request;
            it->curRequestInvalid = false;
            request->getConnectNode()->setPoolIndex(index);
            removeElement(*curList, index);

            LOG_INFO(
                "ConnectedPool(%p) popOnePrestartedNode request(%p) node(%p) "
                "with "
                "type(%d) index(%d/%d) done, reset SSL handle %p to %p and "
                "reset "
                "SocketFd %d "
                "to %d.",
                this, request, request->getConnectNode(), type, index,
                curPool->size(), oldSSL,
                request->getConnectNode()->getSslHandle(), oldSocketFd,
                request->getConnectNode()->getSocketFd());
            return true;
          }  // equalFlag
        }    // canPick
      } else {
        // LOG_DEBUG(
        //     "ConnectedPool(%p) popOnePrestartedNode(index:%d) request(%p) "
        //     "node(%p) with SSL handle(%p) and socketFd(%d) "
        //     "is not ready, cannot pick ...",
        //     this, std::distance(curPool->begin(), it), request,
        //     request->getConnectNode(),
        //     request->getConnectNode()->getSslHandle(),
        //     request->getConnectNode()->getSocketFd());
      }
    }  // for
  }    // curPool
  return false;
}

void ConnectedPool::deletePreNode(
    std::vector<struct ConnectedNodeProcess> *pool) {
  if (pool) {
    for (std::vector<struct ConnectedNodeProcess>::iterator it = pool->begin();
         it != pool->end(); ++it) {
      if (it->request) {
        delete it->request;
        it->request = NULL;
      }
      it->socketFd = INVALID_SOCKET;
      it->sslHandle = NULL;
      it->curRequest = NULL;
      it->canPick = false;
    }  // for

    pool->clear();
  }
}

int ConnectedPool::timeoutPrestartedNode(
    std::vector<struct ConnectedNodeProcess> *pool) {
  int releaseCount = 0;
  std::vector<struct ConnectedNodeProcess>::iterator it;
  for (it = pool->begin(); it != pool->end(); ++it) {
    if (it->status == PreNodeStarted) {
      uint64_t curTimestamp = utility::TextUtils::GetTimestampMs();
      // LOG_DEBUG(
      //     "it->request:%p, "
      //     "it->curRequest->getConnectNode():%p",
      //     it->request);
      // update workableTimestamp from current request
      if (!it->curRequestInvalid && it->curRequest &&
          it->curRequest->getConnectNode() &&
          it->curRequest->getConnectNode()->getNodeProcess() &&
          it->curRequest->getConnectNode()
                  ->getNodeProcess()
                  ->last_op_timestamp_ms > 0) {
        // LOG_DEBUG(
        //     "it->curRequest:%p, "
        //     "it->curRequest->getConnectNode():%p",
        //     it->curRequest, it->curRequest->getConnectNode());
        it->workableTimestamp = it->curRequest->getConnectNode()
                                    ->getNodeProcess()
                                    ->last_op_timestamp_ms;
      }
      uint64_t gapTimestamp = curTimestamp - it->workableTimestamp;
      bool timeout = gapTimestamp >= _prerequestedTimeoutMs;
      bool tokenTimeout = it->tokenExpirationTimestamp > 0 &&
                          curTimestamp >= it->tokenExpirationTimestamp;
      if (timeout || tokenTimeout) {
        if (it->curRequest == NULL) {
          LOG_WARN(
              "Pool(%p:(%p)%p-%p) connectPoolEventCallback should release "
              "prestarted "
              "request(%p) SSL handler(%p) index(%d) "
              "because of %llums. start timestamp is %llu(%s), last "
              "operate timestamp is %llu(%s), and token expiration timestamp "
              "is %llu(%s).",
              this, pool, pool->begin(), pool->end(), it->request,
              it->sslHandle, std::distance(pool->begin(), it), gapTimestamp,
              it->startTimestamp,
              utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
              it->workableTimestamp,
              utility::TextUtils::GetTimeFromMs(it->workableTimestamp).c_str(),
              it->tokenExpirationTimestamp,
              utility::TextUtils::GetTimeFromMs(it->tokenExpirationTimestamp)
                  .c_str());

          // empty this node， delete request later
          it->status = PreNodeToBeCreated;
          it->startTimestamp = 0;
          it->workableTimestamp = 0;
          it->tokenExpirationTimestamp = 0;
          it->socketFd = INVALID_SOCKET;
          it->sslHandle = NULL;
          it->ttsVersion = 0;
          it->sdkName.clear();
          it->startedResponse.clear();
          it->canPick = false;
          it->curRequestInvalid = false;
          it->shouldRelease = true;
          if (!tokenTimeout) {
            it->shouldPreconnect = true;
          }
          releaseCount++;
        } else {
          LOG_WARN(
              "Pool(%p:(%p)%p-%p) connectPoolEventCallback prestarted "
              "request(%p) curRequest(%p) idle "
              "timeout:%llums. start timestamp is %llu(%s). But request(%p) is "
              "using, check later ...",
              this, pool, pool->begin(), pool->end(), it->request,
              it->curRequest, gapTimestamp, it->startTimestamp,
              utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
              it->curRequest);
        }
      } else {
        // valid connection
        int pingResult = Success;
        if (it->request && it->request->getConnectNode()) {
          pingResult = it->request->getConnectNode()->syncPingCmd();
        }
        if (pingResult != Success) {
          LOG_WARN(
              "Pool(%p:(%p)%p-%p) connectPoolEventCallback should release "
              "prestarted "
              "request(%p) SSL handler(%p) index(%d) "
              "because of ping failed. start timestamp is %llu(%s), last "
              "operate timestamp is %llu(%s), and token expiration timestamp "
              "is %llu(%s).",
              this, pool, pool->begin(), pool->end(), it->request,
              it->sslHandle, std::distance(pool->begin(), it),
              it->startTimestamp,
              utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
              it->workableTimestamp,
              utility::TextUtils::GetTimeFromMs(it->workableTimestamp).c_str(),
              it->tokenExpirationTimestamp,
              utility::TextUtils::GetTimeFromMs(it->tokenExpirationTimestamp)
                  .c_str());

          // empty this node, delete request later
          it->status = PreNodeToBeCreated;
          it->startTimestamp = 0;
          it->workableTimestamp = 0;
          it->socketFd = INVALID_SOCKET;
          it->sslHandle = NULL;
          it->ttsVersion = 0;
          it->sdkName.clear();
          it->startedResponse.clear();
          it->canPick = false;
          it->shouldPreconnect = false;
          it->curRequestInvalid = false;
          it->shouldRelease = true;
          releaseCount++;
        }
        // LOG_DEBUG(
        //     "Pool(%p) connectPoolEventCallback "
        //     "request(%p) %llu - %llu = %llu.",
        //     this, it->request, curTimestamp, it->workableTimestamp,
        //     gapTimestamp);
      }
    }
  }  // for

  return releaseCount;
}

int ConnectedPool::timeoutPreconnectedNode(
    std::vector<struct ConnectedNodeProcess> *pool) {
  int releaseCount = 0;
  std::vector<struct ConnectedNodeProcess>::iterator it;
  for (it = pool->begin(); it != pool->end(); ++it) {
    if (it->status == PreNodeConnected) {
      uint64_t curTimestamp = utility::TextUtils::GetTimestampMs();
      // update workableTimestamp from current request
      if (!it->curRequestInvalid && it->curRequest &&
          it->curRequest->getConnectNode() &&
          it->curRequest->getConnectNode()->getNodeProcess() &&
          it->curRequest->getConnectNode()
                  ->getNodeProcess()
                  ->last_op_timestamp_ms > 0) {
        it->workableTimestamp = it->curRequest->getConnectNode()
                                    ->getNodeProcess()
                                    ->last_op_timestamp_ms;
      }
      uint64_t gapTimestamp = curTimestamp - it->workableTimestamp;
      bool timeout = gapTimestamp >= _preconnectedTimeoutMs;
      bool tokenTimeout = it->tokenExpirationTimestamp > 0 &&
                          curTimestamp >= it->tokenExpirationTimestamp;
      if (timeout || tokenTimeout) {
        if (it->curRequest == NULL) {
          LOG_WARN(
              "Pool(%p:(%p)%p-%p) connectPoolEventCallback should release "
              "preconnected "
              "request(%p) SSL handler(%p) index(%d) "
              "because of %llums. start timestamp is %llu(%s), last "
              "operate timestamp is %llu(%s), and token expiration timestamp "
              "is %llu(%s).",
              this, pool, pool->begin(), pool->end(), it->request,
              it->sslHandle, std::distance(pool->begin(), it), gapTimestamp,
              it->startTimestamp,
              utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
              it->workableTimestamp,
              utility::TextUtils::GetTimeFromMs(it->workableTimestamp).c_str(),
              it->tokenExpirationTimestamp,
              utility::TextUtils::GetTimeFromMs(it->tokenExpirationTimestamp)
                  .c_str());

          // empty this node, delete request later
          it->status = PreNodeToBeCreated;
          it->startTimestamp = 0;
          it->workableTimestamp = 0;
          it->socketFd = INVALID_SOCKET;
          it->sslHandle = NULL;
          it->ttsVersion = 0;
          it->startedResponse.clear();
          it->canPick = false;
          it->shouldRelease = true;
          if (!tokenTimeout) {
            it->shouldPreconnect = true;
          }
          it->curRequestInvalid = false;
          releaseCount++;
        } else {
          LOG_WARN(
              "Pool(%p:(%p)%p-%p) connectPoolEventCallback preconnected "
              "request(%p) curRequest(%p) idle "
              "timeout:%llums. start timestamp is %llu(%s). But request(%p) is "
              "using, check later ...",
              this, pool, pool->begin(), pool->end(), it->request,
              it->curRequest, gapTimestamp, it->startTimestamp,
              utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
              it->curRequest);
        }
      } else {
        // valid connection
        int pingResult = Success;
        if (it->request && it->request->getConnectNode()) {
          pingResult = it->request->getConnectNode()->syncPingCmd();
        }
        if (pingResult != Success) {
          LOG_WARN(
              "Pool(%p:(%p)%p-%p) connectPoolEventCallback should release "
              "preconnected "
              "request(%p) SSL handler(%p) index(%d) "
              "because of ping failed(%d). start timestamp is %llu(%s), last "
              "operate timestamp is %llu(%s), and token expiration timestamp "
              "is %llu(%s).",
              this, pool, pool->begin(), pool->end(), it->request,
              it->sslHandle, std::distance(pool->begin(), it), pingResult,
              it->startTimestamp,
              utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
              it->workableTimestamp,
              utility::TextUtils::GetTimeFromMs(it->workableTimestamp).c_str(),
              it->tokenExpirationTimestamp,
              utility::TextUtils::GetTimeFromMs(it->tokenExpirationTimestamp)
                  .c_str());

          // empty this node, delete request later
          it->status = PreNodeToBeCreated;
          it->startTimestamp = 0;
          it->workableTimestamp = 0;
          it->tokenExpirationTimestamp = 0;
          it->socketFd = INVALID_SOCKET;
          it->sslHandle = NULL;
          it->ttsVersion = 0;
          it->sdkName.clear();
          it->startedResponse.clear();
          it->canPick = false;
          it->shouldPreconnect = false;
          it->curRequestInvalid = false;
          it->shouldRelease = true;
          releaseCount++;
        }
        // LOG_DEBUG(
        //     "Pool(%p) connectPoolEventCallback "
        //     "request(%p) %llu - %llu = %llu.",
        //     this, it->request, curTimestamp, it->workableTimestamp,
        //     gapTimestamp);
      }
    }
  }  // for

  return releaseCount;
}

void ConnectedPool::deleteOrPreconnectNodeShouldReleased(
    std::vector<struct ConnectedNodeProcess> *pool, std::string name) {
  // LOG_DEBUG("Pool(%p:(%p)) Name(%s) begin ...", this, pool, name.c_str());

  std::vector<struct ConnectedNodeProcess>::iterator it;
  for (it = pool->begin(); it != pool->end(); ++it) {
    // LOG_DEBUG(
    //     "Pool(%p:(%p)) Name(%s) Request(%p) index(%d) shouldRelease(%s) "
    //     "shouldPreconnect(%s) ...",
    //     this, pool, name.c_str(), it->request, std::distance(pool->begin(),
    //     it), it->shouldRelease ? "true" : "false", it->shouldPreconnect ?
    //     "true" : "false");

    if (it->shouldRelease) {
      it->shouldRelease = false;
      if (it->shouldPreconnect) {
        LOG_INFO(
            "Request(%p) %s index(%d) needs to preconnect now and then delete "
            "...",
            it->request, name.c_str(), std::distance(pool->begin(), it));
        it->shouldPreconnect = false;
        preconnectNodeByRequest(it->request);  // it->request is old request
        // LOG_DEBUG("Request(%p) push into pool finish.", it->request);
      } else {
        LOG_INFO(
            "Request(%p) %s index(%d) needs to be deleted now because the "
            "timeout "
            "...",
            it->request, name.c_str(), std::distance(pool->begin(), it));
      }

      // delete old request
      if (it->request) {
        bool release_lock_ret = true;
        NlsClientImpl *cur_instance =
            it->request->getConnectNode()->getInstance();
        if (cur_instance != NULL) {
          MUTEX_TRY_LOCK_WITH_TAG(cur_instance->_mtxReleaseRequestGuard, 2000,
                                  release_lock_ret, it->request);
          if (!release_lock_ret) {
            LOG_ERROR("Request(%p) lock destroy failed, deadlock has occurred",
                      it->request);
          }
        } else {
          LOG_ERROR("Request(%p) just only created ...", it->request);
          release_lock_ret = false;
        }

        delete it->request;

        if (release_lock_ret) {
          MUTEX_UNLOCK_WITH_TAG(cur_instance->_mtxReleaseRequestGuard,
                                it->request);
        }
      }

      // empty this node
      it->clearNode();
      // it->status = PreNodeToBeCreated;
      // it->startTimestamp = 0;
      // it->workableTimestamp = 0;
      // it->socketFd = INVALID_SOCKET;
      // it->sslHandle = NULL;
      // it->ttsVersion = 0;
      // it->startedResponse = "";
      // it->curRequest = NULL;
      // it->canPick = false;
      // it->request = NULL;
      // it->isAbnormal = false;
    }  // shouldRelease is true
  }    // for

  // LOG_DEBUG("Pool(%p:(%p)) Name(%s) done.", this, pool, name.c_str());
}

void ConnectedPool::preconnectNodeByRequest(INlsRequest *request) {
  if (request) {
    INlsRequest *newRequest = NULL;
    INlsRequestParam *requestParam = request->getRequestParam();
    ConnectNode *node = request->getConnectNode();
    NlsType type = requestParam->_mode;

    LOG_DEBUG(
        "ConnectedPool(%p) preconnectNodeByRequest old request(%p) with "
        "type:%d.",
        this, request, type);

    switch (type) {
      case TypeAsr:
        newRequest = NlsClient::getInstance()->createRecognizerRequest(
            requestParam->getSdkName().c_str(), node->isLongConnection());
        break;
      case TypeRealTime:
        newRequest = NlsClient::getInstance()->createTranscriberRequest(
            requestParam->getSdkName().c_str(), node->isLongConnection());
        break;
      case TypeTts:
        newRequest = NlsClient::getInstance()->createSynthesizerRequest(
            (TtsVersion)requestParam->getVersion(),
            requestParam->getSdkName().c_str(), node->isLongConnection());
        break;
      case TypeStreamInputTts:
        newRequest = NlsClient::getInstance()->createFlowingSynthesizerRequest(
            requestParam->getSdkName().c_str(), node->isLongConnection());
        break;
      default:
        break;
    }  // switch

    if (newRequest) {
      LOG_INFO(
          "ConnectedPool(%p) create new request(%p) from old request(%p) for "
          "PreconnectedPool.",
          this, newRequest, request);
      *(newRequest->getRequestParam()) = *(request->getRequestParam());
      newRequest->getConnectNode()->usePreNodeStartStepByStep(true);
      int ret = NlsEventNetWork::_eventClient->startInner(newRequest);
      bool result = false;
      if (ret == Success) {
        result = newRequest->getConnectNode()->directLinkIpFromCache();
      }
      if (result) {
        result = pushPreconnectedNode(newRequest, type, true);
        if (result) {
          finishPushPreNode(type, newRequest->getConnectNode()->getSocketFd(),
                            newRequest->getConnectNode()->getSslHandle(),
                            newRequest->getConnectNode()->getPoolIndex(),
                            newRequest);
        } else {
          deletePreNodeBySSL(newRequest->getConnectNode()->getSslHandle(),
                             type);
        }
      } else {
        switch (type) {
          case TypeAsr:
            NlsClient::getInstance()->releaseRecognizerRequest(
                (SpeechRecognizerRequest *)newRequest);
            break;
          case TypeRealTime:
            NlsClient::getInstance()->releaseTranscriberRequest(
                (SpeechTranscriberRequest *)newRequest);
            break;
          case TypeTts:
            NlsClient::getInstance()->releaseSynthesizerRequest(
                (SpeechSynthesizerRequest *)newRequest);
            break;
          case TypeStreamInputTts:
            NlsClient::getInstance()->releaseFlowingSynthesizerRequest(
                (FlowingSynthesizerRequest *)newRequest);
            break;
          default:
            break;
        }  // switch
      }
    }
  } else {
    LOG_ERROR("ConnectedPool(%p) preconnectNodeByRequest request is null.",
              this, request);
  }
}

void ConnectedPool::showEveryNode(
    std::vector<struct ConnectedNodeProcess> *pool, std::string name) {
  LOG_DEBUG("==>> ConnectedPool(%p:(%p)%p-%p) show every node in pool %s ...",
            this, pool, pool->begin(), pool->end(), name.c_str());

  LOG_DEBUG(" %s =>", name.c_str());

  std::vector<struct ConnectedNodeProcess>::iterator it;
  for (it = pool->begin(); it != pool->end(); ++it) {
    LOG_DEBUG("   index(%d) status:%s", std::distance(pool->begin(), it),
              getStatusStr(it->status).c_str());
    LOG_DEBUG("    request:%p curRequest:%p", it->request, it->curRequest);
    if (it->request) {
      LOG_DEBUG("    node:%p", it->request->getConnectNode());
    }
    LOG_DEBUG("    socketFd:%d sslHandle:%p", it->socketFd, it->sslHandle);
    if (it->request && it->request->getConnectNode()) {
      LOG_DEBUG("    node socketFd:%d sslHandle:%p",
                it->request->getConnectNode()->getSocketFd(),
                it->request->getConnectNode()->getSslHandle());
    }
    LOG_DEBUG(
        "    shouldRelease:%s shouldPreconnect:%s isAbnormal:%s "
        "canPick:%s curRequestInvalid:%s",
        it->shouldRelease ? "true" : "false",
        it->shouldPreconnect ? "true" : "false",
        it->isAbnormal ? "true" : "false", it->canPick ? "true" : "false",
        it->curRequestInvalid ? "true" : "false");
    LOG_DEBUG("    start:%s workable:%s tokenExpiration:%s",
              utility::TextUtils::GetTimeFromMs(it->startTimestamp).c_str(),
              utility::TextUtils::GetTimeFromMs(it->workableTimestamp).c_str(),
              utility::TextUtils::GetTimeFromMs(it->tokenExpirationTimestamp)
                  .c_str());
  }

  LOG_DEBUG("<<== ConnectedPool(%p) show every node done", this);
}

std::string ConnectedPool::getStatusStr(ConnectedStatus status) {
  std::string result = "unknown";
  switch (status) {
    case PreNodeInvalid:
      result = "PreNodeInvalid";
      break;
    case PreNodeToBeCreated:
      result = "PreNodeToBeCreated";
      break;
    case PreNodeCreated:
      result = "PreNodeCreated";
      break;
    case PreNodeConnected:
      result = "PreNodeConnected";
      break;
    case PreNodeStarted:
      result = "PreNodeStarted";
      break;
    default:
      break;
  }
  return result;
}

void ConnectedPool::insertListInOrder(std::list<int> &lst, int a) {
  std::list<int>::iterator it = std::lower_bound(lst.begin(), lst.end(), a);
  lst.insert(it, a);
}

void ConnectedPool::removeElement(std::list<int> &lst, int a) { lst.remove(a); }

int ConnectedPool::popListFront(std::list<int> &lst) {
  int index = 0;
  if (!lst.empty()) {
    index = lst.front();
    lst.pop_front();
  }
  return index;
}

}  // namespace AlibabaNls
