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
#include "SSLconnect.h"
#include "connectNode.h"
#include "da/dialogAssistantRequest.h"
#include "nlog.h"
#include "nlsClient.h"
#include "nlsClientImpl.h"
#include "nlsEventNetWork.h"
#include "nodeManager.h"

#include "sr/speechRecognizerRequest.h"
#include "st/speechTranscriberRequest.h"
#include "sy/speechSynthesizerRequest.h"
#include "text_utils.h"
#include "utility.h"

namespace AlibabaNls {

NlsClient *NlsClient::_instance = NULL;  // new NlsClient();
#if defined(_MSC_VER)
static HANDLE _mtxNlsClient = CreateMutex(NULL, FALSE, NULL);
#else
static pthread_mutex_t _mtxNlsClient = PTHREAD_MUTEX_INITIALIZER;
#endif

NlsClient::NlsClient() : _impl(NULL) {}

NlsClient::~NlsClient() {}

NlsClient *NlsClient::getInstance(bool sslInitial) {
  MUTEX_LOCK(_mtxNlsClient);
  if (NULL == _instance) {
    // init nlsClient
    _instance = new NlsClient();
    _instance->_impl = new NlsClientImpl(sslInitial);
    LOG_INFO("New NlsClient attach client(%p) to instance(%p).", _instance,
             _instance->_impl);
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return _instance;
}

void NlsClient::releaseInstance() {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    LOG_INFO("Release NlsClient instance:%p.", _instance);
    _instance->_impl->releaseInstanceImpl();
    LOG_INFO("Release NlsClientImpl %p.", _instance->_impl);
    delete _instance->_impl;
    _instance->_impl = NULL;
    LOG_INFO("Release NlsClient instance:%p.", _instance);
    delete _instance;
    _instance = NULL;
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

const char *NlsClient::getVersion() { return NLS_SDK_VERSION_STR; }

void NlsClient::setAddrInFamily(const char *aiFamily) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->setAddrInFamilyImpl(aiFamily);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

void NlsClient::setUseSysGetAddrInfo(bool enable) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->setUseSysGetAddrInfoImpl(enable);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

void NlsClient::setSyncCallTimeout(unsigned int timeout_ms) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->setSyncCallTimeoutImpl(timeout_ms);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

void NlsClient::setDirectHost(const char *ip) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->setDirectHostImpl(ip);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

int NlsClient::calculateUtf8Chars(const char *value) {
  return utility::TextUtils::CharsCalculate(value);
}

void NlsClient::startWorkThread(int threadsNumber) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    LOG_INFO("NLS initialize with version %s",
             utility::TextUtils::GetVersion().c_str());
    LOG_INFO("NLS Git SHA %s", utility::TextUtils::GetGitCommitInfo());
    _instance->_impl->startWorkThreadImpl(threadsNumber);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

int NlsClient::setLogConfig(const char *logOutputFile, const LogLevel logLevel,
                            unsigned int logFileSize, unsigned int logFileNum,
                            LogCallbackMethod logCallback) {
  MUTEX_LOCK(_mtxNlsClient);
  int result = -(EventClientEmpty);
  if (_instance) {
    result = _instance->_impl->setLogConfigImpl(
        logOutputFile, logLevel, logFileSize, logFileNum, logCallback);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return result;
}

SpeechRecognizerRequest *NlsClient::createRecognizerRequest(
    const char *sdkName, bool isLongConnection) {
  MUTEX_LOCK(_mtxNlsClient);
  SpeechRecognizerRequest *request = NULL;
  if (_instance) {
    request = _instance->_impl->createRecognizerRequestImpl(sdkName,
                                                            isLongConnection);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return request;
}

void NlsClient::releaseRecognizerRequest(SpeechRecognizerRequest *request) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->releaseRecognizerRequestImpl(request);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

SpeechTranscriberRequest *NlsClient::createTranscriberRequest(
    const char *sdkName, bool isLongConnection) {
  MUTEX_LOCK(_mtxNlsClient);
  SpeechTranscriberRequest *request = NULL;
  if (_instance) {
    request = _instance->_impl->createTranscriberRequestImpl(sdkName,
                                                             isLongConnection);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return request;
}

void NlsClient::releaseTranscriberRequest(SpeechTranscriberRequest *request) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->releaseTranscriberRequestImpl(request);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

SpeechSynthesizerRequest *NlsClient::createSynthesizerRequest(
    TtsVersion version, const char *sdkName, bool isLongConnection) {
  MUTEX_LOCK(_mtxNlsClient);
  SpeechSynthesizerRequest *request = NULL;
  if (_instance) {
    request = _instance->_impl->createSynthesizerRequestImpl(version, sdkName,
                                                             isLongConnection);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return request;
}

void NlsClient::releaseSynthesizerRequest(SpeechSynthesizerRequest *request) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->releaseSynthesizerRequestImpl(request);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

DialogAssistantRequest *NlsClient::createDialogAssistantRequest(
    DaVersion version, const char *sdkName, bool isLongConnection) {
  MUTEX_LOCK(_mtxNlsClient);
  DialogAssistantRequest *request = NULL;
  if (_instance) {
    request = _instance->_impl->createDialogAssistantRequestImpl(
        version, sdkName, isLongConnection);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return request;
}

void NlsClient::releaseDialogAssistantRequest(DialogAssistantRequest *request) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->releaseDialogAssistantRequestImpl(request);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

FlowingSynthesizerRequest *NlsClient::createFlowingSynthesizerRequest(
    const char *sdkName, bool isLongConnection) {
  MUTEX_LOCK(_mtxNlsClient);
  FlowingSynthesizerRequest *request = NULL;
  if (_instance) {
    request = _instance->_impl->createFlowingSynthesizerRequestImpl(
        sdkName, isLongConnection);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return request;
}

void NlsClient::releaseFlowingSynthesizerRequest(
    FlowingSynthesizerRequest *request) {
  MUTEX_LOCK(_mtxNlsClient);
  if (_instance) {
    _instance->_impl->releaseFlowingSynthesizerRequestImpl(request);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
}

#if defined(__linux__)
int NlsClient::vipServerListGetUrl(const std::string &vipServerDomainList,
                                   const std::string &targetDomain,
                                   std::string &url) {
#ifdef ENABLE_VIPSERVER
  MUTEX_LOCK(_mtxNlsClient);
  int result = -(EventClientEmpty);
  if (_instance) {
    result = _instance->_impl->vipServerListGetUrlImpl(vipServerDomainList,
                                                       targetDomain, url);
  } else {
    LOG_WARN("Current instance has released.");
  }
  MUTEX_UNLOCK(_mtxNlsClient);
  return result;
#else
  LOG_WARN("Donnot enable VipServer.");
#endif  // ENABLE_VIPSERVER
  return Success;
}
#endif

}  // namespace AlibabaNls
