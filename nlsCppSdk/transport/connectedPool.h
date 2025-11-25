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

#ifndef NLS_SDK_CONNECTED_POOL_H
#define NLS_SDK_CONNECTED_POOL_H

#include <list>
#include <vector>
#if defined(_MSC_VER)
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "SSLconnect.h"
#include "connectNode.h"
#include "event2/util.h"
#include "flowingSynthesizerParam.h"
#include "dashCosyVoiceSynthesizerParam.h"
#include "iNlsRequest.h"
#include "nlog.h"
#include "speechRecognizerParam.h"
#include "speechSynthesizerParam.h"
#include "speechTranscriberParam.h"

namespace AlibabaNls {

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

enum ConnectedStatus {
  PreNodeInvalid = 0,
  PreNodeToBeCreated,
  PreNodeCreated,
  PreNodeConnected,
  PreNodeStarted,
};

struct ConnectedNodeProcess {
 public:
  explicit ConnectedNodeProcess()
      : status(PreNodeInvalid),
        startTimestamp(0),
        workableTimestamp(0),
        tokenExpirationTimestamp(0),
        socketFd(INVALID_SOCKET),
        sslHandle(NULL),
        canPick(false),
        ttsVersion(ShortTts),
        request(NULL),
        isAbnormal(false),
        shouldRelease(false),
        shouldPreconnect(false),
        curRequest(NULL),
        curRequestInvalid(false),
        sdkName(""),
        startedResponse(""){};
  ~ConnectedNodeProcess() {
    if (request) {
      delete request;
      request = NULL;
    }
    curRequest = NULL;
    curRequestInvalid = false;
    isAbnormal = false;
    shouldRelease = false;
    shouldPreconnect = false;
    canPick = false;
  };

  NlsType type;
  ConnectedStatus status;
  uint64_t startTimestamp;
  uint64_t workableTimestamp;
  uint64_t tokenExpirationTimestamp;
  /* socketFd & sslHandle 是判断节点的条件 */
  evutil_socket_t socketFd;
  SSLconnect *sslHandle;
  bool canPick; /* ConnectedNode中SSL可被取走 */
  int ttsVersion;
  /* socketFd & sslHandle 所在的最早的request, 它将在ConnectedPool中释放,
   * 而不是在交互过程中释放 */
  INlsRequest *request;
  bool isAbnormal;
  /* 需要从Pool中删除, 并反初始化此request */
  bool shouldRelease;
  /* 需要从PrestartedNodePool中删除, 重新建连加入PreconectedNodePool中 */
  bool shouldPreconnect;
  /* socketFd & sslHandle 当前所属的request */
  INlsRequest *curRequest;
  /* push完后还未finish, 标记此curRequest处于无效状态 */
  bool curRequestInvalid;
  std::string sdkName;
  std::string startedResponse;

  void clearNode() {
    status = PreNodeToBeCreated;
    startTimestamp = 0;
    workableTimestamp = 0;
    tokenExpirationTimestamp = 0;
    socketFd = INVALID_SOCKET;
    sslHandle = NULL;
    canPick = false;
    ttsVersion = 0;
    request = NULL;
    isAbnormal = false;
    shouldRelease = false;
    shouldPreconnect = false;
    curRequest = NULL;
    curRequestInvalid = false;
    sdkName.clear();
    startedResponse.clear();
  }
};

struct ConnectedPoolProcess {
 public:
  explicit ConnectedPoolProcess() : type(TypeRealTime), work(false){};
  ~ConnectedPoolProcess() { work = false; };

  NlsType type;
  /* 此ConnectedPoolProcess开始工作的标记 */
  bool work;
  std::list<int> prestartedIndexList;
  std::list<int> preconnectedIndexList;
  std::vector<struct ConnectedNodeProcess> prestartedRequests;
  std::vector<struct ConnectedNodeProcess> preconnectedRequests;
};

class ConnectedPool {
 public:
  /**
   * @brief 预连接池将会每1秒检查每个节点
   * @param maxNumber 预连接池中每类交互的最大预连接的数量和最大正在交互的数量
   * @param timeoutMs 连接到交互服务器但是未启动交互的预连接超时时间
   * @param requestedTimeoutMs 连接到交互服务器且启动交互的预连接超时时间
   */
  ConnectedPool(unsigned int maxNumber, unsigned int timeoutMs,
                unsigned int requestedTimeoutMs);
  ~ConnectedPool();

#ifdef _MSC_VER
  static unsigned __stdcall loopConnectedPoolEventCallback(LPVOID arg);
#else
  static void *loopConnectedPoolEventCallback(void *arg);
#endif
  static void connectPoolEventCallback(evutil_socket_t socketFd, short event,
                                       void *arg);

  static void nodeReleaseEventCallback(evutil_socket_t socketFd, short event,
                                       void *arg);

  /**
   * @brief 启动预连接池的工作线程
   * @return 成功获取则Success
   */
  int startConnectedPool();
  /**
   * @brief 结束预连接池的工作线程
   * @return 成功获取则Success
   */
  int stopConnectedPool();
  /**
   * @brief 取走一个预连接, 并设置到INlsRequest中. 取走started
   * @return 成功获取则true, 没有可用预连接则false
   */
  bool popPrestartedNode(INlsRequest *request, NlsType type);
  /**
   * @brief 取走一个预连接, 并设置到INlsRequest中. 取走connected
   * @return 成功获取则true, 没有可用预连接则false
   */
  bool popPreconnectedNode(INlsRequest *request, NlsType type);
  /**
   * @brief 将连接的网络连接相关的节点进行存储, 存储的SSL暂时不可用
   * @return 成功存储节点则true, 否则false
   */
  bool pushPreconnectedNode(INlsRequest *request, NlsType type,
                            bool newNode = false);
  /**
   * @brief 将完成建连的交互连接相关的节点进行存储, 存储的SSL暂时不可用
   * @return 成功存储节点则true, 否则false
   */
  bool pushPrestartedNode(INlsRequest *request, NlsType type,
                          bool newNode = false);
  /**
   * @brief PreconnectedNode从preconnected池子移动到prestarted池子
   * @return 成功存储节点则true, 否则false
   */
  bool pushPrestartedNodeFromPreconnected(INlsRequest *request, NlsType type);

  /**
   * @brief 此request使用的SSL是否是ConnectedPool中的有效SSL
   */
  bool sslBelongToPool(INlsRequest *request, NlsType type,
                       bool &oriRequestIsAbnormal, bool &requestInPool);

  /**
   * @brief 此request使用的SSL在ConnectedPool中标记为异常, 后续需要删除
   */
  void curRequestIsAbnormal(INlsRequest *request, NlsType type);

  /**
   * @brief 使存储的SSL变可用可取走
   */
  void finishPushPreNode(NlsType type, evutil_socket_t curSocketFd,
                         SSLconnect *curSslHandle, int index,
                         INlsRequest *request);

  /**
   * @brief 此request是否存储在ConnectedPool中
   */
  bool requestInPool(INlsRequest *request, NlsType type);

  /**
   * @brief 已经释放的request需要从Pool中删除
   */
  bool deletePreNodeByRequest(INlsRequest *request, NlsType type);

  /**
   * @brief 从Pool中找到SSL所属request并删除
   */
  bool deletePreNodeBySSL(SSLconnect *curSslHandle, NlsType type);

 private:
  int getNumberOfThisTypeNodes(NlsType type, int &prestarted,
                               int &preconnected);
  int getNumberOfPreconnectedNodes(NlsType type);
  int getNumberOfPrestartedNodes(NlsType type);
  int initThisNodesPool(NlsType type);
  bool popOnePreconnectedNode(INlsRequest *request, NlsType type);
  bool popOnePrestartedNode(INlsRequest *request, NlsType type);
  void deletePreNode(std::vector<struct ConnectedNodeProcess> *pool);
  int timeoutPrestartedNode(std::vector<struct ConnectedNodeProcess> *pool);
  int timeoutPreconnectedNode(std::vector<struct ConnectedNodeProcess> *pool);
  void deleteOrPreconnectNodeShouldReleased(
      std::vector<struct ConnectedNodeProcess> *pool, std::string name);
  void preconnectNodeByRequest(INlsRequest *request);
  void showEveryNode(std::vector<struct ConnectedNodeProcess> *pool,
                     std::string name);
  std::string getStatusStr(ConnectedStatus status);
  void insertListInOrder(std::list<int> &lst, int a);
  void removeElement(std::list<int> &lst, int a);
  int popListFront(std::list<int> &lst);

  unsigned int _maxPreconnectedNumber;
  unsigned int _preconnectedTimeoutMs;
  unsigned int _prerequestedTimeoutMs;

#ifdef _MSC_VER
  unsigned _poolWorkThreadId;
  HANDLE _poolWorkThreadHandle;
#else
  pthread_t _poolWorkThreadId;
#endif

  struct event_base *_poolWorkBase;
  struct event *_connectPoolEvent;
  struct timeval _connectPoolTimerTv;  // 默认每1秒检查下所有节点
  bool _connectPoolTimerFlag;
  struct event *_nodeReleaseEvent;

  struct ConnectedPoolProcess _fssRequests;
  struct ConnectedPoolProcess _srRequests;
  struct ConnectedPoolProcess _stRequests;
  struct ConnectedPoolProcess _syRequests;

#if defined(_MSC_VER)
  HANDLE _lock;
#else
  pthread_mutex_t _lock;
#endif
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_CONNECTED_POOL_H
