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

#ifndef NLS_SDK_WORK_THREAD_H
#define NLS_SDK_WORK_THREAD_H

#include <list>
#include <queue>

#include "event.h"
#include "event2/util.h"
#include "event2/dns.h"

namespace AlibabaNls {

class ConnectNode;
class INlsRequest;

class WorkThread {
 public:
  WorkThread();
  virtual ~WorkThread();

  static void notifyEventCallback(evutil_socket_t fd, short which, void *arg);
  static void connectEventCallback(evutil_socket_t socketFd,
                                   short event, void *arg);
  static void readEventCallBack(evutil_socket_t socketFd, short what, void *arg);
  static void writeEventCallBack(evutil_socket_t socketFd, short what, void *arg);
  static void dnsEventCallback(int errorCode,
                               struct evutil_addrinfo *address,
                               void *arg);
#ifdef _MSC_VER
  static unsigned __stdcall loopEventCallback(LPVOID arg);
#else
  static void* loopEventCallback(void* arg);
#endif

  static void destroyConnectNode(ConnectNode* node);
  static int nodeRequestProcess(ConnectNode* node);
  static int nodeResponseProcess(ConnectNode* node);

  static void insertQueueNode(WorkThread* thread, INlsRequest * request);
  static INlsRequest* getQueueNode(WorkThread* thread);
  static void insertListNode(WorkThread* thread, INlsRequest * request);
  static void freeListNode(WorkThread* thread, INlsRequest * request);

#ifdef _MSC_VER
  HANDLE _mtxList;
  HANDLE _workThreadHandle;
  unsigned _workThreadId;
#else
  pthread_t _workThreadId;
  pthread_mutex_t _mtxList;
  static pthread_mutex_t _mtxCpu;
#endif

  static int _cpuNumber;
  static int _cpuCurrent;

  struct event_base * _workBase;
  struct evdns_base *_dnsBase;
  struct event _notifyEvent;
  evutil_socket_t _notifyReceiveFd;
  evutil_socket_t _notifySendFd;

  std::queue<INlsRequest*> _nodeQueue;
  std::list<INlsRequest*> _nodeList;

 private:

};

}

#endif /*NLS_SDK_WORK_THREAD_H*/
