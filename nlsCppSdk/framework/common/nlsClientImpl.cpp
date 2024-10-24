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
#include "SSLconnect.h"
#include "connectNode.h"
#include "da/dialogAssistantRequest.h"
#include "nlog.h"
#include "nlsClientImpl.h"
#include "nlsEventNetWork.h"

#include "fss/flowingSynthesizerRequest.h"
#include "sr/speechRecognizerRequest.h"
#include "st/speechTranscriberRequest.h"
#include "sy/speechSynthesizerRequest.h"
#include "utility.h"

#ifdef ENABLE_VIPSERVER
//引入VipClientApi相关的头文件
#include "iphost.h"
#include "option.h"
#include "vipclient.h"
#include "vipclient_helper.hpp"

using namespace middleware::vipclient;
#endif

namespace AlibabaNls {

bool NlsClientImpl::_isInitializeSSL = false;
bool NlsClientImpl::_isInitializeThread = false;
#ifdef ENABLE_VIPSERVER
bool NlsClientImpl::_isInitalizeVsClient = false;
pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsClientImpl::NlsClientImpl(bool sslInitial)
    : _nodeManager(NULL),
      _aiFamily(),
      _directHostIp(),
      _enableSysGetAddr(false),
      _syncCallTimeoutMs(0) {
  strncpy(_aiFamily, "AF_INET", 16);

  // init openssl
  if (sslInitial) {
    if (!_isInitializeSSL) {
      SSLconnect::init();
      _isInitializeSSL = sslInitial;
    }
  }

  _nodeManager = new NlsNodeManager();

#if defined(_MSC_VER)
  _mtxReleaseRequestGuard = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxReleaseRequestGuard, NULL);
#endif

#ifdef ENABLE_VIPSERVER
  VipClientApi::CreateApi();
#endif
}

NlsClientImpl::~NlsClientImpl() {
#if defined(_MSC_VER)
  CloseHandle(_mtxReleaseRequestGuard);
#else
  pthread_mutex_destroy(&_mtxReleaseRequestGuard);
#endif
}

void NlsClientImpl::releaseInstanceImpl() {
  LOG_INFO("Release NlsClientImpl instance:%p.", this);

  if (_isInitializeThread) {
    if (NlsEventNetWork::_eventClient != NULL) {
      NlsEventNetWork::_eventClient->destroyEventNetWork();
      delete NlsEventNetWork::_eventClient;
      NlsEventNetWork::_eventClient = NULL;
    }
    _isInitializeThread = false;
  }

  if (_isInitializeSSL) {
    SSLconnect::destroy();
    _isInitializeSSL = false;
  }

#ifdef ENABLE_VIPSERVER
  LOG_INFO("destroy VipClient.");
  if (_isInitalizeVsClient) {
    VipClientApi::UnInit();
    _isInitalizeVsClient = false;
  }
  VipClientApi::DestoryApi();
#endif

  /* find NlsClientImp in map and erase it */
  int ret = _nodeManager->removeInstanceFromInfo(this);
  if (ret != Success) {
    LOG_ERROR("removeInstanceFromInfo instance(%p) failed, ret:%d", this, ret);
    return;
  }

  delete _nodeManager;
  _nodeManager = NULL;

  LOG_INFO("destroy log instance.");
  utility::NlsLog::destroyLogInstance();  // donnot LOG_XXX after here
}

NlsNodeManager *NlsClientImpl::getNodeManger() { return _nodeManager; }

void NlsClientImpl::setAddrInFamilyImpl(const char *aiFamily) {
  if (aiFamily != NULL && (strncmp(aiFamily, "AF_INET", 16) == 0 ||
                           strncmp(aiFamily, "AF_INET6", 16) == 0 ||
                           strncmp(aiFamily, "AF_UNSPEC", 16) == 0)) {
    memset(_aiFamily, 0, 16);
    strncpy(_aiFamily, aiFamily, 16);
  }
}

void NlsClientImpl::setUseSysGetAddrInfoImpl(bool enable) {
  _enableSysGetAddr = enable;
}

void NlsClientImpl::setSyncCallTimeoutImpl(unsigned int timeout_ms) {
  _syncCallTimeoutMs = timeout_ms;
}

void NlsClientImpl::setDirectHostImpl(const char *ip) {
  memset(_directHostIp, 0, 64);
  if (ip) {
    strncpy(_directHostIp, ip, 64);
  }
}

void NlsClientImpl::startWorkThreadImpl(int threadsNumber) {
  if (NlsEventNetWork::_eventClient == NULL) {
    NlsEventNetWork::_eventClient = new NlsEventNetWork();
  }
  if (!_isInitializeThread) {
    NlsEventNetWork::_eventClient->initEventNetWork(
        this, threadsNumber, _aiFamily, _directHostIp, _enableSysGetAddr,
        _syncCallTimeoutMs);
    _isInitializeThread = true;
  }
}

int NlsClientImpl::setLogConfigImpl(const char *logOutputFile,
                                    const LogLevel logLevel,
                                    unsigned int logFileSize,
                                    unsigned int logFileNum,
                                    LogCallbackMethod logCallback) {
  if (logLevel < LogError || logLevel > LogDebug) {
    return -(InvalidLogLevel);
  }
  if (logFileNum < 1) {
    return -(InvalidLogFileNum);
  }

  utility::NlsLog::getInstance()->logConfig(
      logOutputFile, logLevel, logFileSize, logFileNum, logCallback);

  return Success;
}

SpeechRecognizerRequest *NlsClientImpl::createRecognizerRequestImpl(
    const char *sdkName, bool isLongConnection) {
  SpeechRecognizerRequest *request =
      new SpeechRecognizerRequest(sdkName, isLongConnection);
  if (request) {
    int ret = _nodeManager->addRequestIntoInfoWithInstance((void *)request,
                                                           (void *)this);
    if (ret != Success) {
      LOG_ERROR(
          "Request(%p) checkRequestWithInstance failed(%d), this request has "
          "released.",
          request, ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClientImpl::releaseRecognizerRequestImpl(
    SpeechRecognizerRequest *request) {
  if (request) {
    int ret = Success;
    /* check this request belong to this NlsClientImpl */
    ret = _nodeManager->checkRequestWithInstance((void *)request, (void *)this);
    if (ret != Success) {
      LOG_ERROR("Request(%p) checkRequestWithInstance failed.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid &&
        node->getConnectNodeStatus() != NodeClosed) {
      LOG_DEBUG(
          "Request(%p) Node(%p) invoke cancel by releaseRecognizerRequest.",
          request, node);
      request->cancel();
    }

    releaseRequest(request);
  }
}

SpeechTranscriberRequest *NlsClientImpl::createTranscriberRequestImpl(
    const char *sdkName, bool isLongConnection) {
  SpeechTranscriberRequest *request =
      new SpeechTranscriberRequest(sdkName, isLongConnection);
  if (request) {
    int ret = _nodeManager->addRequestIntoInfoWithInstance((void *)request,
                                                           (void *)this);
    if (ret != Success) {
      LOG_ERROR(
          "Request(%p) checkRequestWithInstance failed(%d), this request has "
          "released.",
          request, ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClientImpl::releaseTranscriberRequestImpl(
    SpeechTranscriberRequest *request) {
  if (request) {
    int ret = Success;
    /* check this request belong to this NlsClientImpl */
    ret = _nodeManager->checkRequestWithInstance((void *)request, (void *)this);
    if (ret != Success) {
      LOG_ERROR("Request(%p) checkRequestWithInstance failed.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid &&
        node->getConnectNodeStatus() != NodeClosed) {
      LOG_DEBUG(
          "Request(%p) Node(%p) invoke cancel by releaseTranscriberRequest.",
          request, node);
      request->cancel();
    }

    releaseRequest(request);
  }
}

SpeechSynthesizerRequest *NlsClientImpl::createSynthesizerRequestImpl(
    TtsVersion version, const char *sdkName, bool isLongConnection) {
  SpeechSynthesizerRequest *request =
      new SpeechSynthesizerRequest((int)version, sdkName, isLongConnection);
  if (request) {
    int ret = _nodeManager->addRequestIntoInfoWithInstance((void *)request,
                                                           (void *)this);
    if (ret != Success) {
      LOG_ERROR(
          "Request(%p) checkRequestWithInstance failed(%d), this request has "
          "released.",
          request, ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClientImpl::releaseSynthesizerRequestImpl(
    SpeechSynthesizerRequest *request) {
  if (request) {
    int ret = Success;
    /* check this request belong to this NlsClientImpl */
    ret = _nodeManager->checkRequestWithInstance((void *)request, (void *)this);
    if (ret != Success) {
      LOG_ERROR("Request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid &&
        node->getConnectNodeStatus() != NodeClosed) {
      LOG_DEBUG(
          "Request(%p) Node(%p) invoke cancel by releaseSynthesizerRequest.",
          request, node);
      request->cancel();
    }

    releaseRequest(request);
  }
}

DialogAssistantRequest *NlsClientImpl::createDialogAssistantRequestImpl(
    DaVersion version, const char *sdkName, bool isLongConnection) {
  DialogAssistantRequest *request =
      new DialogAssistantRequest((int)version, sdkName, isLongConnection);
  if (request) {
    int ret = _nodeManager->addRequestIntoInfoWithInstance((void *)request,
                                                           (void *)this);
    if (ret != Success) {
      LOG_ERROR(
          "Request(%p) checkRequestWithInstance failed(%d), this request has "
          "released.",
          request, ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClientImpl::releaseDialogAssistantRequestImpl(
    DialogAssistantRequest *request) {
  if (request) {
    int ret = Success;
    /* check this request belong to this NlsClientImpl */
    ret = _nodeManager->checkRequestWithInstance((void *)request, (void *)this);
    if (ret != Success) {
      LOG_ERROR("request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && request->getConnectNode()->getExitStatus() == ExitInvalid) {
      request->cancel();
    }

    releaseRequest(request);
  }
}

FlowingSynthesizerRequest *NlsClientImpl::createFlowingSynthesizerRequestImpl(
    const char *sdkName, bool isLongConnection) {
  FlowingSynthesizerRequest *request =
      new FlowingSynthesizerRequest(sdkName, isLongConnection);
  if (request) {
    int ret = _nodeManager->addRequestIntoInfoWithInstance((void *)request,
                                                           (void *)this);
    if (ret != Success) {
      LOG_ERROR(
          "Request(%p) checkRequestWithInstance failed(%d), this request has "
          "released.",
          request, ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClientImpl::releaseFlowingSynthesizerRequestImpl(
    FlowingSynthesizerRequest *request) {
  if (request) {
    int ret = Success;
    /* check this request belong to this NlsClientImpl */
    ret = _nodeManager->checkRequestWithInstance((void *)request, (void *)this);
    if (ret != Success) {
      LOG_ERROR("Request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid &&
        node->getConnectNodeStatus() != NodeClosed) {
      LOG_DEBUG(
          "Request(%p) Node(%p) invoke cancel by "
          "releaseFlowingSynthesizerRequest.",
          request, node);
      request->cancel();
    }

    releaseRequest(request);
  }
}

void NlsClientImpl::releaseRequest(INlsRequest *request) {
  if (request == NULL) {
    LOG_ERROR("Input request is nullptr, you have destroyed request!");
    return;
  }
  if (request->getConnectNode() == NULL) {
    LOG_ERROR("The node in request(%p) is nullptr, you have destroyed request!",
              request);
    return;
  }

  int status = NodeStatusInvalid;
  int ret = _nodeManager->checkRequestExist(request, &status);
  if (ret != Success) {
    LOG_ERROR("Request(%p) checkRequestExist0 failed, %d.", request, ret);
    return;
  }

  /* 准备移出request, 上锁保护, 防止其他线程也同时在释放 */
  bool release_lock_ret = true;
  NlsClientImpl *cur_instance = request->getConnectNode()->getInstance();
  if (cur_instance != NULL) {
    MUTEX_TRY_LOCK(cur_instance->_mtxReleaseRequestGuard, 2000,
                   release_lock_ret);
    if (!release_lock_ret) {
      LOG_ERROR("Request(%p) lock destroy failed, deadlock has occurred",
                request);
    }
  } else {
    LOG_INFO("Request(%p) just only created ...", request);
    release_lock_ret = false;
  }

  ret = _nodeManager->checkRequestExist(request, &status);
  if (ret != Success) {
    LOG_ERROR("Request(%p) checkRequestExist1 failed, %d.", request, ret);
    if (release_lock_ret) {
      MUTEX_UNLOCK(cur_instance->_mtxReleaseRequestGuard);
    }
    return;
  }

  request->getConnectNode()->_releasingFlag = true;

  LOG_INFO("Begin release, request(%p) node(%p) node status:%s exit status:%s.",
           request, request->getConnectNode(),
           request->getConnectNode()->getConnectNodeStatusString().c_str(),
           request->getConnectNode()->getExitStatusString().c_str());

  request->getConnectNode()->delAllEvents();
  request->getConnectNode()->updateDestroyStatus();

  ret = _nodeManager->removeRequestFromInfo(request, false);
  if (ret == Success) {
    delete request;
  } else {
    LOG_ERROR("Release request(%p) is invalid, skip ...", request);
  }
  request = NULL;

  if (release_lock_ret) {
    MUTEX_UNLOCK(cur_instance->_mtxReleaseRequestGuard);
  }
  return;
}

#if defined(__linux__)
int NlsClientImpl::vipServerListGetUrlImpl(
    const std::string &vipServerDomainList, const std::string &targetDomain,
    std::string &url) {
#ifdef ENABLE_VIPSERVER
  std::string domainPattern = ",";
  std::string portPattern = ":";
  std::string vipServerBuff, serverIp, serverPort;
  int port = VipServerPort;

  LOG_DEBUG("vipServerDomainList: %s.", vipServerDomainList.c_str());

  size_t domainStart = 0,
         domainSeek = vipServerDomainList.find_first_of(domainPattern, 0);
  size_t portSeek = 0;

  while (domainSeek != vipServerDomainList.npos) {
    if (domainStart != domainSeek) {
      vipServerBuff =
          vipServerDomainList.substr(domainStart, domainSeek - domainStart);

      LOG_DEBUG("vipServerBuff: %s", vipServerBuff.c_str());

      portSeek = vipServerBuff.find_first_of(portPattern, 0);
      if (portSeek != vipServerBuff.npos) {
        serverIp = vipServerBuff.substr(0, portSeek);
        serverPort = vipServerBuff.substr(portSeek + 1,
                                          vipServerBuff.length() - portSeek);
        if (!serverPort.empty()) {
          port = atoi(serverPort.c_str());
        }
      } else {
        serverIp = vipServerBuff;
      }

      // Get ip
      if (Success == vipServerGetIp(serverIp, port, targetDomain, url)) {
        LOG_DEBUG("vipServerGetIp successed:%s.", url.c_str());
        return Success;
      } else {
        MUTEX_LOCK(_mtx);
        _isInitalizeVsClient = false;
        VipClientApi::UnInit();
        MUTEX_UNLOCK(_mtx);
      }
    }

    domainStart = domainSeek + 1;
    domainSeek = vipServerDomainList.find_first_of(domainPattern, domainStart);
  }

  LOG_DEBUG("Last vipServerBuff: %s", vipServerBuff.c_str());

  port = VipServerPort;
  if (!vipServerDomainList.substr(domainStart).empty()) {
    vipServerBuff = vipServerDomainList.substr(domainStart);
    portSeek = vipServerBuff.find_first_of(portPattern, 0);
    if (portSeek != vipServerBuff.npos) {
      serverIp = vipServerBuff.substr(0, portSeek);
      serverPort =
          vipServerBuff.substr(portSeek + 1, vipServerBuff.size() - portSeek);
      if (!serverPort.empty()) {
        port = atoi(serverPort.c_str());
      }

    } else {
      serverIp = vipServerBuff;
    }

    // Get ip
    return vipServerGetIp(serverIp, port, targetDomain, url);
  }
#else
  LOG_WARN("Donnot enable VipServer.");
#endif  // ENABLE_VIPSERVER
  return Success;
}

int NlsClientImpl::vipServerGetIp(const std::string &vipServerDomain,
                                  const int vipServerPort,
                                  const std::string &targetDomain,
                                  std::string &url) {
#ifdef ENABLE_VIPSERVER
  if (vipServerDomain.empty() || targetDomain.empty() || vipServerPort < 0) {
    LOG_ERROR("vipServerGetIp::Input Param error ...");
    return -(InvalidInputParam);
  }

  int tmpPort = vipServerPort;
  char buff[512] = {0};
  if (tmpPort == 0) {
    tmpPort = VipServerPort;
  }

  if (snprintf(buff, 512, "%s:%d", vipServerDomain.c_str(), tmpPort) < 0) {
    return -(InvalidInputParam);
  }

  LOG_DEBUG("vipServerGetIp: %s.", buff);

  MUTEX_LOCK(_mtx);
  if (!_isInitalizeVsClient) {
    Option option;
    //设置日志最大大小(可选)
    option.set_max_log_size(10LL * 1024 * 1024);
    //初始化
    // buff: vipserver顶级域名
    if (!VipClientApi::Init(buff, option)) {
      LOG_ERROR("Init failed top domain:%s errno:%d  errstr:%s.", buff,
                VipClientApi::Errno(), VipClientApi::Errstr());

      MUTEX_UNLOCK(_mtx);
      return -(VipClientInitFailed);
    } else {
      _isInitalizeVsClient = true;
      LOG_INFO("VipClientApi::Init Successed ...");
    }
  }
  MUTEX_UNLOCK(_mtx);

  std::string ip;
  IPHost host;
  // 同步获取域名(targetDomain)下的一个IPHost
  if (VipClientApi::QueryIp(targetDomain.c_str(), &host, 5 * 1000)) {
    std::string host_str = helper::ToString(host);
    LOG_DEBUG("get domain ip ok %s, iphost:%s", targetDomain.c_str(),
              host_str.c_str());
  } else {
    LOG_ERROR("Target domain:%s QueryIp::Failed. errno:%d  errstr:%s.",
              targetDomain.c_str(), VipClientApi::Errno(),
              VipClientApi::Errstr());
  }

  //同步获取IP列表，2000ms超时
  IPHost iphost;
  if (VipClientApi::QueryIp(targetDomain.c_str(), &iphost, 5 * 1000)) {
    ip = iphost.ip();
    int port = iphost.port();
    LOG_DEBUG("valid ip %s %d", iphost.ip(), iphost.port());

    char buffer[256] = {0};
    if (snprintf(buffer, 256, "ws://%s:%d/ws/v1", ip.c_str(), port) < 0) {
      LOG_ERROR("ERROR : Merge host Failed.");
      return -(VipClientMergeHostFailed);
    }

    url = buffer;

    LOG_DEBUG("targetUrl: %s", url.c_str());
    return Success;
  } else {
    LOG_ERROR("ERROR: QueryIp Failed. errno:%d  errstr:%s.",
              VipClientApi::Errno(), VipClientApi::Errstr());
    return -(VipClientQueryIpFailed);
  }

#else
  LOG_WARN("Donnot enable VipServer.");
#endif  // ENABLE_VIPSERVER
  return Success;
}
#endif

}  // namespace AlibabaNls
