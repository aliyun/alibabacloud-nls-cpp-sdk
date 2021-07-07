/*
 * Copyright 2015 Alibaba Group Holding Limited
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

#if defined(__ANDROID__) || defined(__linux__)
//#define _GNU_SOURCE
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sched.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#include <process.h>
#endif

#include <string>
#include <algorithm>
#include "workThread.h"
#include "iNlsRequest.h"
#include "iNlsRequestParam.h"
#include "connectNode.h"
#include "log.h"
#include "utility.h"

namespace AlibabaNls {

#define HOST_SIZE 256

using std::string;
using std::vector;
using std::list;
using namespace utility;

#if defined(_WIN32)
//HANDLE WorkThread::_mtxList = CreateMutex(NULL, FALSE, NULL);
#else
pthread_mutex_t WorkThread::_mtxCpu = PTHREAD_MUTEX_INITIALIZER;
#endif

int WorkThread::_cpuNumber = 1;
int WorkThread::_cpuCurrent = 0;

WorkThread::WorkThread() {
    LOG_DEBUG("Create WorkThread.");

#if defined(_WIN32)
    _mtxList = CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_init(&_mtxList, NULL);
#endif

//    struct event_config *cfg;
//    cfg = event_config_new();
//    event_config_avoid_method(cfg, "epoll");
//    event_config_require_features(cfg, EV_FEATURE_ET);

//    _workBase = event_base_new_with_config(cfg);
//    event_config_free(cfg);

    _workBase = event_base_new();

    if (!_workBase) {
        LOG_ERROR("event_base_new failed.");
        exit(1);
    }

#if !defined(__APPLE__)
    _dnsBase = evdns_base_new(_workBase, 1);

    if(NULL == _dnsBase) {
        LOG_ERROR("evdns_base_new failed.");
        exit(1);
    }

//    if(evdns_base_set_option(_dnsBase, "timeout:", "1") != 0) {
//        LOG_ERROR("evdns_base_set_option timeout failed.");
//    }
//
//    if(evdns_base_set_option(_dnsBase, "max-timeouts:", "2") != 0) {
//        LOG_ERROR("evdns_base_set_option max-timeout failed.");
//    }

#endif

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

#if defined(_WIN32)
	_workThreadHandle = (HANDLE)_beginthreadex(NULL, 0, loopEventCallback, (LPVOID)this, 0, &_workThreadId);
	CloseHandle(_workThreadHandle);
#else
//    pthread_attr_t attr;
//    pthread_attr_init(&attr);
//    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&_workThreadId, NULL, loopEventCallback, (void*)this);
//    pthread_attr_destroy(&attr);
#endif

    LOG_INFO("WorkThread start working.");
}

WorkThread::~WorkThread() {
	size_t count = 0;

//    LOG_INFO("Begin destroy WorkThread list:%p  %d.", this, _nodeList.size());
    //must check asr is end
	do {
#if defined(_WIN32)
		Sleep(10);
		WaitForSingleObject(_mtxList, INFINITE);
#else
		usleep(10 * 1000);
		pthread_mutex_lock(&_mtxList);
#endif

        std::list<INlsRequest*>::iterator itList;
        for( itList = _nodeList.begin(); itList != _nodeList.end();) {
            INlsRequest* request = *itList;
            ConnectStatus cStatus = request->getConnectNode()->getConnectNodeStatus();
            ExitStatus eStatus = request->getConnectNode()->getExitStatus();
            if(cStatus == NodeInvalid || cStatus == NodeInitial || eStatus == ExitStopped) {
                _nodeList.erase(itList++);
                delete request;
            } else {
                itList++;
            }
        }

        count = _nodeList.size();

#if defined(_WIN32)
		ReleaseMutex(_mtxList);
#else
			pthread_mutex_unlock(&_mtxList);
#endif
	} while (count > 0);

    evutil_closesocket(_notifySendFd);
    evutil_closesocket(_notifyReceiveFd);

    event_del(&_notifyEvent);

    event_base_loopbreak(_workBase);
//    struct timeval tv;
//    tv.tv_sec = 0;
//    tv.tv_usec = 100000;
//    event_base_loopexit(_workBase, &tv);

//#if !defined(__APPLE__)
//    evdns_base_free(_dnsBase, 0);
//#endif

//    event_base_free(_workBase);

#if defined(_WIN32)
	CloseHandle(_mtxList);
#else

//    LOG_INFO("Done destroy WorkThread Begin join:%p.", this);

    pthread_join(_workThreadId, NULL);
	pthread_mutex_destroy(&_mtxList);
#endif

    LOG_INFO("Destroy WorkThread done.");
}

void WorkThread::insertQueueNode(WorkThread* thread, INlsRequest * request) {
#if defined(_WIN32)
    WaitForSingleObject(thread->_mtxList, INFINITE);
#else
    pthread_mutex_lock(&(thread->_mtxList));
#endif

    thread->_nodeQueue.push(request);

#if defined(_WIN32)
    ReleaseMutex(thread->_mtxList);
#else
    pthread_mutex_unlock(&(thread->_mtxList));
#endif
}

INlsRequest* WorkThread::getQueueNode(WorkThread* thread) {
#if defined(_WIN32)
    WaitForSingleObject(thread->_mtxList, INFINITE);
#else
    pthread_mutex_lock(&(thread->_mtxList));
#endif

    INlsRequest *request = thread->_nodeQueue.front();
    thread->_nodeQueue.pop();

#if defined(_WIN32)
    ReleaseMutex(thread->_mtxList);
#else
    pthread_mutex_unlock(&(thread->_mtxList));
#endif

    return request;
}

void WorkThread::insertListNode(WorkThread* thread, INlsRequest * request) {
#if defined(_WIN32)
	WaitForSingleObject(thread->_mtxList, INFINITE);
#else
	pthread_mutex_lock(&(thread->_mtxList));
#endif

	thread->_nodeList.push_back(request);

#if defined(_WIN32)
	ReleaseMutex(thread->_mtxList);
#else
	pthread_mutex_unlock(&(thread->_mtxList));
#endif

	return;
}

void WorkThread::freeListNode(WorkThread* thread, INlsRequest* request) {
#if defined(_WIN32)
	WaitForSingleObject(thread->_mtxList, INFINITE);
#else
	pthread_mutex_lock(&(thread->_mtxList));
#endif

    list<INlsRequest *>::iterator iLocation = find(thread->_nodeList.begin(), thread->_nodeList.end(), request);

    if (iLocation != thread->_nodeList.end()) {
        thread->_nodeList.remove(*iLocation);
        LOG_DEBUG("List requests :%d.", thread->_nodeList.size());
    }

#if defined(_WIN32)
	ReleaseMutex(thread->_mtxList);
#else
	pthread_mutex_unlock(&(thread->_mtxList));
#endif
}

void WorkThread::destroyConnectNode(ConnectNode* node) {

    if (node == NULL) {
        LOG_DEBUG("Input node is null.");
        return ;
    }

    LOG_INFO("Node:%p FreeConnectNode begin.", node);
    freeListNode(node->_eventThread, node->_request);
    if (node->updateDestroyStatus()) {
        LOG_INFO("Node:%p DestroyConnectNode done.", node);
        INlsRequest* request = node->_request;
        delete request;
        request = NULL;
    }

    LOG_INFO("Node:%p FreeConnectNode done.", node);

    return ;
}

#if defined(_WIN32)
unsigned __stdcall WorkThread::loopEventCallback(LPVOID arg) {
#else
void* WorkThread::loopEventCallback(void* arg) {
#endif

    WorkThread *eventParam = (WorkThread*)arg;

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

    if(pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
        LOG_ERROR("pthread_sigmask failed.");
        exit(1);
    }

//    cpu_set_t cpuset;
//    pthread_mutex_lock(&_mtxCpu);
//    if (_cpuCurrent >= _cpuNumber) {
//        _cpuCurrent = 0;
//    }
//
//    CPU_ZERO(&cpuset);
//    CPU_SET(_cpuCurrent, &cpuset);
//
//    LOG_ERROR("Bind: CPU No.%d", _cpuCurrent);
//
//    _cpuCurrent ++;
//    pthread_mutex_unlock(&_mtxCpu);
//
//    //bind process to processor 0
//    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) <0) {
//        LOG_ERROR("pthread_setaffinity_np failed.");
//    }

    prctl(PR_SET_NAME, "eventThread");
#endif

//    LOG_ERROR("event_base_dispatch begin.", _cpuCurrent);
    event_base_dispatch(eventParam->_workBase);
//    LOG_ERROR("event_base_dispatch done.", _cpuCurrent);

#if !defined(__APPLE__)
    evdns_base_free(eventParam->_dnsBase, 0);
#endif

    event_base_free(eventParam->_workBase);

#if defined(_WIN32)
    return 0;
#else
    return NULL;
#endif

}

void WorkThread::connectEventCallback(evutil_socket_t socketFd , short event, void *arg) {
    int errorCode = 0;
    ConnectNode *node = (ConnectNode*)arg;

    if (event == EV_TIMEOUT) {
        LOG_DEBUG("Node:%p connect EV_TIMEOUT.", node);
        goto EventProcessFailed;
    } else if (event == EV_CLOSED) {
        LOG_DEBUG("Node:%p connect EV_CLOSED.", node);
        goto EventProcessFailed;
    } else {
//        LOG_DEBUG("Node:%p Connect Event %02x.", node, event);
        if (node->getConnectNodeStatus() == NodeConnecting) {
            socklen_t len = sizeof(errorCode);
            getsockopt(socketFd, SOL_SOCKET, SO_ERROR, (char *) &errorCode, &len);
            if (!errorCode) {
                LOG_INFO("Node:%p connect return ev_write, check ok.", node);
                node->setConnectNodeStatus(NodeConnected);
            } else {
                if (node->socketConnect() == -1) {
                    goto EventProcessFailed;
                }
            }
        }

        if (node->getConnectNodeStatus() == NodeConnected) {
            int ret = node->sslProcess();
            switch (ret) {
                case 0:
                    LOG_INFO("Node:%p Begin gateway request process.", node);
                    if (nodeRequestProcess(node) == -1) {
                        destroyConnectNode(node);
                    }
                    break;
                case 1:
//                LOG_DEBUG("wait connect.");
                    break;
                default:
                    goto EventProcessFailed;
            }
        }
    }
    return ;

EventProcessFailed:

    LOG_ERROR("Node:%p Connect failed:%s.", node, evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

//    node->closeConnectNode();
    node->disconnectProcess();
    node->setConnectNodeStatus(NodeConnecting);

    if (node->dnsProcess() == -1) {
        LOG_ERROR("Node:%p try delete request.", node);
        destroyConnectNode(node);
    }

    return ;
}

void WorkThread::readEventCallBack(evutil_socket_t socketFd, short what, void *arg) {
    ConnectNode *node = (ConnectNode*)arg;
    char tmp_msg[512] = {0};

    if (what == EV_READ){
        nodeResponseProcess(node);
    } else if (what == EV_TIMEOUT){
        LOG_INFO("Node:%p Recv timeout.", node);

        snprintf(tmp_msg, 512 - 1, "Recv timeout. %s.", evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

        node->handlerTaskFailedEvent(tmp_msg);
        node->closeConnectNode();
    }
//    else if (what == EV_CLOSED){
//        LOG_DEBUG("Node:%p Connect Node is closed.", node);
//        node->closeConnectNode();
//    }
    else {
        LOG_ERROR("Node:%p Unknown event:%02x.", node, what);

        snprintf(tmp_msg, 512 - 1, "{\"TaskFailed\":\"Unknown event:%02x. %s\"}", what, evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));
        node->handlerTaskFailedEvent(tmp_msg);
        node->closeConnectNode();
    }

    if (node->getConnectNodeStatus() == NodeInvalid) {
        destroyConnectNode(node);
    }
}

void WorkThread::writeEventCallBack(evutil_socket_t socketFd, short what, void *arg) {
    char tmp_msg[512] = {0};
    ConnectNode *node = (ConnectNode*)arg;

    if (what == EV_WRITE){
        nodeRequestProcess(node);
    } else if (what == EV_TIMEOUT){
        LOG_DEBUG("Node:%p Send timeout.", node);

        snprintf(tmp_msg, 512 - 1, "{\"TaskFailed\":\"%s\"}", evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

        node->handlerTaskFailedEvent(tmp_msg);
        node->closeConnectNode();
    } else {
        LOG_ERROR("Node:%p Unknown event:%02x.", node, what);

        snprintf(tmp_msg, 512 - 1, "{\"TaskFailed\":\"Unknown event:%02x. %s\"}", what, evutil_socket_error_to_string(evutil_socket_geterror(node->_socketFd)));

        node->handlerTaskFailedEvent(tmp_msg);
        node->closeConnectNode();
    }

    if (node->getConnectNodeStatus() == NodeInvalid) {
        destroyConnectNode(node);
    }
}

#if defined(__APPLE__)
void WorkThread::iosDnsEvent(ConnectNode *node, const char* ip, int aiFamily) {
    int ret = node->connectProcess(ip, aiFamily);
    if (ret == 0) {
        ret = node->sslProcess();
        if (ret == 0) {
            LOG_DEBUG("Node:%p Begin gateway request process.", node);
            if (nodeRequestProcess(node) == -1) {
                destroyConnectNode(node);
            }
            return ;
        }
    }

    if (ret == 1) {
        LOG_DEBUG("Node:%p Add connect event.", node);
        return ;
    } else {
//        node->closeConnectNode();
        node->disconnectProcess();
        node->setConnectNodeStatus(NodeConnecting);
        if (node->dnsProcess() == -1) {
            destroyConnectNode(node);
        }
    }

    return ;
}

#else

void WorkThread::dnsEventCallback(int errorCode,
                                  struct evutil_addrinfo *address,
                                  void *arg) {
	ConnectNode *node = (ConnectNode *)arg;

    if (errorCode) {
        LOG_ERROR("Node:%p %s dns failed: %s.", node, node->_url._host, evutil_gai_strerror(errorCode));
        node->setConnectNodeStatus(NodeConnecting);
        if (node->dnsProcess() == -1) {
            destroyConnectNode(node);
        }
        return ;
    }

    struct evutil_addrinfo  *ai;

    if (address->ai_canonname) {
        LOG_INFO("Node:%p ai_canonname: %s", node, address->ai_canonname);
    }

    for (ai = address; ai; ai = ai->ai_next) {
        char buffer[HOST_SIZE] = {0};
        const char *ip = NULL;
        if (ai->ai_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
            ip = evutil_inet_ntop(AF_INET, &sin->sin_addr, buffer, HOST_SIZE);

            if (ip) {
                LOG_INFO("Node:%p IpV4:%s", node, ip);

                int ret = node->connectProcess(ip, AF_INET);
                if (ret == 0) {
                    ret = node->sslProcess();
                    if (ret == 0) {
                        LOG_DEBUG("Node:%p Begin gateway request process.", node);
                        if (nodeRequestProcess(node) == -1) {
                            destroyConnectNode(node);
                        }

                        return ;
                    }
                }

                if (ret == 1) {
                    break;
                } else {
                    goto ConnectRetry;
                }
            }

        } else if (ai->ai_family == AF_INET6) {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
            ip = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buffer, HOST_SIZE);

            if (ip) {
                LOG_INFO("Node:%p IpV6:%s", node, ip);

                int ret = node->connectProcess(ip, AF_INET6);
                if (ret == 0) {
                    LOG_DEBUG("Node:%p Begin ssl process.", node);
                    ret = node->sslProcess();
                    if (ret == 0) {
                        LOG_DEBUG("Node:%p Begin gateway request process.", node);
                        if (nodeRequestProcess(node) == -1) {
                            destroyConnectNode(node);
                        }
                        return ;
                    }
                }

                if (ret == 1) {
                    break;
                } else {
                    goto ConnectRetry;
                }
            }
        }
    }

    evutil_freeaddrinfo(address);

    return ;

ConnectRetry:
    evutil_freeaddrinfo(address);
//    node->closeConnectNode();
    node->disconnectProcess();
    node->setConnectNodeStatus(NodeConnecting);
    if (node->dnsProcess() == -1) {
        destroyConnectNode(node);
    }

    return ;
}
#endif

void WorkThread::notifyEventCallback(evutil_socket_t fd, short which, void *arg) {
    WorkThread *pThread = (WorkThread*)arg;
    char msgCmd;
    if (recv(pThread->_notifyReceiveFd, (char *)&msgCmd, sizeof(char), 0) <= 0) {
        LOG_ERROR("work Thread recv() failed:%d.", getLastErrorCode());
        return;
    }
    LOG_INFO("work Thread receive: '%c' from main thread.", msgCmd);

    if (msgCmd == 'c') {
        INlsRequest *request = getQueueNode(pThread);
        insertListNode(pThread, request);

        LOG_DEBUG("Node:%p begin dnsprocess.", request->getConnectNode());

        if (request->getConnectNode()->dnsProcess() == -1) {
            destroyConnectNode(request->getConnectNode());
        }
    } else if (msgCmd == 's') {
      event_base_loopbreak(pThread->_workBase);
    } else {
        LOG_ERROR("work Thread recv:'%c'.", msgCmd);
    }

    return ;
}

int WorkThread::nodeRequestProcess(ConnectNode* node) {
    int ret = 0;

//    LOG_DEBUG("Node:%p nodeResquestProcess begin.", node);

    //invoke cancel()
    if (node->getExitStatus() == ExitCancel) {
        node->closeConnectNode();
        return -1;
    }

    ConnectStatus workStatus = node->getConnectNodeStatus();
    LOG_DEBUG("Node:%p workStatus %d.\n", node, workStatus);
    switch(workStatus) {
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
            if (ret == 0) {
                ret = node->sendControlDirective();
            }
            break;

        default:
            ret = -1;
            break;
    }

    if (ret < 0) {
        LOG_ERROR("Node:%p Send failed.\n", node);

        node->handlerTaskFailedEvent(node->getErrorMsg());
        node->closeConnectNode();
        return -1;
    }

//    LOG_DEBUG("Node:%p nodeResquestProcess done.", node);

    return 0;
}

int WorkThread::nodeResponseProcess(ConnectNode* node) {
    int ret = 0;

//    LOG_DEBUG("Node:%p nodeResponseProcess begin.", node);

    //invoke cancel()
    if (node->getExitStatus() == ExitCancel) {
        node->closeConnectNode();
        return -1;
    }

    ConnectStatus workStatus = node->getConnectNodeStatus();
    LOG_DEBUG("Node:%p workStatus %d.\n", node, workStatus);
    switch(workStatus) {
        /*connect to gateWay*/
        case NodeHandshaking:
        case NodeHandshaked:
            ret = node->gatewayResponse();
            if (ret == 0) {
                node->setConnectNodeStatus(NodeStarting);

                if (node->_request->getRequestParam()->_requestType == SpeechTextDialog) {
                    node->addCmdDataBuffer(CmdTextDialog);
                } else {
                    node->addCmdDataBuffer(CmdStart);
                }

                ret = node->nlsSendFrame(node->getCmdEvBuffer());
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

        default:
            ret = -1;
            break;
    }

    if (ret == -1) {
        LOG_ERROR("Node:%p Response failed.\n", node);

        node->handlerTaskFailedEvent(node->getErrorMsg());
        node->closeConnectNode();
    }

//    LOG_DEBUG("Node:%p nodeResponseProcess done.", node);

    return 0;
}

//void WorkThread::stopWorkThread() {
//    char content = 's';
//
//    LOG_DEBUG("Send stopWorkThread command.");
//
//   
//    write(_notifySendFd, &content, sizeof(char));
//
//    return ;
//}

}



