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
#include <map>
#include <queue>
#include <vector>

#include "event.h"
#include "event2/dns.h"
#include "event2/util.h"
#include "nlsClientImpl.h"

namespace AlibabaNls {

#ifdef ENABLE_DNS_IP_CACHE
struct DnsIpCache {
 public:
  explicit DnsIpCache() { same_ip_count = 0; };
  enum DnsIpCacheConstValue {
    /* 此Host下连续WorkThreshold个IP相同, 则开始从IpCache中获得IP*/
    WorkThreshold = 20,
  };
  std::vector<std::string> ip_list;
  uint32_t same_ip_count;
};
#endif

class ConnectNode;
class INlsRequest;
class WorkThread {
 public:
  WorkThread();
  virtual ~WorkThread();

  static void launchEventCallback(evutil_socket_t fd, short which, void *arg);
  static void connectEventCallback(evutil_socket_t socketFd, short event,
                                   void *arg);
#ifdef ENABLE_HIGH_EFFICIENCY
  static void connectTimerEventCallback(evutil_socket_t socketFd, short event,
                                        void *arg);
#endif
  static void readEventCallBack(evutil_socket_t socketFd, short what,
                                void *arg);
  static void writeEventCallBack(evutil_socket_t socketFd, short what,
                                 void *arg);
#ifdef __LINUX__
  static void sysDnsEventCallback(evutil_socket_t socketFd, short what,
                                  void *arg);
#endif
  static void dnsEventCallback(int errorCode, struct evutil_addrinfo *address,
                               void *arg);
  static void directConnect(void *arg, char *ip);
#ifdef _MSC_VER
  static unsigned __stdcall loopEventCallback(LPVOID arg);
#else
  static void *loopEventCallback(void *arg);
#endif

  static void destroyConnectNode(ConnectNode *node);
  static int nodeRequestProcess(ConnectNode *node);
  static int nodeResponseProcess(ConnectNode *node);

  static void insertListNode(WorkThread *thread, INlsRequest *request);
  static bool freeListNode(WorkThread *thread, INlsRequest *request);

  static void setInstance(NlsClientImpl *instance);

  void setUseSysGetAddrInfo(bool enable);
  void setDirectHost(char *ip);
  void setAddrInFamily(int aiFamily);
#ifdef ENABLE_DNS_IP_CACHE
  std::string getIpFromCache(char *host);
  void setIpCache(char *host, char *ip);
#endif
  void updateParameters(ConnectNode *node);

#ifdef _MSC_VER
  unsigned _workThreadId;
  HANDLE _mtxList;
  HANDLE _workThreadHandle;
  static HANDLE _mtxCpu;
#else
  pthread_t _workThreadId;
  pthread_mutex_t _mtxList;
  static pthread_mutex_t _mtxCpu;
#endif

  struct event_base *_workBase;
  struct evdns_base *_dnsBase;

  std::list<INlsRequest *> _nodeList;

 private:
  static NlsClientImpl *_instance;
  int _addrInFamily;
  char _directIp[64];
#ifdef ENABLE_DNS_IP_CACHE
  std::map<std::string, struct DnsIpCache> _dnsIpCache;
#endif
  bool _enableSysGetAddr;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_WORK_THREAD_H
