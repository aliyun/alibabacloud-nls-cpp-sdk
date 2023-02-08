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
#include "nlsGlobal.h"
#include "connectNode.h"
#include "iNlsRequest.h"
#include "nlog.h"
#include "nodeManager.h"

namespace AlibabaNls {

NlsNodeManager::NlsNodeManager() {
#if defined(_MSC_VER)
  _mtxNodeManager = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxNodeManager, NULL);
#endif

  timeout_ms = 2000;
}
NlsNodeManager::~NlsNodeManager() {
#if defined(_MSC_VER)
  CloseHandle(_mtxNodeManager);
#else
  pthread_mutex_destroy(&_mtxNodeManager);
#endif
}

int NlsNodeManager::addRequestIntoInfoWithInstance(void* request, void* instance) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNodeManager, INFINITE);
#else
  pthread_mutex_lock(&_mtxNodeManager);
#endif

  if (instance == NULL) {
    LOG_ERROR("instance is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(EventClientEmpty);
  }
  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty); 
  }

  INlsRequest* nls_request = (INlsRequest*)request;
  ConnectNode* node = (ConnectNode*)nls_request->getConnectNode();
  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(NodeEmpty); 
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->infoByRequest.find(request);
  if (iter != this->infoByRequest.end()) {
    NodeInfo &info = iter->second;
    LOG_WARN("request:%p has added in NodeInfo, status:%s node:%p",
        request, this->getNodeStatusString(info.status).c_str(), info.node);
    if (info.status > NodeStatusCreated && info.status < NodeStatusReleased) {
      LOG_ERROR("request:%p is conflicted in NodeInfo, status:%s node:%p",
          info.request, this->getNodeStatusString(info.status).c_str(), info.node);
      #if defined(_MSC_VER)
      ReleaseMutex(_mtxNodeManager);
      #else
      pthread_mutex_unlock(&_mtxNodeManager);
      #endif

      return -(InvalidRequest);
    } else {
      if (info.instance != instance) {
        LOG_ERROR("the request:%p of instance(%p) isnot in instance(%p)",
            info.request, instance, info.instance);
        #if defined(_MSC_VER)
        ReleaseMutex(_mtxNodeManager);
        #else
        pthread_mutex_unlock(&_mtxNodeManager);
        #endif

        return -(InvalidRequest);
      }

      LOG_WARN("request:%p cover old request in NodeInfo, status:%s node:%p",
          info.request, this->getNodeStatusString(info.status).c_str(), info.node);
      info.request = request;
      info.node = node;
      info.instance = instance;
      info.status = NodeStatusCreated;
      this->requestListByNode[node] = request;
    }
  } else {
    NodeInfo info;
    info.request = request;
    info.node = node;
    info.instance = instance;
    info.status = NodeStatusCreated;
    this->infoByRequest.insert(std::make_pair(request, info));
    this->requestListByNode.insert(std::make_pair(node, request));
    LOG_DEBUG("add request(%p) node(%p) into NodeInfo", request, node);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNodeManager);
#else
  pthread_mutex_unlock(&_mtxNodeManager);
#endif

  return Success;
}

int NlsNodeManager::checkRequestWithInstance(void* request, void* instance) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNodeManager, INFINITE);
#else
  pthread_mutex_lock(&_mtxNodeManager);
#endif

  if (instance == NULL) {
    LOG_ERROR("instance is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(EventClientEmpty);
  }
  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty); 
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->infoByRequest.find(request);
  if (iter != this->infoByRequest.end()) {
    NodeInfo &info = iter->second;
    if (info.instance != instance) {
      LOG_ERROR("the request:%p of instance(%p) isnot in instance(%p)",
          info.request, instance, info.instance);
      #if defined(_MSC_VER)
      ReleaseMutex(_mtxNodeManager);
      #else
      pthread_mutex_unlock(&_mtxNodeManager);
      #endif

      return -(InvalidRequest);
    }
  } else {
    LOG_ERROR("request:%p isnot in NodeInfo", request);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(InvalidRequest);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNodeManager);
#else
  pthread_mutex_unlock(&_mtxNodeManager);
#endif
  return Success;
}

int NlsNodeManager::removeRequestFromInfo(void* request, bool wait) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNodeManager, INFINITE);
#else
  pthread_mutex_lock(&_mtxNodeManager);
#endif

  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty); 
  }

  int timeout = 0;
  std::map<void*, NodeInfo>::iterator iter;
  while (wait) {
    iter = this->infoByRequest.find(request);
    if (iter != this->infoByRequest.end()) {
      NodeInfo &info = iter->second;
      if (info.status != NodeStatusCreated && info.status < NodeStatusClosed) {
        if (timeout >= this->timeout_ms) {
          LOG_WARN("request(%p) node(%p) status(%s) is invalid, wait timeout(%dms), remove it by force.",
              request, info.node, this->getNodeStatusString(info.status).c_str(), timeout);
          break;
        }

        LOG_DEBUG("request(%p) node(%p) status(%s) is waiting timeout(%dms)...",
            request, info.node, this->getNodeStatusString(info.status).c_str(), timeout);

        #if defined(_MSC_VER)
        ReleaseMutex(_mtxNodeManager);
        Sleep(500);
        #else
        pthread_mutex_unlock(&_mtxNodeManager);
        usleep(500 * 1000);
        #endif

        timeout += 500;

        #if defined(_MSC_VER)
        WaitForSingleObject(_mtxNodeManager, INFINITE);
        #else
        pthread_mutex_lock(&_mtxNodeManager);
        #endif
      } else {
        break;
      }
    } else {
      break;
    }
  } // while

  iter = this->infoByRequest.find(request);
  if (iter != this->infoByRequest.end()) {
    NodeInfo &info = iter->second;
    void* node = info.node;
    std::map<void*, void*>::iterator iter2;
    iter2 = this->requestListByNode.find(node);
    if (iter2 != this->requestListByNode.end()) {
      this->requestListByNode.erase(iter2);
    }

    LOG_INFO("request(%p) node(%p) status(%s) removed.",
        request, info.node, this->getNodeStatusString(info.status).c_str());

    infoByRequest.erase(iter);
  } else {
    LOG_ERROR("request:%p isnot in NodeInfo", request);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(InvalidRequest);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNodeManager);
#else
  pthread_mutex_unlock(&_mtxNodeManager);
#endif
  return Success;
}

int NlsNodeManager::removeNodeFromInfo(void* node, bool wait) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNodeManager, INFINITE);
#else
  pthread_mutex_lock(&_mtxNodeManager);
#endif

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(NodeEmpty);
  }

  std::map<void*, void*>::iterator iter0;
  iter0 = this->requestListByNode.find(node);
  if (iter0 == this->requestListByNode.end()) {
    LOG_ERROR("node(%p) isn't in NodeInfo.", node);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif
    return -(NodeEmpty);
  }

  ConnectNode* connect_node = (ConnectNode*)node;
  INlsRequest* request = (INlsRequest*)connect_node->_request;
  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty);
  }

  int timeout = 0;
  std::map<void*, NodeInfo>::iterator iter;
  do {
    iter = this->infoByRequest.find(request);
    if (iter != this->infoByRequest.end()) {
      NodeInfo &info = iter->second;
      if (info.status != NodeStatusCreated && info.status < NodeStatusClosed) {
        if (timeout >= this->timeout_ms) {
          LOG_WARN("request(%p) node(%p) status(%s) is invalid, wait timeout(%dms), remove it by force.",
              request, info.node, this->getNodeStatusString(info.status).c_str(), timeout);
          break;
        }
        #if defined(_MSC_VER)
        ReleaseMutex(_mtxNodeManager);
        Sleep(500);
        #else
        pthread_mutex_unlock(&_mtxNodeManager);
        usleep(500 * 1000);
        #endif

        timeout += 500;

        #if defined(_MSC_VER)
        WaitForSingleObject(_mtxNodeManager, INFINITE);
        #else
        pthread_mutex_lock(&_mtxNodeManager);
        #endif

        LOG_DEBUG("request(%p) node(%p) status(%s) is waiting timeout(%dms)...",
            request, info.node, this->getNodeStatusString(info.status).c_str(), timeout);
      } else {
        break;
      }
    } else {
      break;
    }
  } while (wait);

  iter = this->infoByRequest.find(request);
  if (iter != this->infoByRequest.end()) {
    NodeInfo &info = iter->second;    
    void* node = info.node;
    std::map<void*, void*>::iterator iter2;
    iter2 = this->requestListByNode.find(node);
    if (iter2 != this->requestListByNode.end()) {
      this->requestListByNode.erase(iter2);
    }

    // LOG_DEBUG("request(%p) node(%p) status(%s) removed.",
    //     request, info.node, this->getNodeStatusString(info.status).c_str());

    infoByRequest.erase(iter);
  } else {
    LOG_ERROR("request:%p isnot in NodeInfo", request);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(InvalidRequest);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNodeManager);
#else
  pthread_mutex_unlock(&_mtxNodeManager);
#endif
  return Success;
}

int NlsNodeManager::removeInstanceFromInfo(void* instance) {
  return Success;
}

int NlsNodeManager::checkRequestExist(void* request, int* status) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNodeManager, INFINITE);
#else
  pthread_mutex_lock(&_mtxNodeManager);
#endif

  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty); 
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->infoByRequest.find(request);
  if (iter != this->infoByRequest.end()) {
    NodeInfo &info = iter->second;
    if (info.request != request) {
      LOG_ERROR("the request:%p mismatch the request:%p in NodeInfo",
          request, info.request);
      #if defined(_MSC_VER)
      ReleaseMutex(_mtxNodeManager);
      #else
      pthread_mutex_unlock(&_mtxNodeManager);
      #endif

      return -(InvalidRequest);
    }
    *status = info.status;
  } else {
    LOG_ERROR("Request:%p isn't in NodeInfo", request);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNodeManager);
#else
  pthread_mutex_unlock(&_mtxNodeManager);
#endif
  return Success;
}

int NlsNodeManager::checkNodeExist(void* node, int* status) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNodeManager, INFINITE);
#else
  pthread_mutex_lock(&_mtxNodeManager);
#endif

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(NodeEmpty); 
  }

  std::map<void*, void*>::iterator iter0;
  iter0 = this->requestListByNode.find(node);
  if (iter0 == this->requestListByNode.end()) {
    LOG_ERROR("node(%p) isn't in NodeInfo.", node);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif
    return -(NodeEmpty);
  }

  ConnectNode* connect_node = (ConnectNode*)node;
  INlsRequest* request = (INlsRequest*)connect_node->_request;
  if (request == NULL) {
    LOG_ERROR("request is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty); 
  }

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->infoByRequest.find(request);
  if (iter != this->infoByRequest.end()) {
    NodeInfo &info = iter->second;
    if (info.request != request) {
      LOG_ERROR("the request:%p mismatch the request:%p in NodeInfo",
          request, info.request);
      #if defined(_MSC_VER)
      ReleaseMutex(_mtxNodeManager);
      #else
      pthread_mutex_unlock(&_mtxNodeManager);
      #endif

      return -(InvalidRequest);
    }
    *status = info.status;
  } else {
    LOG_ERROR("Request:%p isnot in NodeInfo", request);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(RequestEmpty);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNodeManager);
#else
  pthread_mutex_unlock(&_mtxNodeManager);
#endif
  return Success;
}

int NlsNodeManager::updateNodeStatus(void* node, int status) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtxNodeManager, INFINITE);
#else
  pthread_mutex_lock(&_mtxNodeManager);
#endif

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif

    return -(NodeEmpty); 
  }

  std::map<void*, void*>::iterator iter0;
  iter0 = this->requestListByNode.find(node);
  if (iter0 == this->requestListByNode.end()) {
    LOG_ERROR("node(%p) isn't in NodeInfo.", node);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif
    return -(NodeEmpty);
  }

  ConnectNode* connect_node = (ConnectNode*)node;
  INlsRequest* request = (INlsRequest*)connect_node->_request;

  std::map<void*, NodeInfo>::iterator iter;
  iter = this->infoByRequest.find(request);
  if (iter != this->infoByRequest.end()) {
    NodeInfo &info = iter->second;
    LOG_DEBUG("Node:%p set node status from %s to %s.",
        info.node,
        this->getNodeStatusString(info.status).c_str(),
        this->getNodeStatusString(status).c_str());
    info.status = status;
  } else {
    LOG_ERROR("Request:%p isn't in NodeInfo", request);
    #if defined(_MSC_VER)
    ReleaseMutex(_mtxNodeManager);
    #else
    pthread_mutex_unlock(&_mtxNodeManager);
    #endif
    
    return -(InvaildNodeStatus);
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtxNodeManager);
#else
  pthread_mutex_unlock(&_mtxNodeManager);
#endif
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

} // namespace AlibabaNls
