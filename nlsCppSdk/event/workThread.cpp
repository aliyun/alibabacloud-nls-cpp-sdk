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

#include <algorithm>
#include <string>
#ifdef _MSC_VER
#include <process.h>
#else
#include <sched.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

#include "connectNode.h"
#include "iNlsRequest.h"
#include "iNlsRequestParam.h"
#include "nlog.h"
#include "nlsClientImpl.h"
#include "nlsGlobal.h"
#include "nlsRequestParamInfo.h"
#include "nodeManager.h"
#include "text_utils.h"
#include "utility.h"
#include "workThread.h"

namespace AlibabaNls {

NlsClientImpl *WorkThread::_instance = NULL;

#if defined(_MSC_VER)
HANDLE WorkThread::_mtxCpu = NULL;
#else
pthread_mutex_t WorkThread::_mtxCpu = PTHREAD_MUTEX_INITIALIZER;
#endif

WorkThread::WorkThread()
    : _workBase(NULL),
      _dnsBase(NULL),
      _workThreadId(0),
      _addrInFamily(AF_INET),
      _directIp(),
      _enableSysGetAddr(false) {
#if defined(_MSC_VER)
  _mtxList = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxList, NULL);
#endif

  _workBase = event_base_new();
  if (NULL == _workBase) {
    LOG_ERROR("WorkThread(%p) invoke event_base_new failed.", this);
    exit(1);
  }
  int features = event_base_get_features(_workBase);
  LOG_INFO("WorkThread(%p) create evbase(%p), get features %d", this, _workBase,
           features);

  _dnsBase = evdns_base_new(_workBase, 1);
  if (NULL == _dnsBase) {
    LOG_WARN("WorkThread(%p) invoke evdns_base_new failed.", this);
    // no need dnsBase if _directIp true
  } else {
    // many parameters please see
    // https://libevent.org/libevent-book/Ref9_dns.html

    // disable mixed cases
    LOG_DEBUG("WorkThread(%p) evdns_base setting randomize-case 0.", this);
    evdns_base_set_option(_dnsBase, "randomize-case", "0");

    // How long, in seconds, do we wait for a response from a DNS server before
    // we assume we aren’t getting one?
    int default_ms = D_DEFAULT_CONNECTION_TIMEOUT_MS;
    float timeout_sec = (float)default_ms / 1000;
    std::string timeout_sec_str = utility::TextUtils::to_string(timeout_sec);
    LOG_DEBUG("WorkThread(%p) evdns_base setting timeout %s seconds.", this,
              timeout_sec_str.c_str());
    evdns_base_set_option(_dnsBase, "timeout", timeout_sec_str.c_str());

    // add search domain for nls-gateway.cn-shanghai-internal.aliyuncs.com
    evdns_base_search_add(_dnsBase, "gds.alibabadns.com");
  }

#if defined(_MSC_VER)
  _workThreadHandle = (HANDLE)_beginthreadex(NULL, 0, loopEventCallback,
                                             (LPVOID)this, 0, &_workThreadId);
  CloseHandle(_workThreadHandle);
#else
  pthread_create(&_workThreadId, NULL, loopEventCallback, (void *)this);
#endif
}

WorkThread::~WorkThread() {
  LOG_DEBUG(
      "Destroy WorkThread(%p) begin, close all fd and events, nodeList "
      "size:%d.",
      this, _nodeList.size());

  if (_instance == NULL) {
    LOG_WARN("NlsClientImpl has not yet been created");
    return;
  }

  int try_count = 500;
  NlsClientImpl *client = _instance;
  NlsNodeManager *node_manager = client->getNodeManger();

  // must check asr end
  do {
#ifdef _MSC_VER
    Sleep(10);
#else
    usleep(10 * 1000);
#endif
    MUTEX_LOCK(_mtxList);

    int ret = Success;
    std::list<INlsRequest *>::iterator itList;
    for (itList = _nodeList.begin(); itList != _nodeList.end();) {
      INlsRequest *request = *itList;
      if (request == NULL) {
        _nodeList.erase(itList++);
        continue;
      }

      ConnectNode *node = request->getConnectNode();
      if (node == NULL) {
        LOG_ERROR(
            "The node of request(%p) is nullptr, you have destroyed request or "
            "relesed instance!");
        _nodeList.erase(itList++);
        continue;
      }

      /* 1. 检查request是否存在于全局管理的node_manager中, request可能已经释放,
       * 需要跳过. */
      int status = NodeStatusInvalid;
      ret = node_manager->checkRequestExist(request, &status);
      if (ret != Success) {
        LOG_ERROR("Request(%p) checkRequestExist failed, ret:%d.", request,
                  ret);
        _nodeList.erase(itList++);
        continue;
      }

      ConnectStatus node_status = node->getConnectNodeStatus();
      ExitStatus exit_status = node->getExitStatus();
      LOG_DEBUG("Request(%p) Node(%p) node status:%s, exit status:%s.", request,
                node, node->getConnectNodeStatusString().c_str(),
                node->getExitStatusString().c_str());

      /* 2. 删除request并从全局管理的node_manager中移除 */
      if (node_status == NodeInvalid || node_status == NodeCreated) {
        _nodeList.erase(itList++);
        node_manager->removeRequestFromInfo(request, false);
        delete request;
        request = NULL;
      } else {
        LOG_WARN(
            "Destroy WorkThread(%p) node(%p) is invalid, node status:%s, exit "
            "status:%s, skip ...",
            this, node, node->getConnectNodeStatusString().c_str(),
            node->getExitStatusString().c_str());
        ++itList;
      }
    }  // for

    LOG_DEBUG("Destroy WorkThread(%p) _nodeList:%d, try_count:%d.", this,
              _nodeList.size(), try_count);

    /* 3. 超过尝试限制后, 强制销毁所有request, 清空_nodeList. */
    if (try_count-- <= 0) {
      for (itList = _nodeList.begin(); itList != _nodeList.end();) {
        INlsRequest *request = *itList;
        _nodeList.erase(itList++);
        node_manager->removeRequestFromInfo(request, false);
        delete request;
        request = NULL;
      }  // for
    }

    MUTEX_UNLOCK(_mtxList);
  } while (_nodeList.size() > 0 && try_count-- > 0);  // do while

  event_base_loopbreak(_workBase);

#if defined(_MSC_VER)
  CloseHandle(_mtxList);
#else
  LOG_DEBUG(
      "Destroy WorkThread(%p) join _workThreadId:0x%lx, please waiting ...",
      this, _workThreadId);
  if (_workThreadId != 0) {
    pthread_join(_workThreadId, NULL);
  }
  pthread_mutex_destroy(&_mtxList);
#endif

  LOG_DEBUG("Destroy WorkThread(%p) done.", this);
}

void WorkThread::insertListNode(WorkThread *thread, INlsRequest *request) {
  if (thread == NULL || request == NULL) {
    LOG_ERROR("thread or request is nullptr.");
    return;
  }

  MUTEX_LOCK(thread->_mtxList);
  std::list<INlsRequest *>::iterator iLocation =
      find(thread->_nodeList.begin(), thread->_nodeList.end(), request);
  if (iLocation == thread->_nodeList.end()) {
    thread->_nodeList.push_back(request);
  }
  MUTEX_UNLOCK(thread->_mtxList);
  return;
}

bool WorkThread::freeListNode(WorkThread *thread, INlsRequest *request) {
  if (thread == NULL || request == NULL) {
    LOG_ERROR("thread or request is nullptr.");
    return false;
  }

  MUTEX_LOCK(thread->_mtxList);

  std::list<INlsRequest *>::iterator iLocation =
      find(thread->_nodeList.begin(), thread->_nodeList.end(), request);

  if (iLocation != thread->_nodeList.end()) {
    thread->_nodeList.remove(*iLocation);
  }

  MUTEX_UNLOCK(thread->_mtxList);
  return true;
}

/**
 * @brief: 释放request和node
 * @return:
 */
void WorkThread::destroyConnectNode(ConnectNode *node) {
  MUTEX_LOCK(_mtxCpu);

  if (node == NULL) {
    LOG_ERROR("Input node is nullptr.");
    MUTEX_UNLOCK(_mtxCpu);
    return;
  }

  LOG_INFO("Node(%p) destroyConnectNode begin, node status:%s exit status:%s.",
           node, node->getConnectNodeStatusString().c_str(),
           node->getExitStatusString().c_str());

  NlsClientImpl *client = _instance;
  NlsNodeManager *node_manager = client->getNodeManger();
  int status = NodeStatusInvalid;

  bool success = freeListNode(node->getEventThread(), node->getRequest());

  if (success && node->updateDestroyStatus()) {
    INlsRequest *request = node->getRequest();
    if (request) {
      LOG_DEBUG("Node(%p) destroy request.", node);

      /* 准备移出request, 上锁保护, 防止其他线程也同时在释放 */
      bool release_lock_ret = true;
      MUTEX_TRY_LOCK(client->_mtxReleaseRequestGuard, 2000, release_lock_ret);
      if (!release_lock_ret) {
        LOG_ERROR("Request(%p) lock destroy failed, deadlock has occurred",
                  request);
      }

      int ret = node_manager->checkRequestExist(request, &status);
      if (ret != -(RequestEmpty)) {
        node_manager->removeRequestFromInfo(request, false);
      }
      if (ret == Success) {
        delete request;
      }
      request = NULL;
      node->setRequest(NULL);

      if (release_lock_ret) {
        MUTEX_UNLOCK(client->_mtxReleaseRequestGuard);
      }
    } else {
      LOG_WARN("The request of node(%p) is nullptr.", node);
    }
    LOG_INFO("Node(%p) destroyConnectNode done.", node);
  }

  LOG_INFO("Node(%p) destroyConnectNode finish.", node);

  MUTEX_UNLOCK(_mtxCpu);
  return;
}

#if defined(_MSC_VER)
unsigned __stdcall WorkThread::loopEventCallback(LPVOID arg) {
#else
void *WorkThread::loopEventCallback(void *arg) {
#endif

  WorkThread *eventParam = static_cast<WorkThread *>(arg);

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

  prctl(PR_SET_NAME, "eventThread");
#endif

  LOG_DEBUG("workThread(%p) create loopEventCallback.", arg);

  if (eventParam->_workBase) {
    LOG_DEBUG("workThread(%p) event_base_dispatch ...", arg);
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

  eventParam->_workThreadId = 0;

  LOG_DEBUG("workThread(%p) loopEventCallback exit.", arg);

#if defined(_MSC_VER)
  return Success;
#else
  return NULL;
#endif
}

#ifdef ENABLE_HIGH_EFFICIENCY
/**
 * @brief: 定时进行connect()后检查链接状态并开启ssl握手.
 * @return:
 */
void WorkThread::connectTimerEventCallback(evutil_socket_t socketFd,
                                           short event, void *arg) {
  int errorCode = 0;
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  node->_inEventCallbackNode = true;

  LOG_DEBUG("Node(%p) connectTimerEventCallback node status:%s ...", node,
            node->getConnectNodeStatusString().c_str());

  if (event == EV_CLOSED) {
    LOG_DEBUG("Node(%p) connect get EV_CLOSED.", node);
    goto ConnectTimerProcessFailed;
  } else {
    // event == EV_TIMEOUT
    if (node->getConnectNodeStatus() == NodeConnecting) {
      socklen_t len = sizeof(errorCode);
      getsockopt(socketFd, SOL_SOCKET, SO_ERROR, (char *)&errorCode, &len);
      if (!errorCode) {
        LOG_DEBUG(
            "Node(%p) connect return ev_write, check ok, set "
            "NodeStatus:NodeConnected.",
            node);
        node->setConnectNodeStatus(NodeConnected);

#ifndef _MSC_VER
        // get client ip and port from socketFd
        struct sockaddr_in client;
        char client_ip[20];
        socklen_t client_len = sizeof(client);
        getsockname(socketFd, (struct sockaddr *)&client, &client_len);
        inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
        LOG_DEBUG("Node(%p) local %s:%d", node, client_ip,
                  ntohs(client.sin_port));
#endif

        node->setConnected(true);
      } else {
        /* 再次尝试connect(), 并启动下一次connectEventCallback */
        if (node->socketConnect() < 0) {
          /* socket connect 失败 */
          goto ConnectTimerProcessFailed;
        }
      }
    }

    /* connect成功, 开始握手 */
    if (node->getConnectNodeStatus() == NodeConnected) {
      int ret = node->sslProcess();
      switch (ret) {
        case 0:
          LOG_DEBUG("Node(%p) begin gateway request process.", node);
          /* 进入gatewayRequest()的ssl握手阶段 */
          if (nodeRequestProcess(node) < 0) {
            destroyConnectNode(node);
          }
          break;
        case 1:
          /* sslProcess()中已经启动了下一次_connectEvent */
          // LOG_DEBUG("wait connecting ...");
          break;
        default:
          goto HandshakeTimerProcessFailed;
      }
    }
  }

#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif

  LOG_DEBUG("Node(%p) connectTimerEventCallback done.", node);
  return;

HandshakeTimerProcessFailed:
ConnectTimerProcessFailed:
  /*
   * connect失败, 或者connect成功但是handshake失败.
   * 进行断链并重回connecting阶段, 然后再开始dns解析.
   */
  LOG_ERROR("Node(%p) connect or handshake failed, socket error mesg:%s.", node,
            evutil_socket_error_to_string(
                evutil_socket_geterror(node->getSocketFd())));

  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);

  if (node->dnsProcess(node->getEventThread()->_addrInFamily,
                       node->getEventThread()->_directIp,
                       node->getEventThread()->_enableSysGetAddr) < 0) {
    LOG_ERROR("Node(%p) try delete request.", node);
    destroyConnectNode(node);
  }

#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif

  LOG_DEBUG("Node(%p) connectTimerEventCallback done with failure.", node);
  return;
}
#endif

/**
 * @brief: connect()后检查链接状态并开启ssl握手.
 * @return:
 */
void WorkThread::connectEventCallback(evutil_socket_t socketFd, short event,
                                      void *arg) {
  int errorCode = 0;
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  node->_inEventCallbackNode = true;

  // LOG_DEBUG("Node(%p) connectEventCallback node status:%s ...",
  //     node, node_manager->getNodeStatusString(status).c_str());

  if (event == EV_TIMEOUT) {
    LOG_ERROR("Node(%p) connect get EV_TIMEOUT.", node);
    goto ConnectProcessFailed;
  } else if (event == EV_CLOSED) {
    LOG_DEBUG("Node(%p) connect get EV_CLOSED.", node);
    goto ConnectProcessFailed;
  } else {
    LOG_DEBUG("Node(%p) current connect node status:%s, EV:%02x.", node,
              node->getConnectNodeStatusString().c_str(), event);
    if (node->getConnectNodeStatus() == NodeConnecting) {
      socklen_t len = sizeof(errorCode);
      getsockopt(socketFd, SOL_SOCKET, SO_ERROR, (char *)&errorCode, &len);
      if (!errorCode) {
        LOG_DEBUG(
            "Node(%p) connect return ev_write, check ok, set "
            "NodeStatus:NodeConnected.",
            node);
        node->setConnectNodeStatus(NodeConnected);
        node->setConnected(true);

#ifndef _MSC_VER
        // get client ip and port from socketFd
        struct sockaddr_in client;
        char client_ip[20];
        socklen_t client_len = sizeof(client);
        getsockname(socketFd, (struct sockaddr *)&client, &client_len);
        inet_ntop(AF_INET, &client.sin_addr, client_ip, sizeof(client_ip));
        LOG_DEBUG("Node(%p) local %s:%d", node, client_ip,
                  ntohs(client.sin_port));
#endif
      } else {
        /* 再次尝试connect(), 并启动下一次connectEventCallback */
        if (node->socketConnect() < 0) {
          /* socket connect 失败 */
          goto ConnectProcessFailed;
        }
      }
    }

    /* connect成功, 开始握手 */
    if (node->getConnectNodeStatus() == NodeConnected) {
      int ret = node->sslProcess();
      switch (ret) {
        case 0:
          LOG_DEBUG("Node(%p) begin gateway request process.", node);
          /* 进入gatewayRequest()的ssl握手阶段 */
          if (nodeRequestProcess(node) < 0) {
            destroyConnectNode(node);
          }
          break;
        case 1:
          /* sslProcess()中已经启动了下一次_connectEvent */
          // LOG_DEBUG("wait connecting.");
          break;
        default:
          goto HandshakeProcessFailed;
      }
    }
  }

#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif

  LOG_DEBUG("Node(%p) connectEventCallback done.", node);
  return;

HandshakeProcessFailed:
ConnectProcessFailed:
  /*
   * connect失败, 或者connect成功但是handshake失败.
   * 进行断链并重回connecting阶段, 然后再开始dns解析.
   */
  LOG_ERROR("Node(%p) connect or handshake failed, socket error mesg:%s.", node,
            evutil_socket_error_to_string(
                evutil_socket_geterror(node->getSocketFd())));

#ifdef ENABLE_DNS_IP_CACHE
  node->getEventThread()->setIpCache(NULL, NULL);
#endif
  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);

  if (node->dnsProcess(node->getEventThread()->_addrInFamily,
                       node->getEventThread()->_directIp,
                       node->getEventThread()->_enableSysGetAddr) < 0) {
    LOG_ERROR("Node(%p) try delete request.", node);
    destroyConnectNode(node);
  }

#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif

  LOG_DEBUG("Node(%p) connectEventCallback done with failure.", node);
  return;
}

void WorkThread::readEventCallBack(evutil_socket_t socketFd, short what,
                                   void *arg) {
  char tmp_msg[512] = {0};
  int ret = Success;
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  if (node == NULL) {
    LOG_ERROR("Node is nullptr!!!");
    return;
  }
  NlsNodeManager *node_manager = node->getInstance()->getNodeManger();
  int status = NodeStatusInvalid;
  int result = node_manager->checkNodeExist(node, &status);
  if (result != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", node, result);
    return;
  }

  node->_inEventCallbackNode = true;

  // LOG_DEBUG("Node(%p) readEventCallBack begin, current event:%d, node
  // status:%s, exit status:%s.",
  //     node, what,
  //     node->getConnectNodeStatusString().c_str(),
  //     node->getExitStatusString().c_str());

  if (node->getExitStatus() == ExitCancel) {
    LOG_WARN("Node(%p) skip this operation ...", node);
    node->closeConnectNode();
#ifdef _MSC_VER
    SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
    SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                     node->_inEventCallbackNode);
#endif
    return;
  }

  if (what == EV_READ) {
    ret = nodeResponseProcess(node);
    if (ret == -(InvalidRequest)) {
      LOG_ERROR("Node(%p) has invalid request, skip all operation.", node);
      node->closeConnectNode();
#ifdef _MSC_VER
      SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
      SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                       node->_inEventCallbackNode);
#endif
      return;
    } else if (ret == -(EventClientEmpty)) {
      LOG_ERROR("Instance has released, skip all operation.");
      return;
    } else if (ret == -(InvalidStatusWhenReleasing)) {
      LOG_ERROR("Node(%p) is releasing, skip all operation.", node);
      return;
    }
  } else if (what == EV_TIMEOUT) {
    snprintf(tmp_msg, 512 - 1, "Recv timeout. socket error:%s.",
             evutil_socket_error_to_string(
                 evutil_socket_geterror(node->getSocketFd())));

    LOG_ERROR("Node(%p) error msg:%s.", node, tmp_msg);

    node->closeConnectNode();
    node->handlerTaskFailedEvent(tmp_msg, EvRecvTimeout);
  } else {
    snprintf(tmp_msg, 512 - 1, "Unknown event:%02x. %s", what,
             evutil_socket_error_to_string(
                 evutil_socket_geterror(node->getSocketFd())));

    LOG_ERROR("Node(%p) error msg:%s.", node, tmp_msg);

    node->closeConnectNode();
    node->handlerTaskFailedEvent(tmp_msg, EvUnknownEvent);
  }

  // LOG_DEBUG("Node(%p) readEventCallBack done.", node);
#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif
  return;
}

void WorkThread::writeEventCallBack(evutil_socket_t socketFd, short what,
                                    void *arg) {
  char tmp_msg[512] = {0};
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  if (node == NULL) {
    LOG_ERROR("Node is nullptr!!!");
    return;
  }
  NlsNodeManager *node_manager = node->getInstance()->getNodeManger();
  int status = NodeStatusInvalid;
  int result = node_manager->checkNodeExist(node, &status);
  if (result != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", node, result);
    return;
  }

  node->_inEventCallbackNode = true;

  // LOG_DEBUG("Node(%p) writeEventCallBack current event:%d, node status:%s,
  // exit status:%s.",
  //     node, what,
  //     node->getConnectNodeStatusString().c_str(),
  //     node->getExitStatusString().c_str());

  if (node->getExitStatus() == ExitCancel) {
    LOG_WARN("Node(%p) skip this operation ...", node);
    node->closeConnectNode();
#ifdef _MSC_VER
    SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
    SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                     node->_inEventCallbackNode);
#endif
    return;
  }

  if (what == EV_WRITE) {
    nodeRequestProcess(node);
  } else if (what == EV_TIMEOUT) {
    snprintf(tmp_msg, 512 - 1, "Send timeout. socket error:%s",
             evutil_socket_error_to_string(
                 evutil_socket_geterror(node->getSocketFd())));

    LOG_ERROR("Node(%p) %s", node, tmp_msg);

    node->closeConnectNode();
    node->handlerTaskFailedEvent(tmp_msg, EvSendTimeout);
  } else {
    snprintf(tmp_msg, 512 - 1, "Unknown event:%02x. %s", what,
             evutil_socket_error_to_string(
                 evutil_socket_geterror(node->getSocketFd())));

    LOG_ERROR("Node(%p) %s.", node, tmp_msg);

    node->closeConnectNode();
    node->handlerTaskFailedEvent(tmp_msg, EvUnknownEvent);
  }

  if (node->getConnectNodeStatus() == NodeInvalid) {
    destroyConnectNode(node);
  }

  // LOG_DEBUG("Node(%p) writeEventCallBack done.", node);
#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif
  return;
}

/**
 * @brief: IP直连
 * @return:
 */
void WorkThread::directConnect(void *arg, char *ip) {
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  if (ip) {
    LOG_DEBUG("Node(%p) direct IpV4:%s.", node, ip);

    int ret = node->connectProcess(ip, AF_INET);
    if (ret == 0) {
      ret = node->sslProcess();
      if (ret == Success) {
        LOG_DEBUG("Node(%p) begin gateway request process.", node);
        if (nodeRequestProcess(node) < 0) {
          destroyConnectNode(node);
        }
        return;
      }
    }

    if (ret == 1) {
      LOG_DEBUG("Node(%p) connectProcess return 1, will try connect ...", node);
      // connect  EINPROGRESS
      return;
    } else {
      LOG_ERROR(
          "Node(%p) goto DirectConnectRetry with ret:%d and node status:%s "
          "exit status:%s.",
          node, ret, node->getConnectNodeStatusString().c_str(),
          node->getExitStatusString().c_str());
#ifdef ENABLE_DNS_IP_CACHE
      node->getEventThread()->setIpCache(NULL, NULL);
#endif
      goto DirectConnectRetry;
    }
  }

DirectConnectRetry:
  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);
  if (node->dnsProcess(node->getEventThread()->_addrInFamily, ip,
                       node->getEventThread()->_enableSysGetAddr) < 0) {
    destroyConnectNode(node);
  }

  return;
}

/**
 * @brief: 启动语音交互请求
 * @return:
 */
void WorkThread::launchEventCallback(evutil_socket_t fd, short which,
                                     void *arg) {
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  if (NULL == node) {
    LOG_ERROR("Node is nullptr!!!");
    return;
  }
  NlsNodeManager *node_manager = node->getInstance()->getNodeManger();
  int status = NodeStatusInvalid;
  int result = node_manager->checkNodeExist(node, &status);
  if (result != Success) {
    LOG_ERROR("The node(%p) checkNodeExist failed, result:%d.", node, result);
    return;
  }

  INlsRequest *request = node->getRequest();
  WorkThread *pThread = node->getEventThread();
  if (pThread == NULL) {
    LOG_ERROR("The WorkThread of Node(%p) is nullptr, skipping ...", node);
    return;
  }

  node->_inEventCallbackNode = true;

  LOG_DEBUG(
      "WorkThread(%p) Node(%p) Request(%p) trigger launchEventCallback with "
      "reconnection mechanism(%s).",
      pThread, node, request,
      request->getRequestParam()->_enableReconnect ? "true" : "false");

  if (node->getExitStatus() == ExitCancel ||
      node->getExitStatus() == ExitStopping) {
    LOG_WARN(
        "WorkThread(%p) Node(%p) is canceling/stopping, current node "
        "status:%s, skip "
        "here.",
        pThread, node, node->getConnectNodeStatusString().c_str());
    node->setConnectNodeStatus(NodeInvoked);
#ifdef _MSC_VER
    SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
    SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                     node->_inEventCallbackNode);
#endif
    return;
  }

  insertListNode(pThread, request);

  node->setConnectNodeStatus(NodeInvoked);
  /* 将request设置的参数传入node */
  node->updateParameters();

  LOG_DEBUG("WorkThread(%p) Node(%p) begin dnsProcess.", pThread, node);
  if (node->dnsProcess(node->getEventThread()->_addrInFamily,
                       node->getEventThread()->_directIp,
                       node->getEventThread()->_enableSysGetAddr) < 0) {
    LOG_WARN(
        "WorkThread(%p) Node(%p) dnsProcess failed, ready to "
        "destroyConnectNode.",
        pThread, node);
    destroyConnectNode(node);
  }

#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif
  return;
}

#ifdef __LINUX__
void WorkThread::sysDnsEventCallback(evutil_socket_t socketFd, short what,
                                     void *arg) {
  if (what == EV_READ) {
    /* check this node is alive */
    NlsClientImpl *client = _instance;
    NlsNodeManager *node_manager = client->getNodeManger();
    int status = NodeStatusInvalid;
    ConnectNode *node = static_cast<ConnectNode *>(arg);
    int ret = node_manager->checkNodeExist(node, &status);
    if (ret != Success) {
      LOG_ERROR("checkNodeExist failed, ret:%d.", ret);
      return;
    }

    dnsEventCallback(node->_dnsErrorCode, node->_addrinfo, arg);
    node->_dnsErrorCode = 0;
  }
  return;
}
#endif

/**
 * @brief: 进行DNS获得IP后开始链接
 * @return:
 */
void WorkThread::dnsEventCallback(int errorCode,
                                  struct evutil_addrinfo *address, void *arg) {
  ConnectNode *node = static_cast<ConnectNode *>(arg);
  NlsClientImpl *client = _instance;
  NlsNodeManager *node_manager = client->getNodeManger();
  int status = NodeStatusInvalid;
  int ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d.", ret);
    return;
  } else {
    if (status >= NodeStatusCancelling) {
      LOG_WARN(
          "Node(%p) checkNodeExist failed, status:%s, node status:%s, do "
          "nothing later...",
          node, node->getConnectNodeStatusString().c_str(),
          node_manager->getNodeStatusString(status).c_str());
      // maybe mem leak here
      destroyConnectNode(node);
      return;
    }
  }

  WorkThread *pThread = node->getEventThread();
  if (pThread == NULL) {
    LOG_ERROR("The WorkThread of Node(%p) is nullptr, skipping ...", node);
    return;
  }
  node->_dnsRequestCallbackStatus = 1;
  node->_inEventCallbackNode = true;
  if (errorCode) {
    LOG_ERROR("WorkThread(%p) Node(%p) %s dns failed: %s.", pThread, node,
              node->getUrlAddress()._host, evutil_gai_strerror(errorCode));
    node->setConnectNodeStatus(NodeConnecting);
    if (node->dnsProcess(node->getEventThread()->_addrInFamily,
                         node->getEventThread()->_directIp,
                         node->getEventThread()->_enableSysGetAddr) < 0) {
      destroyConnectNode(node);
    }

    // check node again!!!
    ret = node_manager->checkNodeExist(node, &status);
    if (ret != Success) {
      LOG_ERROR("checkNodeExist failed, ret:%d.", ret);
      return;
    }
#ifdef _MSC_VER
    SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
    SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                     node->_inEventCallbackNode);
#endif
    node->_dnsRequestCallbackStatus = 2;
    return;
  }

  if (address->ai_canonname) {
    LOG_DEBUG("WorkThread(%p) Node(%p) ai_canonname: %s", pThread, node,
              address->ai_canonname);
  }

  struct evutil_addrinfo *ai;
  for (ai = address; ai; ai = ai->ai_next) {
    char buffer[HostSize] = {0};
    const char *ip = NULL;
    if (ai->ai_family == AF_INET) {
      struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
      ip = evutil_inet_ntop(AF_INET, &sin->sin_addr, buffer, HostSize);

      if (ip) {
        LOG_DEBUG("WorkThread(%p) Node(%p) IpV4:%s.", pThread, node, ip);
#ifdef ENABLE_DNS_IP_CACHE
        node->getEventThread()->setIpCache(
            (char *)node->getRequest()->getRequestParam()->_url.c_str(),
            (char *)ip);
#endif

        int ret = node->connectProcess(ip, AF_INET);
        if (ret == 0) {
          ret = node->sslProcess();
          if (ret == 0) {
            LOG_DEBUG("WorkThread(%p) Node(%p) begin gateway request process.",
                      pThread, node);
            if (nodeRequestProcess(node) < 0) {
              destroyConnectNode(node);
            }

            // check node again!!!
            ret = node_manager->checkNodeExist(node, &status);
            if (ret != Success) {
              LOG_ERROR("checkNodeExist failed, ret:%d.", ret);
              return;
            }
#ifdef _MSC_VER
            SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
            SEND_COND_SIGNAL(node->_mtxEventCallbackNode,
                             node->_cvEventCallbackNode,
                             node->_inEventCallbackNode);
#endif
            node->_dnsRequestCallbackStatus = 2;
            return;
          }
        }

        if (ret == 1) {
          LOG_DEBUG(
              "WorkThread(%p) Node(%p) connectProcess or sslProcess return 1, "
              "will try "
              "connect ...",
              pThread, node);
          // connect EINPROGRESS
          break;
        } else {
          LOG_DEBUG("WorkThread(%p) Node(%p) goto ConnectRetry, ret:%d.",
                    pThread, node, ret);
          goto ConnectRetry;
        }
      }

    } else if (ai->ai_family == AF_INET6) {
      struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
      ip = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buffer, HostSize);

      if (ip) {
        LOG_DEBUG("WorkThread(%p) Node(%p) IpV6:%s.", pThread, node, ip);
#ifdef ENABLE_DNS_IP_CACHE
        node->getEventThread()->setIpCache(
            (char *)node->getRequest()->getRequestParam()->_url.c_str(),
            (char *)ip);
#endif

        int ret = node->connectProcess(ip, AF_INET6);
        if (ret == 0) {
          LOG_DEBUG("WorkThread(%p) Node(%p) begin ssl process.", pThread,
                    node);
          ret = node->sslProcess();
          if (ret == 0) {
            LOG_DEBUG("WorkThread(%p) Node(%p) begin gateway request process.",
                      pThread, node);
            if (nodeRequestProcess(node) < 0) {
              destroyConnectNode(node);
            }

            // check node again!!!
            ret = node_manager->checkNodeExist(node, &status);
            if (ret != Success) {
              LOG_ERROR("checkNodeExist failed, ret:%d.", ret);
              return;
            }
#ifdef _MSC_VER
            SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
            SEND_COND_SIGNAL(node->_mtxEventCallbackNode,
                             node->_cvEventCallbackNode,
                             node->_inEventCallbackNode);
#endif
            node->_dnsRequestCallbackStatus = 2;
            return;
          }
        }

        if (ret == 1) {
          LOG_DEBUG(
              "WorkThread(%p) Node(%p) connectProcess return 1, will try "
              "connect ...",
              pThread, node);
          break;
        } else {
          LOG_DEBUG("WorkThread(%p) Node(%p) goto ConnectRetry.", pThread,
                    node);
          goto ConnectRetry;
        }
      }
    }
  }  // for

  evutil_freeaddrinfo(address);

  // check node again!!!
  ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d.", ret);
    return;
  }
#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif
  node->_dnsRequestCallbackStatus = 2;
  return;

ConnectRetry:
  evutil_freeaddrinfo(address);
  node->disconnectProcess();
  node->setConnectNodeStatus(NodeConnecting);
  if (node->dnsProcess(node->getEventThread()->_addrInFamily,
                       node->getEventThread()->_directIp,
                       node->getEventThread()->_enableSysGetAddr) < 0) {
    destroyConnectNode(node);
  }

  // check node again!!!
  ret = node_manager->checkNodeExist(node, &status);
  if (ret != Success) {
    LOG_ERROR("checkNodeExist failed, ret:%d.", ret);
    return;
  }
#ifdef _MSC_VER
  SET_EVENT(node->_inEventCallbackNode, node->_mtxEventCallbackNode);
#else
  SEND_COND_SIGNAL(node->_mtxEventCallbackNode, node->_cvEventCallbackNode,
                   node->_inEventCallbackNode);
#endif
  node->_dnsRequestCallbackStatus = 2;
  return;
}

/**
 * @brief: 开始gateway的请求处理
 * @return: 成功则Success, 失败则返回负值.
 */
int WorkThread::nodeRequestProcess(ConnectNode *node) {
  int ret = Success;
  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    return -(NodeEmpty);
  }
  NlsNodeManager *node_manager = node->getInstance()->getNodeManger();
  int status = NodeStatusInvalid;
  int result = node_manager->checkNodeExist(node, &status);
  if (result != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", node, result);
    return result;
  }

  ConnectStatus workStatus = node->getConnectNodeStatus();
  // LOG_DEBUG("Node(%p) workStatus %d(%s).",
  //     node, workStatus, node->getConnectNodeStatusString().c_str());
  switch (workStatus) {
    /*connect to gateWay*/
    case NodeHandshaking:
      node->gatewayRequest();
      ret = node->nlsSendFrame(node->getCmdEvBuffer());
      node->setConnectNodeStatus(NodeHandshaked);
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
    LOG_ERROR("Node(%p) Send failed, ret:%d.", node, ret);

    std::string failedInfo = node->getErrorMsg();
    if (failedInfo.empty()) {
      char tmp_msg[512] = {0};
      snprintf(tmp_msg, 512 - 1,
               "workThread workStatus(%s) Send failed. error_code(%d)",
               node->getConnectNodeStatusString().c_str(), ret);
      failedInfo.assign(tmp_msg);
    }
    node->handlerTaskFailedEvent(failedInfo);
    node->closeConnectNode();
    return ret;
  }

  // LOG_DEBUG("Node(%p) nodeResquestProcess done.", node);
  return Success;
}

/**
 * @brief: 接收gateway的响应
 * @return: 成功则Success, 失败则返回负值.
 */
int WorkThread::nodeResponseProcess(ConnectNode *node) {
  int ret = Success;

  // LOG_DEBUG("Node(%p) nodeResponseProcess begin ...", node);

  if (node == NULL) {
    LOG_ERROR("node is nullptr.");
    return -(NodeEmpty);
  }
  NlsNodeManager *node_manager = node->getInstance()->getNodeManger();
  int status = NodeStatusInvalid;
  int result = node_manager->checkNodeExist(node, &status);
  if (result != Success) {
    LOG_ERROR("Node(%p) checkNodeExist failed, result:%d.", node, result);
    return result;
  }
  if (node->_releasingFlag) {
    LOG_ERROR("Node(%p) is releasing!!! skipping ...", node);
    return -(InvalidStatusWhenReleasing);
  }

  ConnectStatus workStatus = node->getConnectNodeStatus();
  // LOG_DEBUG("Node(%p) current node status:%s.",
  //     node, node->getConnectNodeStatusString().c_str());
  switch (workStatus) {
    /*connect to gateway*/
    case NodeHandshaking:
    case NodeHandshaked:
      ret = node->gatewayResponse();
      if (ret == 0) {
        /* ret == 0 mean parsing response successfully */
        node->setConnectNodeStatus(NodeStarting);
        if (node->getRequest()->getRequestParam()->_requestType ==
            SpeechTextDialog) {
          node->addCmdDataBuffer(CmdTextDialog);
        } else {
          node->addCmdDataBuffer(CmdStart);
        }
        ret = node->nlsSendFrame(node->getCmdEvBuffer());
        if (ret >= 0) {
          node->sendFakeSynthesisStarted();
        }
      } else if (ret == -(NlsReceiveEmpty)) {
        LOG_WARN("Node(%p) nlsReceive empty, try again...", node);
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
      } else if (workStatus == NodeWakeWording) {
        ret = node->nlsSendFrame(node->getWwvEvBuffer());
      }
      break;

    case NodeStarted:
      ret = node->webSocketResponse();
      break;

    case NodeConnecting:
      if (node->isLongConnection()) {
        /*
         * 在长链接模式下, 可能存在进入NodeConnecting而非NodeStarted状态的情况
         * 以NodeStarted来处理......
         */
        LOG_WARN("Node(%p) NodeConnecting is abnormal.", node);
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
    if (ret == -(EventClientEmpty)) {
      LOG_ERROR("Instance has released, skip all operation.");
      return ret;
    }
    if (ret == -(InvalidStatusWhenReleasing)) {
      LOG_ERROR("Node(%p) is releasing, skip all operation.", node);
      return ret;
    }

    if (NodeClosed == node->getConnectNodeStatus()) {
      LOG_WARN(
          "Node(%p) current node status is NodeClosed, please ignore this "
          "warn.",
          node);
      return Success;
    }
    LOG_ERROR("Node(%p) response failed, ret:%d.", node, ret);

    std::string failedInfo = node->getErrorMsg();
    if (failedInfo.empty()) {
      char tmp_msg[512] = {0};
      snprintf(tmp_msg, 512 - 1,
               "workThread workStatus(%s) Response failed. error_code(%d)",
               node->getConnectNodeStatusString().c_str(), ret);
      failedInfo.assign(tmp_msg);
    }
    if (ret == -(InvalidRequest)) {
      LOG_ERROR(
          "Node(%p) Response failed, errormsg:%s. But request has released, "
          "ignore TaskFailed and Closed event.",
          node, failedInfo.c_str());
    } else {
      node->closeConnectNode();
      node->handlerTaskFailedEvent(failedInfo);
    }
  }

  // LOG_DEBUG("Node(%p) nodeResponseProcess done.", node);

  return ret;
}

void WorkThread::setAddrInFamily(int aiFamily) { _addrInFamily = aiFamily; }

void WorkThread::setDirectHost(char *directIp) {
  memset(_directIp, 0, 64);
  if (directIp && strnlen(directIp, 64) > 0) {
    strncpy(_directIp, directIp, 64);
  }
}

void WorkThread::setUseSysGetAddrInfo(bool enable) {
  _enableSysGetAddr = enable;
}

void WorkThread::setInstance(NlsClientImpl *instance) { _instance = instance; }

#ifdef ENABLE_DNS_IP_CACHE
std::string WorkThread::getIpFromCache(char *host) {
  MUTEX_LOCK(_mtxList);
  std::string ip_str = "";
  if (host != NULL) {
    if (WebSocketTcp::urlWithAccess(host)) {
      LOG_DEBUG("Using special host, without IpCache.");
      MUTEX_UNLOCK(_mtxList);
      return ip_str;
    }

    std::string host_str(host);
    std::map<std::string, struct DnsIpCache>::iterator iter;
    iter = _dnsIpCache.find(host_str);
    if (iter != _dnsIpCache.end()) {
      // find all IP info of this host
      struct DnsIpCache ips = iter->second;
      uint32_t count = ips.ip_list.size();
      if (ips.same_ip_count < DnsIpCache::WorkThreshold) {
        LOG_DEBUG("Host(%s) try to get more IPs. (%d/%d)", host,
                  ips.same_ip_count, DnsIpCache::WorkThreshold);
      } else {
        if (count > 0) {
          int index = rand() % count;
          ip_str = ips.ip_list[index];
          LOG_INFO("Get Ip %s from host(%s) %d/%d.", ip_str.c_str(), host,
                   index, count);
        } else {
          LOG_ERROR("Host(%s) is empty.", host);
        }
      }
    }
  }
  MUTEX_UNLOCK(_mtxList);
  return ip_str;
}

void WorkThread::setIpCache(char *host, char *ip) {
  MUTEX_LOCK(_mtxList);
  if (host == NULL || ip == NULL) {
    LOG_INFO("Clear _dnsIpCache");
    for (std::map<std::string, struct DnsIpCache>::iterator iter =
             _dnsIpCache.begin();
         iter != _dnsIpCache.end(); ++iter) {
      iter->second.ip_list.clear();
    }
    _dnsIpCache.clear();
  } else {
    std::string host_str(host);
    std::string ip_str(ip);
    std::map<std::string, struct DnsIpCache>::iterator iter;
    iter = _dnsIpCache.find(host_str);
    if (iter != _dnsIpCache.end()) {
      // find all IP info of this host
      struct DnsIpCache ips = iter->second;
      uint32_t count = ips.ip_list.size();
      if (count > 0) {
        std::vector<std::string>::iterator ip_iter;
        ip_iter = find(ips.ip_list.begin(), ips.ip_list.end(), ip_str);
        if (ip_iter == ips.ip_list.end()) {
          iter->second.ip_list.push_back(ip_str);
          iter->second.same_ip_count = 0;
          LOG_INFO("Push new ip(%s) into host(%s) cache", ip_str.c_str(),
                   host_str.c_str());
        } else {
          iter->second.same_ip_count++;
        }
      } else {
        LOG_ERROR("Host(%s) is empty.", host);
      }
    } else {
      // cannot find address
      struct DnsIpCache ip_cache;
      ip_cache.ip_list.push_back(ip_str);
      _dnsIpCache.insert(std::make_pair(host_str, ip_cache));
      LOG_INFO("New IP cache by host(%s) and ip(%s).", host_str.c_str(),
               ip_str.c_str());
    }
  }
  MUTEX_UNLOCK(_mtxList);
}
#endif

void WorkThread::updateParameters(ConnectNode *node) {
  if (node) {
    if (_dnsBase) {
      time_t timeout_ms = node->getRequest()->getRequestParam()->getTimeout();
      float timeout_sec = (float)timeout_ms / 1000;
      std::string timeout_sec_str = utility::TextUtils::to_string(timeout_sec);
      LOG_DEBUG("WorkThread(%p) evdns_base setting timeout %s seconds.", this,
                timeout_sec_str.c_str());
      evdns_base_set_option(_dnsBase, "timeout", timeout_sec_str.c_str());
    }
  }
}

}  // namespace AlibabaNls
