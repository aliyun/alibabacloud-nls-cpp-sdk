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

#include "Config.h"
#include "nlsClient.h"
#include "nlog.h"
#include "connectNode.h"
#include "SSLconnect.h"
#include "nlsEventNetWork.h"

#include "sr/speechRecognizerRequest.h"
#include "st/speechTranscriberRequest.h"
#include "sy/speechSynthesizerRequest.h"
#include "da/dialogAssistantRequest.h"

namespace AlibabaNls {

NlsClient* NlsClient::_instance = NULL;  //new NlsClient();
bool NlsClient::_isInitializeSSL = false;
bool NlsClient::_isInitializeThread = false;

#if defined(_MSC_VER)
HANDLE NlsClient::_mtx = CreateMutex(NULL, FALSE, NULL);
#else
pthread_mutex_t NlsClient::_mtx = PTHREAD_MUTEX_INITIALIZER;
#endif

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
      NlsEventNetWork::destroyEventNetWork();
      _isInitializeThread = false;
    }

    if (_isInitializeSSL) {
      LOG_DEBUG("delete NlsClient release ssl.");
      SSLconnect::destroy();
      _isInitializeSSL = false;
    }

    delete _instance;
    _instance = NULL;

    utility::NlsLog::destroyLogInstance();  // donnot LOG_XXX after here
  }

#if defined(_MSC_VER)
  ReleaseMutex(_mtx);
#else
  pthread_mutex_unlock(&_mtx);
#endif
}

NlsClient::NlsClient() {}
NlsClient::~NlsClient() {}

const char* NlsClient::getVersion()	{
  return NLS_SDK_VERSION_STR;
}

void NlsClient::startWorkThread(int threadsNumber) {
#if defined(_MSC_VER)
  WaitForSingleObject(_mtx, INFINITE);
#else
  pthread_mutex_lock(&_mtx);
#endif

  if (!_isInitializeThread) {
    NlsEventNetWork::initEventNetWork(threadsNumber);
    _isInitializeThread = true;
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
    return -1;
  }

  if (logFileSize < 0) {
    return -1;
  }

  utility::NlsLog::getInstance()->logConfig(
      logOutputFile, logLevel, logFileSize, logFileNum);

	return 0;
}

void NlsClient::releaseRequest(INlsRequest* request) {
  LOG_DEBUG("releaseRequest begin. %d:%s",
      request->getConnectNode()->getConnectNodeStatus(),
      request->getConnectNode()->getConnectNodeStatusString().c_str());

  if (request->getConnectNode()->getConnectNodeStatus() == NodeInitial) {
    LOG_INFO("released the Request -> 0");
    delete request;
    request = NULL;
    LOG_INFO("released the Request done 0");
    return;
  }

  if (request->getConnectNode()->updateDestroyStatus()) {
    if (request->getConnectNode()->getConnectNodeStatus() == NodeInvalid) {
      LOG_INFO("released the Request -> 1");
      delete request;
      request = NULL;
      LOG_INFO("released the Request done 1");
      return;
    }
  }

  LOG_DEBUG("releaseRequest done.");
}

SpeechRecognizerRequest* NlsClient::createRecognizerRequest() {
  return new SpeechRecognizerRequest();
}

void NlsClient::releaseRecognizerRequest(SpeechRecognizerRequest* request) {
//  LOG_DEBUG("releaseRecognizerRequest ->");
  if (request) {
    if (request->getConnectNode()->getExitStatus() == ExitInvalid) {
      request->stop();
    }

    releaseRequest(request);
    request = NULL;
  }
//  LOG_DEBUG("releaseRecognizerRequest done.");
}

SpeechTranscriberRequest* NlsClient::createTranscriberRequest() {
  return new SpeechTranscriberRequest();
}

void NlsClient::releaseTranscriberRequest(SpeechTranscriberRequest* request) {
  if (request) {
    if (request->getConnectNode()->getExitStatus() == ExitInvalid) {
      request->stop();
    }

    releaseRequest(request);
    request = NULL;
  }
}

SpeechSynthesizerRequest* NlsClient::createSynthesizerRequest(TtsVersion version){
  return new SpeechSynthesizerRequest((int)version);
}

void NlsClient::releaseSynthesizerRequest(SpeechSynthesizerRequest* request) {
  if (request) {
    if (request->getConnectNode()->getExitStatus() == ExitInvalid) {
      request->stop();
    }

    releaseRequest(request);
    request = NULL;
  }
}

DialogAssistantRequest* NlsClient::createDialogAssistantRequest(
    DaVersion version) {
  return new DialogAssistantRequest((int) version);
}

void NlsClient::releaseDialogAssistantRequest(DialogAssistantRequest* request) {
  if (request) {
    if (request->getConnectNode()->getExitStatus() == ExitInvalid) {
      request->stop();
    }

    releaseRequest(request);
    request = NULL;
  }
}

}  // namespace AlibabaNls
