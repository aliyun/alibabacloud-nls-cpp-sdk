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
#include "Config.h"
#include "nlsClient.h"
#include "nlog.h"
#include "connectNode.h"
#include "SSLconnect.h"
#include "nlsEventNetWork.h"
#include "nodeManager.h"
#include "text_utils.h"

#include "sr/speechRecognizerRequest.h"
#include "st/speechTranscriberRequest.h"
#include "sy/speechSynthesizerRequest.h"
#include "da/dialogAssistantRequest.h"

namespace AlibabaNls {

NlsClient* NlsClient::_instance = NULL;  //new NlsClient();
bool NlsClient::_isInitializeSSL = false;
bool NlsClient::_isInitializeThread = false;
char NlsClient::_aiFamily[16] = "AF_INET";
char NlsClient::_direct_host_ip[64] = {0};
bool NlsClient::_enableSysGetAddr = false;

#if defined(_MSC_VER)
HANDLE NlsClient::_mtx = CreateMutex(NULL, FALSE, NULL);
#else
pthread_mutex_t NlsClient::_mtx = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsClient::NlsClient() {}
NlsClient::~NlsClient() {}

NlsClient* NlsClient::getInstance(bool sslInitial) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtx, INFINITE);
#else
  pthread_mutex_lock(&_mtx);
#endif

  if (NULL == _instance) {
    //init openssl
    if (sslInitial) {
      if (!_isInitializeSSL) {
        SSLconnect::init();
        _isInitializeSSL = sslInitial;
      }
    }

    //init nlsClient
    _instance = new NlsClient();
    _instance->_node_manager = new NlsNodeManager();
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtx);
#else
  pthread_mutex_unlock(&_mtx);
#endif
  return _instance;
}

/*
 */
void NlsClient::releaseInstance() {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtx, INFINITE);
#else
  pthread_mutex_lock(&_mtx);
#endif

  if (_instance) {
    LOG_DEBUG("release NlsClient instance:%p.", _instance);

    if (_isInitializeThread) {
      if (NlsEventNetWork::_eventClient != NULL) {
        NlsEventNetWork::_eventClient->destroyEventNetWork();
        delete NlsEventNetWork::_eventClient;
        NlsEventNetWork::_eventClient = NULL;

        NlsEventNetWork::tryDestroyMutex();
      }
      _isInitializeThread = false;
    }

    if (_isInitializeSSL) {
      LOG_DEBUG("delete NlsClient release ssl.");
      SSLconnect::destroy();
      _isInitializeSSL = false;
    }

    /* find _instance in map and erase it */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->removeInstanceFromInfo((void*)_instance);
    if (ret != Success) {
      LOG_ERROR("removeInstanceFromInfo instance(%p) failed, ret:%d", _instance, ret);
      return;
    }

    delete (NlsNodeManager*)_instance->_node_manager;
    _instance->_node_manager = NULL;
    delete _instance;
    _instance = NULL;

    utility::NlsLog::destroyLogInstance();  // donnot LOG_XXX after here
  } else {
    LOG_WARN("instance has released.");
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtx);
#else
  pthread_mutex_unlock(&_mtx);
#endif
}

void* NlsClient::getNodeManger() {
  return _instance->_node_manager;
}

const char* NlsClient::getVersion()	{
  return NLS_SDK_VERSION_STR;
}

void NlsClient::setAddrInFamily(const char* aiFamily) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtx, INFINITE);
#else
  pthread_mutex_lock(&_mtx);
#endif

  if (_instance) {
    if (aiFamily != NULL &&
        (strncmp(aiFamily, "AF_INET", 16) == 0 ||
         strncmp(aiFamily, "AF_INET6", 16) == 0 ||
         strncmp(aiFamily, "AF_UNSPEC", 16) == 0)) {
      memset(_aiFamily, 0, 16);
      strncpy(_aiFamily, aiFamily, 16);
    }
  } else {
    LOG_WARN("instance has released.");
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtx);
#else
  pthread_mutex_unlock(&_mtx);
#endif
}

void NlsClient::setUseSysGetAddrInfo(bool enable) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtx, INFINITE);
#else
  pthread_mutex_lock(&_mtx);
#endif

  if (_instance) {
    _enableSysGetAddr = enable;
  } else {
    LOG_WARN("instance has released.");
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtx);
#else
  pthread_mutex_unlock(&_mtx);
#endif
}

void NlsClient::setDirectHost(const char* ip) {
  int result = 0;
#if defined(_MSC_VER)
  WaitForSingleObject(_mtx, INFINITE);
#else
  pthread_mutex_lock(&_mtx);
#endif

  if (_instance) {
    memset(_direct_host_ip, 0, 64);
    if (ip) {
      strncpy(_direct_host_ip, ip, 64);
    }
  } else {
    LOG_WARN("instance has released.");
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtx);
#else
  pthread_mutex_unlock(&_mtx);
#endif
}

int NlsClient::calculateUtf8Chars(const char* value) {
  return utility::TextUtils::CharsCalculate(value);
}

void NlsClient::startWorkThread(int threadsNumber) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtx, INFINITE);
#else
  pthread_mutex_lock(&_mtx);
#endif

  if (NlsEventNetWork::_eventClient == NULL) {
    NlsEventNetWork::_eventClient = new NlsEventNetWork();
  }

  if (_instance) {
    if (!_isInitializeThread) {
      NlsEventNetWork::tryCreateMutex();
      NlsEventNetWork::_eventClient->initEventNetWork(
          _instance, threadsNumber, _aiFamily, _direct_host_ip, _enableSysGetAddr);
      _isInitializeThread = true;

      LOG_INFO("NLS Initialize with version %s", utility::TextUtils::GetVersion().c_str());
      LOG_INFO("NLS Git SHA %s", utility::TextUtils::GetGitCommitInfo());
    }
  } else {
    LOG_WARN("instance has released.");
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtx);
#else
  pthread_mutex_unlock(&_mtx);
#endif
}

int NlsClient::setLogConfig(const char* logOutputFile,
                            const LogLevel logLevel,
                            unsigned int logFileSize,
                            unsigned int logFileNum) {
  if ((logLevel < 1) || (logLevel > 4)) {
    return -(InvalidLogLevel);
  }

  if (logFileSize < 0) {
    return -(InvalidLogFileSize);
  }

  if (_instance) {
    utility::NlsLog::getInstance()->logConfig(
        logOutputFile, logLevel, logFileSize, logFileNum);
  } else {
    LOG_WARN("instance has released.");
  }

  return Success;
}

void NlsClient::releaseRequest(INlsRequest* request) {
  if (_instance == NULL) {
    LOG_ERROR("instance has released.");
    return;
  }

  if (request == NULL || request->getConnectNode() == NULL) {
    LOG_ERROR("releaseRequest begin. request or node is nullptr, you have destroyed request or relesed instance!");
    return;
  }

  NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
  int status = NodeStatusInvalid;
  int ret = node_manager->checkRequestExist(request, &status);

  LOG_DEBUG("releaseRequest begin. request:%p Node:%p status(%d):%s exit_status(%d):%s, ret:%d, node status:%s",
      request,
      request->getConnectNode(),
      request->getConnectNode()->getConnectNodeStatus(),
      request->getConnectNode()->getConnectNodeStatusString().c_str(),
      request->getConnectNode()->getExitStatus(),
      request->getConnectNode()->getExitStatusString().c_str(),
      ret, node_manager->getNodeStatusString(status).c_str());

  if (request->getConnectNode()->getConnectNodeStatus() == NodeInitial) {
    LOG_DEBUG("releaseRequest release request(%p) node(%p) success with NodeInitial.",
        request, request->getConnectNode());
    node_manager->removeRequestFromInfo(request, false);
    delete request;
    request = NULL;
    return;
  } else if (request->getConnectNode()->getConnectNodeStatus() == NodeConnecting) {
    LOG_DEBUG("releaseRequest release request(%p) node(%p) success with NodeConnecting.",
        request, request->getConnectNode());
    request->getConnectNode()->setConnectNodeStatus(NodeInvalid);
  } else if (request->getConnectNode()->getConnectNodeStatus() == NodeStarted) {
    if (request->getConnectNode()->_isLongConnection) {
      LOG_WARN("request:%p Node:%p cStatus(NodeStarted) is abnormal, maybe get (err:IDLE_TIMEOUT) after start...",
          request, request->getConnectNode());
      request->getConnectNode()->setConnectNodeStatus(NodeInvalid);
    }
    if (request->getConnectNode()->getExitStatus() == ExitStopping) {
      LOG_WARN("request:%p Node:%p cStatus(NodeStarted) eStatus(ExitStopping), occurring in releaseRequest in runtime...",
          request, request->getConnectNode());
      request->getConnectNode()->setConnectNodeStatus(NodeInvalid);
    }
  }

  if (request->getConnectNode()->getExitStatus() == ExitCancel) {
    LOG_WARN("request:%p Node:%p cStatus(%s) eStatus(ExitCancel), occurring in invoking cancel()...",
        request, request->getConnectNode(),
        request->getConnectNode()->getConnectNodeStatusString().c_str());
    request->getConnectNode()->setConnectNodeStatus(NodeInvalid);
    node_manager->updateNodeStatus(request->getConnectNode(), NodeStatusClosed);
  }

  // ready to destroy node
  if (request->getConnectNode()->updateDestroyStatus()) {
    if (request->getConnectNode()->getConnectNodeStatus() == NodeInvalid) {
      LOG_DEBUG("releaseRequest release request(%p) node(%p) success with NodeInvalid.",
          request, request->getConnectNode());
      node_manager->removeRequestFromInfo(request, false);
      delete request;
      request = NULL;
      return;
    }
  } else {
    if (request->getConnectNode()->getConnectNodeStatus() == NodeInvalid) {
      if (request->getConnectNode()->_isLongConnection) {
        LOG_DEBUG("releaseRequest release request(%p) node(%p) success with longConnect NodeInvalid.",
            request, request->getConnectNode());
        node_manager->removeRequestFromInfo(request, false);
        delete request;
        request = NULL;
        return;
      }
    }
  }

  LOG_DEBUG("releaseRequest request(%p) node(%p) done without release.",
      request, request->getConnectNode());
}

SpeechRecognizerRequest* NlsClient::createRecognizerRequest(
    const char* sdkName, bool isLongConnection) {
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return NULL;
  }

  SpeechRecognizerRequest* request = new SpeechRecognizerRequest(sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("add request into NodeInfo failed, ret:%d", ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseRecognizerRequest(SpeechRecognizerRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return;
  }

  if (request) {
    int ret = Success;
    /* check this request belong to this _instance */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    ret = node_manager->checkRequestWithInstance((void*)request, _instance);
    if (ret != Success) {
      LOG_ERROR("request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid) {
      if (request->getConnectNode()->getCallbackStatus() == CallbackCancelled) {
        request->cancel();
      } else {
        request->stop();
      }
    }

    releaseRequest(request);
    request = NULL;
  }
}

SpeechTranscriberRequest* NlsClient::createTranscriberRequest(
    const char* sdkName, bool isLongConnection) {
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return NULL;
  }

  SpeechTranscriberRequest* request = new SpeechTranscriberRequest(sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("add request into NodeInfo failed, ret:%d", ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseTranscriberRequest(SpeechTranscriberRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return;
  }
  if (request) {
    int ret = Success;
    /* check this request belong to this _instance */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    ret = node_manager->checkRequestWithInstance((void*)request, _instance);
    if (ret != Success) {
      LOG_ERROR("request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && request->getConnectNode()->getExitStatus() == ExitInvalid) {
      if (request->getConnectNode()->getCallbackStatus() == CallbackCancelled) {
        request->cancel();
      } else {
        LOG_DEBUG("request(%p) %s callbackstatus:%d", request, __func__, request->getConnectNode()->getCallbackStatus());
        request->stop();
      }
    }

    releaseRequest(request);
    request = NULL;
  }
}

SpeechSynthesizerRequest* NlsClient::createSynthesizerRequest(
    TtsVersion version, const char* sdkName, bool isLongConnection){
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return NULL;
  }

  SpeechSynthesizerRequest* request = new SpeechSynthesizerRequest((int)version, sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("add request into NodeInfo failed, ret:%d", ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseSynthesizerRequest(SpeechSynthesizerRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return;
  }
  if (request) {
    int ret = Success;
    /* check this request belong to this _instance */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    ret = node_manager->checkRequestWithInstance((void*)request, _instance);
    if (ret != Success) {
      LOG_ERROR("request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node == NULL) {
      LOG_ERROR("releaseSynthesizerRequest node is nullptr");
    }
    if (node && request->getConnectNode()->getExitStatus() == ExitInvalid) {
      if (request->getConnectNode()->getCallbackStatus() == CallbackCancelled) {
        request->cancel();
      } else {
        request->stop();
      }
    }

    releaseRequest(request);
    request = NULL;
  }
}

DialogAssistantRequest* NlsClient::createDialogAssistantRequest(
    DaVersion version, const char* sdkName, bool isLongConnection) {
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return NULL;
  }

  DialogAssistantRequest* request = new DialogAssistantRequest((int)version, sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("add request into NodeInfo failed, ret:%d", ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseDialogAssistantRequest(DialogAssistantRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("instance has released.");
    return;
  }
  if (request) {
    int ret = Success;
    /* check this request belong to this _instance */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    ret = node_manager->checkRequestWithInstance((void*)request, _instance);
    if (ret != Success) {
      LOG_ERROR("request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && request->getConnectNode()->getExitStatus() == ExitInvalid) {
      if (request->getConnectNode()->getCallbackStatus() == CallbackCancelled) {
        request->cancel();
      } else {
        request->stop();
      }
    }

    releaseRequest(request);
    request = NULL;
  }
}

}  // namespace AlibabaNls
