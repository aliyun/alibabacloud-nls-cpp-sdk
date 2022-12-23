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


#include <math.h>
#include <signal.h>
#include <string>
#include <algorithm>
#ifdef _MSC_VER
#include <process.h>
#else
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sched.h>
#endif

#include "nlsGlobal.h"
#include "iNlsRequest.h"
#include "iNlsRequestParam.h"
#include "nodeManager.h"
#include "nlsClient.h"
#include "workThread.h"
#include "connectNode.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

#define HOST_SIZE 256

int WorkThread::_addrInFamily = AF_INET;
char WorkThread::_directIp[64] = {0};
bool WorkThread::_enableSysGetAddr = false;
NlsClient* WorkThread::_instance = NULL;

#if defined(_MSC_VER)
HANDLE WorkThread::_mtxCpu = NULL;
#else
pthread_mutex_t WorkThread::_mtxCpu = PTHREAD_MUTEX_INITIALIZER;
#endif


WorkThread::WorkThread() {
  LOG_DEBUG("Create WorkThread.");
#if defined(_MSC_VER)
  _mtxList = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxList, NULL);
#endif

  _workBase = event_base_new();
  if (NULL == _workBase) {
    LOG_ERROR("event_base_new failed.");
    exit(1);
  }

  _dnsBase = evdns_base_new(_workBase, 1);
  if (NULL == _dnsBase) {
    LOG_WARN("evdns_base_new failed.");
    // no need dnsBase if _directIp true
  }

  evutil_socket_t pair[2];
  if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == -1) {
    LOG_ERROR("evutil_socketpair failed.");
    exit(1);
  }

  _notifyReceiveFd = pair[0];
  _notifySendFd = pair[1];

  if (event_assign(&_notifyEvent,
                   _workBase,
                   _notifyReceiveFd,
                   EV_READ | EV_PERSIST,
                   notifyEventCallback,
                   (void *)this) == -1) {
    LOG_ERROR("event_assign failed.");
    exit(1);
  }

  if (event_add(&_notifyEvent, 0) == -1 ) {
    LOG_ERROR("event_add failed.");
    exit(1);
  }

#if defined(_MSC_VER)
  _workThreadId = 0;
  _workThreadHandle = (HANDLE)_beginthreadex(NULL, 0, loopEventCallback, (LPVOID)this, 0, &_workThreadId);
  CloseHandle(_workThreadHandle);
#else
  _workThreadId = 0;
  pthread_create(&_workThreadId, NULL, loopEventCallback, (void*)this);
  usleep(100 * 1000);
#endif
  LOG_DEBUG("WorkThread start working.");
}

WorkThread::~WorkThread() {
  size_t count = 0;
  int try_count = 500;

  LOG_DEBUG("Begin destroy WorkThread(%p), nodeList size:%d.",
      this, _nodeList.size());
  //must check asr is end
  do {
#ifdef _MSC_VER
    Sleep(10);
    WaitForSingleObject(_mtxList, INFINITE);
#else
    usleep(10 * 1000);
    pthread_mutex_lock(&_mtxList);
#endif
    NlsClient* client = _instance;
    NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
    int ret = Success;
    std::list<INlsRequest*>::iterator itList;
    for (itList = _nodeList.begin(); itList != _nodeList.end();) {
      INlsRequest* request = *itList;
      if (request == NULL) {
        _nodeList.erase(itList++);
        continue;
      }

      int status = NodeStatusInvalid;
      ret = node_manager->checkRequestExist(request, &status);
      if (ret != Success) {
        LOG_ERROR("Request:%p checkRequestExist failed, ret:%d", request, ret);
        _nodeList.erase(itList++);
        continue;
      }

      ConnectNode *node = request->getConnectNode();
      if (node == NULL) {
        LOG_ERROR("Node is nullptr, you have destroyed request or relesed instance!");
        _nodeList.erase(itList++);
        continue;
      }

      ConnectStatus cStatus = request->getConnectNode()->getConnectNodeStatus();
      ExitStatus eStatus = request->getConnectNode()->getExitStatus();
      LOG_DEBUG("Request:%p Node:%p cStatus:%s eStatus:%s",
          request, request->getConnectNode(),
          request->getConnectNode()->getConnectNodeStatusString().c_str(),
          request->getConnectNode()->getExitStatusString().c_str());

      if (cStatus == NodeInvalid || cStatus == NodeInitial ||
          eStatus == ExitStopped) {
        _nodeList.erase(itList++);
        node_manager->removeRequestFromInfo(request, false);
        delete request;
        request = NULL;
      } else {
        LOG_WARN("destroy WorkThread(%p) Node(%p) cStatus:%s, eStatus:%s",
            this, request->getConnectNode(),
            request->getConnectNode()->getConnectNodeStatusString().c_str(),
            request->getConnectNode()->getExitStatusString().c_str());
        itList++;
      }
    }  // for

    count = _nodeList.size();
    LOG_DEBUG("destroy WorkThread(%p) count:%d, try_count:%d.", this, count, try_count);

    if (try_count-- <= 0) {
      for (itList = _nodeList.begin(); itList != _nodeList.end();) {
        INlsRequest* request = *itList;
        _nodeList.erase(itList++);
        node_manager->removeRequestFromInfo(request, false);
        delete request;
        request = NULL;
      }  // for
    }

#if defined(_MSC_VER)
    ReleaseMutex(_mtxList);
#else
    pthread_mutex_unlock(&_mtxList);
#endif
  } while (count > 0 && try_count-- > 0);  // do while

  LOG_DEBUG("destroy WorkThread(%p) deleted all requests", this);

  evutil_closesocket(_notifySendFd);
  evutil_closesocket(_notifyReceiveFd);
  event_del(&_notifyEvent);
  event_base_loopbreak(_workBase);

#if defined(_MSC_VER)
  CloseHandle(_mtxList);
#else
  LOG_DEBUG("destroy WorkThread(%p) join _workThreadId:%d", this, _workThreadId);
  pthread_join(_workThreadId, NULL);
  pthread_mutex_destroy(&_mtxList);
#endif

  _instance = NULL;

  LOG_DEBUG("Destroy WorkThread(%p) done.", this);
}

int WorkThread::insertQueueNode(WorkThread* thread, INlsRequest * request) {
#if defined(_MSC_VER)
  WaitForSingleObject(thread->_mtxList, INFINITE);
#else
  pthread_mutex_lock(&(thread->_mtxList));
#endif

  int queue_size = thread->_nodeQueue.size();
  if (queue_size < 0) {
    LOG_ERROR("WorkThread(%p) _nodeQueue size:%d is invalid.", thread, queue_size);
    #if defined(_MSC_VER)
    ReleaseMutex(thread->_mtxList);
    #else
    pthread_mutex_unlock(&(thread->_mtxList));
    #endif
    return -(InvalidNodeQueue);
  }

  std::queue<INlsRequest*> queue(thread->_nodeQueue);
  int i = 0;
  for (i = 0; i < queue_size; i++) {
    INlsRequest* get = queue.front();
    if (get == request) {
      break;
    }
    queue.pop();
  }
  if (i == queue_size) {
    // cannot find request matched
    thread->_nodeQueue.push(request);
  } else {
  }

#if defined(_MSC_VER)
  ReleaseMutex(thread->_mtxList);
#else
  pthread_mutex_unlock(&(thread->_mtxList));
#endif
  return Success;
}

INlsRequest* WorkThread::getQueueNode(WorkThread* thread) {
#if defined(_MSC_VER)
  WaitForSingleObject(thread->_mtxList, INFINITE);
#else
  pthread_mutex_lock(&(thread->_mtxList));
#endif

  INlsRequest *request = thread->_nodeQueue.front();
  thread->_nodeQueue.pop();

#if defined(_MSC_VER)
  ReleaseMutex(thread->_mtxList);
#else
  pthread_mutex_unlock(&(thread->_mtxList));
#endif

  return request;
}

void WorkThread::insertListNode(WorkThread* thread, INlsRequest * request) {
#if defined(_MSC_VER)
  WaitForSingleObject(thread->_mtxList, INFINITE);
#else
  pthread_mutex_lock(&(thread->_mtxList));
#endif

  std::list<INlsRequest *>::iterator iLocation =
      find(thread->_nodeList.begin(), thread->_nodeList.end(), request);
  if (iLocation == thread->_nodeList.end()) {
    thread->_nodeList.push_back(request);
  } else {
  }

#if defined(_MSC_VER)
  ReleaseMutex(thread->_mtxList);
#else
  pthread_mutex_unlock(&(thread->_mtxList));
#endif

  return;
}

void WorkThread::freeListNode(WorkThread* thread, INlsRequest* request) {
#if defined(_MSC_VER)
  WaitForSingleObject(thread->_mtxList, INFINITE);
#else
  pthread_mutex_lock(&(thread->_mtxList));
#endif

  std::list<INlsRequest *>::iterator iLocation =
      find(thread->_nodeList.begin(), thread->_nodeList.end(), request);

  if (iLocation != thread->_nodeList.end()) {
    thread->_nodeList.remove(*iLocation);
  }

#if defined(_MSC_VER)
  ReleaseMutex(thread->_mtxList);
#else
  pthread_mutex_unlock(&(thread->_mtxList));
#endif
}

void WorkThread::destroyConnectNode(ConnectNode* node) {
#ifdef _MSC_VER
  WaitForSingleObject(_mtxCpu, INFINITE);
#else
  pthread_mutex_lock(&_mtxCpu);
#endif

  if (node == NULL) {
    LOG_ERROR("Input node is null.");
  #if defined(_MSC_VER)
    ReleaseMutex(_mtxCpu);
  #else
    pthread_mutex_unlock(&_mtxCpu);
  #endif
    return;
  }

  LOG_INFO("Node:%p destroyConnectNode begin.", node);

  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("Node:%p checkNodeExist failed, ret:%d", node);
    return;
  }
  node_manager->updateNodeStatus(node, NodeStatusReleased);

  freeListNode(node->_eventThread, node->_request);
  if (node->updateDestroyStatus()) {
    LOG_INFO("Node:%p destroyConnectNode done.", node);
    INlsRequest* request = node->_request;
    if (request) {
      node_manager->removeRequestFromInfo(request, false);
      delete request;
      request = NULL;
    }
  }

  LOG_INFO("Node:%p destroyConnectNode finish.", node);

#if defined(_MSC_VER)
  ReleaseMutex(_mtxCpu);
#else
  pthread_mutex_unlock(&_mtxCpu);
#endif

  return;
}

#if defined(_MSC_VER)
unsigned __stdcall WorkThread::loopEventCallback(LPVOID arg) {
#else
void* WorkThread::loopEventCallback(void* arg) {
#endif

  WorkThread *eventParam = (WorkThread*)arg;

#if defined(__ANDROID__) || defined (__linux__)
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

  prctl(PR_SET_NAME, "eventThread");
#endif

  LOG_DEBUG("workThread(%p) create loopEventCallback", arg);

  if (eventParam->_workBase) {
    event_base_dispatch(eventParam->_workBase);
  }
  if (eventParam->_dnsBase) {
    evdns_base_free(eventParam->_dnsBase, 0);
    eventParam->_dnsBase = NULL;
  }
  if (eventParam->_workBase) {
    event_base_free(eventParam->_workBase);
    eventParam->_workBase = NULL;
  }

#if defined(_MSC_VER)
  return 0;
#else
  return NULL;
#endif
}

#ifdef ENABLE_HIGH_EFFICIENCY
/*
 * Description:
 * Return:
 * Others:
 */
void WorkThread::connectTimerEventCallback(
    evutil_socket_t socketFd , short event, void *arg) {
  int errorCode = 0;
  ConnectNode *node = (ConnectNode*)arg;

  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN("Node:%p checkNodeExist falied, node is %s, do nothing later...",
          node, node_manager->getNodeStatusString(status).c_str());
      destroyConnectNode(node);
      return;
    }
  }

  // LOG_DEBUG("Node:%p connectTimerEventCallback node status:%s ...",
  //     node, node_manager->getNodeStatusString(status).c_str());

  if (event == EV_CLOSED) {
    LOG_DEBUG("Node:%p connect EV_CLOSED.", node);
    goto EventProcessFailed;
  } else {
    // event == EV_TIMEOUT
    if (node->getConnectNodeStatus() == NodeConnecting) {
      socklen_t len = sizeof(errorCode);
      getsockopt(socketFd, SOL_SOCKET, SO_ERROR, (char *) &errorCode, &len);
      if (!errorCode) {
        LOG_DEBUG("Node:%p connect return ev_write, check ok, set NodeStatus:NodeConnected.", node);
        node->setConnectNodeStatus(NodeConnected);
        node_manager->updateNodeStatus(node, NodeStatusConnected);

        #ifndef _MSC_VER
        // get client ip and port from socketFd
        struct sockaddr_in client;
        char client_ip[20];
        socklen_t client_len = sizeof(client);
        getsockname(socketFd, (struct sockaddr *)&client, &client_len);
        inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
        LOG_DEBUG("Node:%p local %s:%d", node, client_ip, ntohs(client.sin_port));
        #endif

        node->_isConnected = true;
      } else {
        if (node->socketConnect() < 0) {
          goto EventProcessFailed;
        }
      }
    }

    if (node->getConnectNodeStatus() == NodeConnected) {
      int ret = node->sslProcess();
      switch (ret) {
        case 0:
          LOG_DEBUG("Node:%p Begin gateway request process.", node);
          if (nodeRequestProcess(node) < 0) {
            destroyConnectNode(node);
          }
          break;
        case 1:
          //LOG_DEBUG("wait connect.");
          break;
        default:
          goto EventProcessFailed;
      }
    }
  }

  return;

EventProcessFailed:
  LOG_ERROR("Node:%p Connect failed:%s.",
      node,
      evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

  //node->closeConnectNode();
  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);
  node_manager->updateNodeStatus(node, NodeStatusConnecting);

  if (node->dnsProcess(_addrInFamily, _directIp, _enableSysGetAddr) < 0) {
    LOG_ERROR("Node:%p try delete request.", node);
    destroyConnectNode(node);
  }

  return;
}
#endif

/*
 * Description: 
 * Return: 
 * Others:
 */
void WorkThread::connectEventCallback(
    evutil_socket_t socketFd , short event, void *arg) {
  int errorCode = 0;
  ConnectNode *node = (ConnectNode*)arg;

  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN("Node:%p checkNodeExist falied, node is %s, do nothing later...",
          node, node_manager->getNodeStatusString(status).c_str());
      node->disconnectProcess();
      destroyConnectNode(node);
      return;
    }
  }

  // LOG_DEBUG("Node:%p connectEventCallback node status:%s ...",
  //     node, node_manager->getNodeStatusString(status).c_str());

  if (event == EV_TIMEOUT) {
    LOG_DEBUG("Node:%p connect EV_TIMEOUT.", node);
    goto EventProcessFailed;
  } else if (event == EV_CLOSED) {
    LOG_DEBUG("Node:%p connect EV_CLOSED.", node);
    goto EventProcessFailed;
  } else {
    // LOG_DEBUG("Node:%p connect status(%d):%s, event %02x",
    //     node, node->getConnectNodeStatus(),
    //     node->getConnectNodeStatusString().c_str(), event);
    if (node->getConnectNodeStatus() == NodeConnecting) {
      socklen_t len = sizeof(errorCode);
      getsockopt(socketFd, SOL_SOCKET, SO_ERROR, (char *) &errorCode, &len);
      if (!errorCode) {
        LOG_DEBUG("Node:%p connect return ev_write, check ok, set NodeStatus:NodeConnected.", node);
        node->setConnectNodeStatus(NodeConnected);
        node_manager->updateNodeStatus(node, NodeStatusConnected);

        #ifndef _MSC_VER
        // get client ip and port from socketFd
        struct sockaddr_in client;
        char client_ip[20];
        socklen_t client_len = sizeof(client);
        getsockname(socketFd, (struct sockaddr *)&client, &client_len);
        inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
        LOG_DEBUG("Node:%p local %s:%d", node, client_ip, ntohs(client.sin_port));
        #endif

        node->_isConnected = true;
      } else {
        if (node->socketConnect() < 0) {
          goto EventProcessFailed;
        }
      }
    }

    if (node->getConnectNodeStatus() == NodeConnected) {
      int ret = node->sslProcess();
      switch (ret) {
        case 0:
          LOG_DEBUG("Node:%p Begin gateway request process.", node);
          if (nodeRequestProcess(node) < 0) {
            destroyConnectNode(node);
          }
          break;
        case 1:
          //LOG_DEBUG("wait connect.");
          break;
        default:
          goto EventProcessFailed;
      }
    }
  }

  return;

EventProcessFailed:
  LOG_ERROR("Node:%p Connect failed:%s.",
      node,
      evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

  //node->closeConnectNode();
  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);
  node_manager->updateNodeStatus(node, NodeStatusConnecting);

  if (node->dnsProcess(_addrInFamily, _directIp, _enableSysGetAddr) < 0) {
    LOG_ERROR("Node:%p try delete request.", node);
    destroyConnectNode(node);
  }

  return;
}

void WorkThread::readEventCallBack(
    evutil_socket_t socketFd, short what, void *arg) {
  ConnectNode *node = (ConnectNode*)arg;
  char tmp_msg[512] = {0};

  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN("Node:%p checkNodeExist falied, node is %s, do nothing later...",
          node, node_manager->getNodeStatusString(status).c_str());
      if (status != NodeStatusReleased) {
        destroyConnectNode(node);
      }
      return;
    }
  }

  // LOG_DEBUG("Node:%p readEventCallBack what:%d.", node, what);

  if (what == EV_READ){
    ret = nodeResponseProcess(node);
    if (ret == -(InvalidRequest)) {
      LOG_ERROR("Node:%p has invalid request, skip all operation.", node);
      return;
    }
  } else if (what == EV_TIMEOUT){
    snprintf(tmp_msg, 512 - 1,
        "Recv timeout. socket error:%s.",
        evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

    LOG_ERROR("Node:%p %s", node, tmp_msg);

    node->handlerTaskFailedEvent(tmp_msg, EvRecvTimeout);
    node->closeConnectNode();
  } else {
    snprintf(tmp_msg, 512 - 1, "Unknown event:%02x. %s",
        what,
        evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

    LOG_ERROR("Node:%p %s", node, tmp_msg);

    node->handlerTaskFailedEvent(tmp_msg, EvUnknownEvent);
    node->closeConnectNode();
  }

  if (node->getConnectNodeStatus() == NodeInvalid &&
      node->getExitStatus() != ExitStopped) {
    destroyConnectNode(node);
  }

  return;
}

void WorkThread::writeEventCallBack(
    evutil_socket_t socketFd, short what, void *arg) {
  char tmp_msg[512] = {0};
  ConnectNode *node = (ConnectNode*)arg;

  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN("Node:%p checkNodeExist falied, node status is %s, do nothing later...",
          node, node_manager->getNodeStatusString(status).c_str());
      destroyConnectNode(node);
      return;
    }
  }

  // LOG_DEBUG("Node:%p writeEventCallBack what:%d.", node, what);

  if (what == EV_WRITE){
    nodeRequestProcess(node);
  } else if (what == EV_TIMEOUT){
    snprintf(tmp_msg, 512 - 1,
        "Send timeout. socket error:%s",
        evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

    LOG_ERROR("Node:%p %s", node, tmp_msg);

    node->handlerTaskFailedEvent(tmp_msg, EvSendTimeout);
    node->closeConnectNode();
  } else {
    snprintf(tmp_msg, 512 - 1, "Unknown event:%02x. %s",
        what,
        evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

    LOG_ERROR("Node:%p %s", node, tmp_msg);

    node->handlerTaskFailedEvent(tmp_msg, EvUnknownEvent);
    node->closeConnectNode();
  }

  if (node->getConnectNodeStatus() == NodeInvalid) {
    destroyConnectNode(node);
  }
}

void WorkThread::directConnect(void *arg, char *ip) {
  ConnectNode *node = (ConnectNode *)arg;
  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN("Node:%p checkNodeExist falied, node status is %s, do nothing later...",
          node, node_manager->getNodeStatusString(status).c_str());
      destroyConnectNode(node);
      return;
    }
  }

  if (ip) {
    LOG_DEBUG("Node:%p direct IpV4:%s", node, ip);

    int ret = node->connectProcess(ip, AF_INET);
    if (ret == 0) {
      ret = node->sslProcess();
      if (ret == 0) {
        LOG_DEBUG("Node:%p Begin gateway request process.", node);
        if (nodeRequestProcess(node) < 0) {
          destroyConnectNode(node);
        }
        return;
      }
    }

    if (ret == 1) {
      // connect  EINPROGRESS
      return;
    } else {
      LOG_DEBUG("Node:%p goto DirectConnectRetry.", node);
      goto DirectConnectRetry;
    }
  }

DirectConnectRetry:
  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);
  node_manager->updateNodeStatus(node, NodeStatusConnecting);
  if (node->dnsProcess(_addrInFamily, ip, _enableSysGetAddr) < 0) {
    destroyConnectNode(node);
  }

  return;
}

#ifndef _MSC_VER
void WorkThread::sysDnsEventCallback(
    evutil_socket_t socketFd, short what, void *arg) {
  ConnectNode *node = (ConnectNode *)arg;
  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN("Node:%p checkNodeExist falied, node status is %s, do nothing later...",
          node, node_manager->getNodeStatusString(status).c_str());
      destroyConnectNode(node);
      return;
    }
  }

  dnsEventCallback(what, node->_addrinfo, arg);

  return;
}
#endif

void WorkThread::dnsEventCallback(int errorCode,
                                  struct evutil_addrinfo *address,
                                  void *arg) {
  ConnectNode *node = (ConnectNode *)arg;
  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN("Node:%p checkNodeExist falied, status:%s, node status:%s, do nothing later...",
          node, node->getConnectNodeStatusString().c_str(),
          node_manager->getNodeStatusString(status).c_str());
      // maybe mem leak here
      destroyConnectNode(node);
      return;
    }
  }

  if (errorCode) {
    LOG_ERROR("Node:%p %s dns failed: %s.",
        node, node->_url._host, evutil_gai_strerror(errorCode));
    node->setConnectNodeStatus(NodeConnecting);
    node_manager->updateNodeStatus(node, NodeStatusConnecting);
    if (node->dnsProcess(_addrInFamily, _directIp, _enableSysGetAddr) < 0) {
      destroyConnectNode(node);
    }
    return;
  }

  if (address->ai_canonname) {
    LOG_DEBUG("Node:%p ai_canonname: %s", node, address->ai_canonname);
  }

  struct evutil_addrinfo *ai;
  for (ai = address; ai; ai = ai->ai_next) {
    char buffer[HOST_SIZE] = {0};
    const char *ip = NULL;
    if (ai->ai_family == AF_INET) {
      struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
      ip = evutil_inet_ntop(AF_INET, &sin->sin_addr, buffer, HOST_SIZE);

      if (ip) {
        LOG_DEBUG("Node:%p IpV4:%s", node, ip);

        int ret = node->connectProcess(ip, AF_INET);
        if (ret == 0) {
          ret = node->sslProcess();
          if (ret == 0) {
            LOG_DEBUG("Node:%p Begin gateway request process.", node);
            if (nodeRequestProcess(node) < 0) {
              destroyConnectNode(node);
            }

            return ;
          }
        }

        if (ret == 1) {
          // connect  EINPROGRESS
          break;
        } else {
          LOG_DEBUG("Node:%p goto ConnectRetry, ret:%d.", node, ret);
          goto ConnectRetry;
        }
      }

    } else if (ai->ai_family == AF_INET6) {
      struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
      ip = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buffer, HOST_SIZE);

      if (ip) {
        LOG_DEBUG("Node:%p IpV6:%s", node, ip);

        int ret = node->connectProcess(ip, AF_INET6);
        if (ret == 0) {
          LOG_DEBUG("Node:%p Begin ssl process.", node);
          ret = node->sslProcess();
          if (ret == 0) {
            LOG_DEBUG("Node:%p Begin gateway request process.", node);
            if (nodeRequestProcess(node) < 0) {
              destroyConnectNode(node);
            }
            return ;
          }
        }

        if (ret == 1) {
          break;
        } else {
          LOG_DEBUG("Node:%p goto ConnectRetry.", node);
          goto ConnectRetry;
        }
      }
    }
  }

  evutil_freeaddrinfo(address);

  return;

ConnectRetry:
  evutil_freeaddrinfo(address);  
  // node->closeConnectNode();
  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);
  node_manager->updateNodeStatus(node, NodeStatusConnecting);
  if (node->dnsProcess(_addrInFamily, _directIp, _enableSysGetAddr) < 0) {
    destroyConnectNode(node);
  }

  return;
}

/*
 * Description: libevent收到命令消息进行事件处理的回调
 * Return: 
 * Others:
 */
void WorkThread::notifyEventCallback(evutil_socket_t fd, short which, void *arg) {
  WorkThread *pThread = (WorkThread*)arg;
  int ret = Success;
  char msgCmd;
  if (recv(pThread->_notifyReceiveFd, (char *)&msgCmd, sizeof(char), 0) <= 0) {
    LOG_ERROR("work Thread recv() failed:%d.", utility::getLastErrorCode());
    return;
  }

  LOG_DEBUG("workThread(%p) receive: '%c' from main thread.", pThread, msgCmd);

  if (msgCmd == 'c') {
    INlsRequest *request = getQueueNode(pThread);
    if (request == NULL) {
      LOG_ERROR("request is nullptr, you have destroyed request or relesed instance!");
      return;
    }

    NlsClient* client = _instance;
    NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
    int status = NodeStatusInvalid;
    ret = node_manager->checkRequestExist(request, &status);
    if (ret != Success) {
      if (ret == -(RequestEmpty) || ret == -(NodeEmpty)) {
        LOG_ERROR("request(%p) checkRequestExist failed, ret:%d, do nothing later...", request, ret);
      } else {
        LOG_ERROR("request(%p) checkRequestExist failed, ret:%d, chang to CmdCancel.", request, ret);
        event_base_loopbreak(pThread->_workBase);
      }
      return;
    } else {
      if (status >= NodeStatusCancelling) {
        LOG_WARN("request(%p) node(%p) checkRequestExist falied, node is %s, do nothing later...",
            request, request->getConnectNode(), node_manager->getNodeStatusString(status).c_str());
        destroyConnectNode(request->getConnectNode());
        return;
      }
    }

    insertListNode(pThread, request);

    LOG_DEBUG("workThread(%p) Node:%p begin dnsProcess.", pThread, request->getConnectNode());

    if (request->getConnectNode()->dnsProcess(
          _addrInFamily, _directIp, _enableSysGetAddr) < 0) {
      destroyConnectNode(request->getConnectNode());
    }
  } else if (msgCmd == 's') {
    event_base_loopbreak(pThread->_workBase);
  } else {
    LOG_ERROR("workThread(%p) recv invalid cmd:'%c'.", pThread, msgCmd);
  }

  return;
}

/*
 * Description: 开始gateway的请求处理
 * Return: 成功则Success, 失败则返回负值.
 * Others:
 */
int WorkThread::nodeRequestProcess(ConnectNode* node) {
  int ret = Success;

  //LOG_DEBUG("Node:%p nodeResquestProcess begin.", node);

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    return -(NodeEmpty);
  }

  //invoke cancel()
  if (node->getExitStatus() == ExitCancel) {
    node->closeConnectNode();
    return -(CancelledExitStatus);
  }

  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return ret;
  }

  ConnectStatus workStatus = node->getConnectNodeStatus();
  // LOG_DEBUG("Node:%p workStatus %d(%s).",
  //     node, workStatus, node->getConnectNodeStatusString().c_str());
  switch(workStatus) {
    /*connect to gateWay*/
    case NodeHandshaking:
      node->gatewayRequest();
      ret = node->nlsSendFrame(node->getCmdEvBuffer());
      node->setConnectNodeStatus(NodeHandshaked);
      node_manager->updateNodeStatus(node, NodeStatusHandshaked);
      break;

    case NodeHandshaked:
    case NodeStarting:
      ret = node->nlsSendFrame(node->getCmdEvBuffer());
      break;

    case NodeWakeWording:
      ret = node->nlsSendFrame(node->getWwvEvBuffer());
      if (ret == 0) {
        if (node->getWakeStatus()) {
          node->addCmdDataBuffer(CmdWarkWord);
          ret = node->nlsSendFrame(node->getCmdEvBuffer());
        }
      }
      break;

    case NodeStarted:
      ret = node->nlsSendFrame(node->getBinaryEvBuffer());
      /* 音频数据发送完毕，检测是否需要发送控制指令数据 */
      if (ret == 0) {
        ret = node->sendControlDirective();
      }
      break;

    default:
      ret = -(InvalidWorkStatus);
      break;
  }

  if (ret < 0) {
    LOG_ERROR("Node:%p Send failed, ret:%d.", node, ret);

    std::string failedInfo = node->getErrorMsg();
    if (failedInfo.empty()) {
      char tmp_msg[512] = {0};
      snprintf(tmp_msg, 512 - 1, "workThread workStatus(%s) Send failed. error_code(%d)",
          node->getConnectNodeStatusString().c_str(), ret);
      failedInfo.assign(tmp_msg);
    }
    node->handlerTaskFailedEvent(failedInfo);
    node->closeConnectNode();
    return ret;
  }

  //LOG_DEBUG("Node:%p nodeResquestProcess done.", node);

  return Success;
}

/*
 * Description: 接收gateway的响应
 * Return: 成功则Success, 失败则返回负值.
 * Others:
 */
int WorkThread::nodeResponseProcess(ConnectNode* node) {
  int ret = Success;

  //LOG_DEBUG("Node:%p nodeResponseProcess begin.", node);

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    return -(NodeEmpty);
  }

  //invoke cancel()
  if (node->getExitStatus() == ExitCancel) {
    node->closeConnectNode();
    return -(CancelledExitStatus);
  }

  NlsClient* client = _instance;
  NlsNodeManager* node_manager = (NlsNodeManager*)client->getNodeManger();
  int status = NodeStatusInvalid;
  ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d", ret);
    return ret;
  }

  ConnectStatus workStatus = node->getConnectNodeStatus();
  LOG_DEBUG("Node:%p workStatus %d(%s).",
      node, workStatus, node->getConnectNodeStatusString().c_str());
  switch(workStatus) {
    /*connect to gateway*/
    case NodeHandshaking:
    case NodeHandshaked:
      ret = node->gatewayResponse();
      if (ret == 0) {
        /* ret == 0 mean parsing response successfully */
        node->setConnectNodeStatus(NodeStarting);
        node_manager->updateNodeStatus(node, NodeStatusRunning);
        if (node->_request->getRequestParam()->_requestType == SpeechTextDialog) {
          node->addCmdDataBuffer(CmdTextDialog);
        } else {
          node->addCmdDataBuffer(CmdStart);
        }
        ret = node->nlsSendFrame(node->getCmdEvBuffer());
      } else if (ret == -(NlsReceiveEmpty)) {
        LOG_WARN("Node:%p nlsReceive empty, try again...", node);
        return Success;
      }
      break;

    /*send start command*/
    case NodeStarting:
    case NodeWakeWording:
      ret = node->webSocketResponse();
      workStatus = node->getConnectNodeStatus();
      if (workStatus == NodeStarted) {
        ret = node->nlsSendFrame(node->getBinaryEvBuffer());
        if (ret == 0) {
          ret = node->sendControlDirective();
        }
      } else if (workStatus == NodeWakeWording){
        ret = node->nlsSendFrame(node->getWwvEvBuffer());
      }
      break;

    case NodeStarted:
      ret = node->webSocketResponse();
      break;

    case NodeConnecting:
      if (node->_isLongConnection) {
        /*
         * 在长链接模式下, 可能存在进入NodeConnecting而非NodeStarted状态的情况
         * 以NodeStarted来处理......
         */
        LOG_WARN("Node:%p NodeConnecting is abnormal", node);
        ret = node->webSocketResponse();
      } else {
        ret = -(InvalidWorkStatus);
      }
      break;

    case NodeInvalid:
      // request has released
      ret = -(InvalidRequest);
      break;

    default:
      ret = -(InvalidWorkStatus);
      break;
  }

  if (ret < 0) {
    LOG_ERROR("Node:%p Response failed, ret:%d", node, ret);

    std::string failedInfo = node->getErrorMsg();
    if (failedInfo.empty()) {
      char tmp_msg[512] = {0};
      snprintf(tmp_msg, 512 - 1, "workThread workStatus(%s) Response failed. error_code(%d)",
          node->getConnectNodeStatusString().c_str(), ret);
      failedInfo.assign(tmp_msg);
    }
    if (ret == -(InvalidRequest)) {
      LOG_ERROR("Node:%p Response failed, errormsg:%s. But request has released, ignore TaskFailed and Closed event.", node, failedInfo.c_str());
    } else {
      node->handlerTaskFailedEvent(failedInfo);
      node->closeConnectNode();
    }
  }

  //LOG_DEBUG("Node:%p nodeResponseProcess done.", node);

  return ret;
}

void WorkThread::setAddrInFamily(int aiFamily) {
  _addrInFamily = aiFamily;
}

void WorkThread::setDirectHost(char *directIp) {
  memset(_directIp, 0, 64);
  if (directIp && strnlen(directIp, 64) > 0) {
    strncpy(_directIp, directIp, 64);
  }
}

void WorkThread::setUseSysGetAddrInfo(bool enable) {
  _enableSysGetAddr = enable;
}

void WorkThread::setInstance(NlsClient* instance) {
  _instance = instance;
}

}



