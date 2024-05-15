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

#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <time.h>

#include "connectNode.h"
#include "iNlsRequest.h"
#include "nlog.h"
#include "nlsGlobal.h"
#include "nodeManager.h"
#include "utility.h"

namespace AlibabaNls {

NlsNodeManager::NlsNodeManager() : _timeout_ms(DefaultRemoveTimeout) {
#if defined(_MSC_VER)
  _mtxNodeManager = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxNodeManager, NULL);
#endif
}

NlsNodeManager::~NlsNodeManager() {
#if defined(_MSC_VER)
  CloseHandle(_mtxNodeManager);
#else
  pthread_mutex_destroy(&_mtxNodeManager);
#endif
}

/**
 * @brief: 新创建request加入到NodeManager中
 * @return:
 */
int NlsNodeManager::addRequestIntoInfoWithInstance(void* request,
                                                   void* instance) {
  MUTEX_LOCK(_mtxNodeManager);

  if (instance == NULL) {
    LOG_ERROR("instance is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(EventClientEmpty);
  }
  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(RequestEmpty);
  }

  INlsRequest* nls_request = static_cast<INlsRequest*>(request);
  ConnectNode* node = static_cast<ConnectNode*>(nls_request->getConnectNode());
  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(NodeEmpty);
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->_infoByRequest.find(request);
  if (iter != this->_infoByRequest.end()) {
    NodeInfo& info = iter->second;
    LOG_WARN("request:%p has added in NodeInfo, status:%s node:%p", request,
             this->getNodeStatusString(info.status).c_str(), info.node);
    if (info.status > NodeStatusCreated && info.status < NodeStatusReleased) {
      LOG_ERROR("request:%p is conflicted in NodeInfo, status:%s node:%p",
                info.request, this->getNodeStatusString(info.status).c_str(),
                info.node);
      MUTEX_UNLOCK(_mtxNodeManager);
      return -(InvalidRequest);
    } else {
      if (info.instance != instance) {
        LOG_ERROR("the request:%p of instance(%p) isnot in instance(%p)",
                  info.request, instance, info.instance);
        MUTEX_UNLOCK(_mtxNodeManager);
        return -(InvalidRequest);
      }

      LOG_WARN("request:%p cover old request in NodeInfo, status:%s node:%p",
               info.request, this->getNodeStatusString(info.status).c_str(),
               info.node);
      info.request = request;
      info.node = node;
      info.instance = instance;
      info.uuid = node->getNodeUUID();
      info.status = NodeStatusCreated;
      this->_requestListByNode[node] = request;
    }
  } else {
    NodeInfo info;
    info.request = request;
    info.node = node;
    info.instance = instance;
    info.uuid = node->getNodeUUID();
    info.status = NodeStatusCreated;
    this->_infoByRequest.insert(std::make_pair(request, info));
    this->_requestListByNode.insert(std::make_pair(node, request));
    LOG_DEBUG("add request(%p) node(%p) into NodeInfo", request, node);
  }

  MUTEX_UNLOCK(_mtxNodeManager);
  return Success;
}

/**
 * @brief: 检查request是否属于此instance
 * @return: Error Code
 */
int NlsNodeManager::checkRequestWithInstance(void* request, void* instance) {
  MUTEX_LOCK(_mtxNodeManager);

  if (instance == NULL) {
    LOG_ERROR("instance is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(EventClientEmpty);
  }
  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(RequestEmpty);
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->_infoByRequest.find(request);
  if (iter != this->_infoByRequest.end()) {
    NodeInfo& info = iter->second;
    if (info.instance != instance) {
      LOG_ERROR("the request:%p of instance(%p) isnot in instance(%p)",
                info.request, instance, info.instance);
      MUTEX_UNLOCK(_mtxNodeManager);
      return -(InvalidRequest);
    } else {
      INlsRequest* nls_request = static_cast<INlsRequest*>(request);
      ConnectNode* node =
          static_cast<ConnectNode*>(nls_request->getConnectNode());
      std::map<void*, void*>::iterator node_iter;
      node_iter = this->_requestListByNode.find(node);
      if (node_iter == this->_requestListByNode.end()) {
        LOG_ERROR("node(%p) isn't in NodeInfo, request(%p) is invalid.", node,
                  request);
        MUTEX_UNLOCK(_mtxNodeManager);
        return -(InvalidRequest);
      } else {
        std::string uuid = node->getNodeUUID();
        if (info.uuid != uuid) {
          LOG_ERROR("the uuid(%s) isnot in node(%p), request(%p) is invalid.",
                    uuid.c_str(), node, request);
          MUTEX_UNLOCK(_mtxNodeManager);
          return -(InvalidRequest);
        }
      }
    }
  } else {
    LOG_ERROR("request:%p isnot in NodeInfo", request);
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(InvalidRequest);
  }

  MUTEX_UNLOCK(_mtxNodeManager);
  return Success;
}

/**
 * @brief: request释放后从NodeManager中删除此request
 * @return:
 */
int NlsNodeManager::removeRequestFromInfo(void* request, bool wait) {
  MUTEX_LOCK(_mtxNodeManager);

  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(RequestEmpty);
  }

  int timeout = 0;
  std::map<void*, NodeInfo>::iterator iter;
  while (wait) {
    iter = this->_infoByRequest.find(request);
    if (iter != this->_infoByRequest.end()) {
      NodeInfo& info = iter->second;
      if (info.status != NodeStatusCreated && info.status < NodeStatusClosed) {
        if (timeout >= this->_timeout_ms) {
          LOG_WARN(
              "request(%p) node(%p) status(%s) is invalid, wait timeout(%dms), "
              "remove it by force.",
              request, info.node,
              this->getNodeStatusString(info.status).c_str(), timeout);
          break;
        }

        LOG_DEBUG("request(%p) node(%p) status(%s) is waiting timeout(%dms)...",
                  request, info.node,
                  this->getNodeStatusString(info.status).c_str(), timeout);

        MUTEX_UNLOCK(_mtxNodeManager);
#if defined(_MSC_VER)
        Sleep(StepSleepMs);
#else
        usleep(StepSleepMs * 1000);
#endif
        timeout += StepSleepMs;
        MUTEX_LOCK(_mtxNodeManager);
      } else {
        break;
      }
    } else {
      break;
    }
  }  // while

  iter = this->_infoByRequest.find(request);
  if (iter != this->_infoByRequest.end()) {
    NodeInfo& info = iter->second;
    void* node = info.node;
    std::map<void*, void*>::iterator iter2;
    iter2 = this->_requestListByNode.find(node);
    if (iter2 != this->_requestListByNode.end()) {
      this->_requestListByNode.erase(iter2);
    }

    LOG_INFO("request(%p) node(%p) status(%s) removed.", request, info.node,
             this->getNodeStatusString(info.status).c_str());

    _infoByRequest.erase(iter);
  } else {
    LOG_ERROR("request:%p isnot in NodeInfo", request);
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(InvalidRequest);
  }

  MUTEX_UNLOCK(_mtxNodeManager);
  return Success;
}

int NlsNodeManager::removeInstanceFromInfo(void* instance) { return Success; }

int NlsNodeManager::checkRequestExist(void* request, int* status) {
  MUTEX_LOCK(_mtxNodeManager);

  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(RequestEmpty);
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->_infoByRequest.find(request);
  if (iter != this->_infoByRequest.end()) {
    NodeInfo& info = iter->second;
    if (info.request != request) {
      LOG_ERROR("the request:%p mismatch the request:%p in NodeInfo", request,
                info.request);
      MUTEX_UNLOCK(_mtxNodeManager);
      return -(InvalidRequest);
    } else {
      INlsRequest* nls_request = static_cast<INlsRequest*>(request);
      ConnectNode* node =
          static_cast<ConnectNode*>(nls_request->getConnectNode());
      std::map<void*, void*>::iterator node_iter;
      node_iter = this->_requestListByNode.find(node);
      if (node_iter == this->_requestListByNode.end()) {
        LOG_ERROR("node(%p) isn't in NodeInfo, request(%p) is invalid.", node,
                  request);
        MUTEX_UNLOCK(_mtxNodeManager);
        return -(InvalidRequest);
      } else {
        std::string uuid = node->getNodeUUID();
        if (info.uuid != uuid) {
          LOG_ERROR("the uuid(%s) isnot in node(%p), request(%p) is invalid.",
                    uuid.c_str(), node, request);
          MUTEX_UNLOCK(_mtxNodeManager);
          return -(InvalidRequest);
        }
      }
    }
    *status = info.status;
  } else {
    LOG_ERROR("Request:%p isn't in NodeInfo", request);
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(RequestEmpty);
  }

  MUTEX_UNLOCK(_mtxNodeManager);
  return Success;
}

int NlsNodeManager::checkNodeExist(void* node, int* status) {
  MUTEX_LOCK(_mtxNodeManager);

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(NodeEmpty);
  }

  std::map<void*, void*>::iterator iter0;
  iter0 = this->_requestListByNode.find(node);
  if (iter0 == this->_requestListByNode.end()) {
    LOG_ERROR("node(%p) isn't in NodeInfo.", node);
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(NodeEmpty);
  }

  ConnectNode* connect_node = static_cast<ConnectNode*>(node);
  INlsRequest* request = static_cast<INlsRequest*>(connect_node->getRequest());
  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(RequestEmpty);
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->_infoByRequest.find(request);
  if (iter != this->_infoByRequest.end()) {
    NodeInfo& info = iter->second;
    if (info.request != request) {
      LOG_ERROR("the request:%p mismatch the request:%p in NodeInfo", request,
                info.request);
      MUTEX_UNLOCK(_mtxNodeManager);
      return -(InvalidRequest);
    } else {
      std::string uuid = connect_node->getNodeUUID();
      if (info.uuid != uuid) {
        LOG_ERROR("the uuid(%s) isnot in node(%p), request(%p) is invalid.",
                  uuid.c_str(), connect_node, request);
        MUTEX_UNLOCK(_mtxNodeManager);
        return -(InvalidRequest);
      }
    }
    *status = info.status;
  } else {
    LOG_ERROR("Request:%p isnot in NodeInfo", request);
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(RequestEmpty);
  }

  MUTEX_UNLOCK(_mtxNodeManager);
  return Success;
}

int NlsNodeManager::updateNodeStatus(void* node, int status) {
  MUTEX_LOCK(_mtxNodeManager);

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(NodeEmpty);
  }

  std::map<void*, void*>::iterator iter0;
  iter0 = this->_requestListByNode.find(node);
  if (iter0 == this->_requestListByNode.end()) {
    LOG_ERROR("node(%p) isn't in NodeInfo.", node);
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(NodeEmpty);
  }

  ConnectNode* connect_node = static_cast<ConnectNode*>(node);
  INlsRequest* request = static_cast<INlsRequest*>(connect_node->getRequest());

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->_infoByRequest.find(request);
  if (iter != this->_infoByRequest.end()) {
    NodeInfo& info = iter->second;
    std::string uuid = connect_node->getNodeUUID();
    if (info.uuid != uuid) {
      LOG_ERROR("the uuid(%s) isnot in node(%p), request(%p) is invalid.",
                uuid.c_str(), connect_node, request);
      MUTEX_UNLOCK(_mtxNodeManager);
      return -(InvalidRequest);
    }

    LOG_DEBUG("Node:%p set node status from %s to %s.", info.node,
              this->getNodeStatusString(info.status).c_str(),
              this->getNodeStatusString(status).c_str());
    info.status = status;
  } else {
    LOG_ERROR("Request:%p isn't in NodeInfo", request);
    MUTEX_UNLOCK(_mtxNodeManager);
    return -(InvaildNodeStatus);
  }

  MUTEX_UNLOCK(_mtxNodeManager);
  return Success;
}

std::string NlsNodeManager::getNodeStatusString(int status) {
  std::string ret_str("Unknown");
  switch (status) {
    case NodeStatusInvalid:
      ret_str.assign("NodeStatusInvalid");
      break;
    case NodeStatusCreated:
      ret_str.assign("NodeStatusCreated");
      break;
    case NodeStatusInvoking:
      ret_str.assign("NodeStatusInvoking");
      break;
    case NodeStatusInvoked:
      ret_str.assign("NodeStatusInvoked");
      break;
    case NodeStatusConnecting:
      ret_str.assign("NodeStatusConnecting");
      break;
    case NodeStatusConnected:
      ret_str.assign("NodeStatusConnected");
      break;
    case NodeStatusHandshaking:
      ret_str.assign("NodeStatusHandshaking");
      break;
    case NodeStatusHandshaked:
      ret_str.assign("NodeStatusHandshaked");
      break;
    case NodeStatusRunning:
      ret_str.assign("NodeStatusRunning");
      break;
    case NodeStatusCancelling:
      ret_str.assign("NodeStatusCancelling");
      break;
    case NodeStatusClosing:
      ret_str.assign("NodeStatusClosing");
      break;
    case NodeStatusClosed:
      ret_str.assign("NodeStatusClosed");
      break;
    case NodeStatusReleased:
      ret_str.assign("NodeStatusReleased");
      break;
    default:
      ret_str.assign("Unknown");
      break;
  }

  return ret_str;
}

}  // namespace AlibabaNls
