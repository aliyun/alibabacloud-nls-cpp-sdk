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
#include "Config.h"
#include "nlsClient.h"
#include "log.h"
#include "connectNode.h"
#include "eventNetWork.h"

#include "sr/speechRecognizerRequest.h"
//#include "sr/speechRecognizerSyncRequest.h"
#include "st/speechTranscriberRequest.h"
//#include "st/speechTranscriberSyncRequest.h"
#include "sy/speechSynthesizerRequest.h"
#include "da/dialogAssistantRequest.h"

#if !defined(__APPLE__)
#include "commonSsl.h"
#endif

namespace AlibabaNls {

using namespace utility;
//using namespace transport;

NlsClient* NlsClient::_instance = NULL;//new NlsClient();
bool NlsClient::_isInitializeSSL = false;
bool NlsClient::_isInitializeThread = false;
#if defined(_WIN32)
	HANDLE NlsClient::_mtx = CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_t NlsClient::_mtx = PTHREAD_MUTEX_INITIALIZER;
#endif


/*
 1: init openssl
 2: init log
 3: init vipServer
 4: startup libevent work threads
 */
NlsClient* NlsClient::getInstance(bool sslInitial) {

#if defined(_WIN32)
	WaitForSingleObject(_mtx, INFINITE);
#else
	pthread_mutex_lock(&_mtx);
#endif
	if (NULL == _instance) {
		//init openssl
    	if(sslInitial) {
        	if(!_isInitializeSSL) {
#if !defined(__APPLE__)
				SslConnect::init();
#endif
            	_isInitializeSSL = sslInitial;
        	}
    	}

		//init libevent work threads
//		NlsEventClientNetWork::initEventNetWork(count);

		//init nlsClient
		_instance = new NlsClient();

    }
#if defined(_WIN32)
	ReleaseMutex(_mtx);
#else
	pthread_mutex_unlock(&_mtx);
#endif

    return _instance;
}

/*
 1: destroy openssl
 2: destroy log
 3: destroy vipServer
 4: stop libevent work threads
 */
void NlsClient::releaseInstance() {
#if defined(_WIN32)
		WaitForSingleObject(_mtx, INFINITE);
#else
		pthread_mutex_lock(&_mtx);
#endif
    if (_instance) {
		LOG_DEBUG("release NlsClient.");

		if(_isInitializeThread) {
			NlsEventClientNetWork::destroyEventNetWork();
		}

		if (_isInitializeSSL) {
#if !defined(__APPLE__)
			LOG_DEBUG("delete NlsClient release ssl.");

			SslConnect::destroy();
#endif
			_isInitializeSSL = false;
		}

		NlsLog::destroyLogInstance();

        delete _instance;
        _instance = NULL;
    }

#if defined(_WIN32)
		ReleaseMutex(_mtx);
#else
		pthread_mutex_unlock(&_mtx);
#endif
}

NlsClient::NlsClient() {
//	_eventWork = NULL;
}

NlsClient::~NlsClient() {
//	delete _eventWork;
//	_eventWork = NULL;
}

const char* NlsClient::getVersion()	{
	return NLS_SDK_VERSION_STR;
}

void NlsClient::startWorkThread(int threadsNumber) {
#if defined(_WIN32)
	WaitForSingleObject(_mtx, INFINITE);
#else
	pthread_mutex_lock(&_mtx);
#endif

	if(!_isInitializeThread) {
		NlsEventClientNetWork::initEventNetWork(threadsNumber);
		_isInitializeThread = true;
	}

#if defined(_WIN32)
	ReleaseMutex(_mtx);
#else
	pthread_mutex_unlock(&_mtx);
#endif

}

int NlsClient::setLogConfig(const char* logOutputFile, const LogLevel logLevel, unsigned int logFileSize) {

	if ((logLevel < 1) || (logLevel > 4)) {
		return -1;
	}

	if (logFileSize < 0) {
		return -1;
	}

	NlsLog::_logInstance->logConfig(logOutputFile, logLevel, logFileSize);

	return 0;
}

void NlsClient::releaseRequest(INlsRequest* request) {
	if (request->getConnectNode()->getConnectNodeStatus() == NodeInitial) {
		LOG_INFO("released the SpeechRecognizerRequest");
		delete request;
		request = NULL;
		return ;
	}

	if (request->getConnectNode()->updateDestroyStatus()) {
		if (request->getConnectNode()->getConnectNodeStatus() == NodeInvalid) {
			LOG_INFO("released the SpeechRecognizerRequest");
			delete request;
			request = NULL;
			return;
		}
	}
}

SpeechRecognizerRequest* NlsClient::createRecognizerRequest() {
	return new SpeechRecognizerRequest();
}

void NlsClient::releaseRecognizerRequest(SpeechRecognizerRequest* request) {
	if (request) {
		if (request->getConnectNode()->getExitStatus() == ExitInvalid) {
			request->stop();
		}

		releaseRequest(request);
	}
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
	}
}

DialogAssistantRequest* NlsClient::createDialogAssistantRequest(DaVersion version) {
    return new DialogAssistantRequest((int) version);
}

void NlsClient::releaseDialogAssistantRequest(DialogAssistantRequest* request) {
	if (request) {
		if (request->getConnectNode()->getExitStatus() == ExitInvalid) {
			request->stop();
		}

		releaseRequest(request);
	}
}


}
