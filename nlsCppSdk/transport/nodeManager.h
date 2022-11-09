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
#ifndef NLS_SDK_NODE_MANAGER_H
#define NLS_SDK_NODE_MANAGER_H

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <map>

namespace AlibabaNls {

enum NodeStatus {
  NodeStatusInvalid = 0,
  NodeStatusCreated,
  NodeStatusConnecting,
  NodeStatusConnected,
  NodeStatusHandshaking,
  NodeStatusHandshaked,
  NodeStatusRunning,
  NodeStatusCancelling,
  NodeStatusClosed,
  NodeStatusReleased,
};

typedef struct {
  void* request;
  void* node;
  void* instance;
  int status;
} NodeInfo;

class NlsNodeManager {
 public:
  NlsNodeManager();
  virtual ~NlsNodeManager();

  int addRequestIntoInfoWithInstance(void* request, void* instance);
  int checkRequestWithInstance(void* request, void* instance);
  int removeInstanceFromInfo(void* instance);
  int removeRequestFromInfo(void* request, bool wait);
  int removeNodeFromInfo(void* node, bool wait);

  int checkRequestExist(void* request, int* status);
  int checkNodeExist(void* node, int* status);
  int updateNodeStatus(void* node, int status);
  std::string getNodeStatusString(int status);

#ifdef _MSC_VER
  HANDLE _mtxNodeManager;
#else
  pthread_mutex_t _mtxNodeManager;
#endif

  std::map<void*, void*> requestListByNode;
  std::map<void*, NodeInfo> infoByRequest;
  int timeout_ms;
};

} // namespace AlibabaNls

#endif // NLS_SDK_NODE_MANAGER_H