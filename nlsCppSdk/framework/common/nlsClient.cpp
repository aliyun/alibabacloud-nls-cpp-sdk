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
    LOG_DEBUG("Release NlsClient instance:%p.", _instance);

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

    LOG_INFO("destroy log instance.");
    utility::NlsLog::destroyLogInstance();  // donnot LOG_XXX after here
  } else {
    LOG_WARN("Current instance has released.");
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
    LOG_WARN("Current instance has released.");
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
    LOG_WARN("Current instance has released.");
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
    LOG_WARN("Current instance has released.");
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
      NlsEventNetWork::_eventClient->initEventNetWork(
          _instance, threadsNumber, _aiFamily, _direct_host_ip, _enableSysGetAddr);
      _isInitializeThread = true;

      LOG_INFO("NLS initialize with version %s", utility::TextUtils::GetVersion().c_str());
      LOG_INFO("NLS Git SHA %s", utility::TextUtils::GetGitCommitInfo());
    }
  } else {
    LOG_WARN("Current instance has released.");
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
  if (logLevel < LogError || logLevel > LogDebug) {
    return -(InvalidLogLevel);
  }
  if (logFileNum < 1) {
    return -(InvalidLogFileNum);
  }

  if (_instance) {
    utility::NlsLog::getInstance()->logConfig(
        logOutputFile, logLevel, logFileSize, logFileNum);
  } else {
    LOG_WARN("Current instance has released.");
  }

  return Success;
}

void NlsClient::releaseRequest(INlsRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
    return;
  }
  if (request == NULL) {
    LOG_ERROR("Input request is nullptr, you have destroyed request!");
    return;
  }
  if (request->getConnectNode() == NULL) {
    LOG_ERROR("The node in request(%p) is nullptr, you have destroyed request!", request);
    return;
  }

  NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
  int status = NodeStatusInvalid;
  int ret = node_manager->checkRequestExist(request, &status);
  if (ret != Success) {
    LOG_ERROR("Request(%p) checkRequestExist failed.", request);
    return;
  }

  LOG_INFO("Begin release, request(%p) node(%p) node status:%s exit status:%s.",
      request,
      request->getConnectNode(),
      request->getConnectNode()->getConnectNodeStatusString().c_str(),
      request->getConnectNode()->getExitStatusString().c_str());

  request->getConnectNode()->delAllEvents();
  request->getConnectNode()->updateDestroyStatus();

  delete request;
  node_manager->removeRequestFromInfo(request, false);
  request = NULL;

  return;
}

SpeechRecognizerRequest* NlsClient::createRecognizerRequest(
    const char* sdkName, bool isLongConnection) {
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
    return NULL;
  }

  SpeechRecognizerRequest* request = new SpeechRecognizerRequest(sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("Add request into NodeInfo failed, ret:%d.", ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseRecognizerRequest(SpeechRecognizerRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
    return;
  }

  if (request) {
    int ret = Success;
    /* check this request belong to this _instance */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    ret = node_manager->checkRequestWithInstance((void*)request, _instance);
    if (ret != Success) {
      LOG_ERROR("Request(%p) checkRequestWithInstance failed.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid) {
      request->cancel();
    }

    releaseRequest(request);
    request = NULL;
  }
}

SpeechTranscriberRequest* NlsClient::createTranscriberRequest(
    const char* sdkName, bool isLongConnection) {
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
    return NULL;
  }

  SpeechTranscriberRequest* request = new SpeechTranscriberRequest(sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("Add request into NodeInfo failed, ret:%d.", ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseTranscriberRequest(SpeechTranscriberRequest* request) {
  if (_instance == NULL) {
    LOG_ERROR("Current instance has released.");
    return;
  }
  if (request) {
    int ret = Success;
    /* check this request belong to this _instance */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    ret = node_manager->checkRequestWithInstance((void*)request, _instance);
    if (ret != Success) {
      LOG_ERROR("Request(%p) checkRequestWithInstance failed.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid) {
      request->cancel();
    }

    releaseRequest(request);
    request = NULL;
  }
}

SpeechSynthesizerRequest* NlsClient::createSynthesizerRequest(
    TtsVersion version, const char* sdkName, bool isLongConnection){
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
    return NULL;
  }

  SpeechSynthesizerRequest* request = new SpeechSynthesizerRequest((int)version, sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("Add request into NodeInfo failed, ret:%d.", ret);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseSynthesizerRequest(SpeechSynthesizerRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
    return;
  }
  if (request) {
    int ret = Success;
    /* check this request belong to this _instance */
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    ret = node_manager->checkRequestWithInstance((void*)request, _instance);
    if (ret != Success) {
      LOG_ERROR("Request(%p) is invalid.", request);
      return;
    }

    ConnectNode *node = request->getConnectNode();
    if (node && node->getExitStatus() == ExitInvalid) {
      request->cancel();
    }

    releaseRequest(request);
    request = NULL;
  }
}

DialogAssistantRequest* NlsClient::createDialogAssistantRequest(
    DaVersion version, const char* sdkName, bool isLongConnection) {
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
    return NULL;
  }

  DialogAssistantRequest* request = new DialogAssistantRequest((int)version, sdkName, isLongConnection);
  if (request) {
    NlsNodeManager* node_manager = (NlsNodeManager*)_instance->_node_manager;
    int ret = node_manager->addRequestIntoInfoWithInstance((void*)request, (void*)_instance);
    if (ret != Success) {
      LOG_ERROR("Request(%p) checkRequestWithInstance failed.", request);
      delete request;
      request = NULL;
    }
  }

  return request;
}

void NlsClient::releaseDialogAssistantRequest(DialogAssistantRequest* request) {
  if (_instance == NULL) {
    LOG_WARN("Current instance has released.");
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
      request->cancel();
    }

    releaseRequest(request);
    request = NULL;
  }
}

}  // namespace AlibabaNls
