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

#include "nlsClient.h"
#include "openssl/crypto.h"
#include "openssl/ssl.h"
#include "util/log.h"
#include "sr/speechRecognizerRequest.h"
#include "st/speechTranscriberRequest.h"
#include "sy/speechSynthesizerRequest.h"

using namespace util;

namespace SSL_ALI {

#define MUTEX_TYPE       pthread_mutex_t
#define MUTEX_SETUP(x)   pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))

static MUTEX_TYPE *mutex_buf = NULL;

static void locking_function(int mode, int n, const char *file, int line) {
	if (mode & CRYPTO_LOCK) {
        MUTEX_LOCK(mutex_buf[n]);
    } else {
        MUTEX_UNLOCK(mutex_buf[n]);
    }
}

static unsigned long id_function(void) {
#if defined (_WIN32)
	return pthread_self().x;
#elif defined (__APPLE__)
	return (unsigned long)pthread_self();
#else
	return pthread_self();
#endif
}

int thread_setup(void) {
	int i;

	mutex_buf = (pthread_mutex_t*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
	if (!mutex_buf) {
		return 0;
    }

    for (i = 0; i < CRYPTO_num_locks(); i++) {
        MUTEX_SETUP(mutex_buf[i]);
    }

	CRYPTO_set_id_callback(id_function);
	CRYPTO_set_locking_callback(locking_function);

    return 1;
}

int thread_cleanup(void) {
	int i;

	if (!mutex_buf) {
        return -1;
    }

	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);

    for (i = 0; i < CRYPTO_num_locks(); i++) {
        MUTEX_CLEANUP(mutex_buf[i]);
    }

    OPENSSL_free(mutex_buf);
	mutex_buf = NULL;
	return 0;
}
}

NlsClient* NlsClient::_instance = NULL;
pthread_mutex_t NlsClient::_mtx = PTHREAD_MUTEX_INITIALIZER;
bool NlsClient::_isInitializeSSL = false;

NlsClient* NlsClient::getInstance(bool sslInitial) {

    pthread_mutex_lock(&_mtx);

    if(sslInitial) {
        if(!_isInitializeSSL) {
			LOG_DEBUG("initialized ssl");
            SSL_ALI::thread_setup();
            SSLeay_add_ssl_algorithms();
            _isInitializeSSL = sslInitial;
        }
    }

    if (NULL == _instance) {
        _instance = new NlsClient();
    }
    pthread_mutex_unlock(&_mtx);

    return _instance;
}

void NlsClient::releaseInstance() {

    pthread_mutex_lock(&_mtx);
    if (_instance) {
        LOG_DEBUG("release NlsClient.");

        delete _instance;
        _instance = NULL;
    }
    pthread_mutex_unlock(&_mtx);

}

NlsClient::NlsClient() {

}

NlsClient::~NlsClient() {
	if (_isInitializeSSL) {
        LOG_DEBUG("delete NlsClient release ssl.");

		SSL_ALI::thread_cleanup();
        _isInitializeSSL = false;
	}

	if(Log::_output != NULL && Log::_output != stdout) {
        LOG_DEBUG("delete NlsClient close log file.");
		fclose(Log::_output);
	}
}

int NlsClient::setLogConfig(const char* logOutputFile, int logLevel) {
	if (logOutputFile != NULL) {
		FILE* fs = fopen(logOutputFile, "w+");
		if (fs != NULL) {
			Log::_output = fs;
		} else {
			LOG_ERROR("open the log output file failed.");
            return -1;
        }
	} else {
        LOG_WARN("logOutputFile is NULL, it will output to stdout");
        return -1;
    }

	if (logLevel >= LogError && logLevel <= LogDebug) {
		Log::_logLevel = logLevel;
	} else {
        Log::_logLevel = LogDebug;
	}

	return 0;
}

SpeechRecognizerRequest* NlsClient::createRecognizerRequest(SpeechRecognizerCallback* onResultReceivedEvent,
															const char* config) {
	if (NULL == onResultReceivedEvent) {
		LOG_ERROR("the callback is NULL");
		return NULL;
	}
	
	return new SpeechRecognizerRequest(onResultReceivedEvent, config);
}

void NlsClient::releaseRecognizerRequest(SpeechRecognizerRequest* request) {
    if (request) {
        delete request;
        request = NULL;
        LOG_DEBUG("released the SpeechRecognizerRequest");
    }
}

SpeechTranscriberRequest* NlsClient::createTranscriberRequest(SpeechTranscriberCallback* onResultReceivedEvent,
                                                              const char* config) {
	if (NULL == onResultReceivedEvent) {
		LOG_ERROR("the callback is NULL");
		return NULL;
	}
	
	return new SpeechTranscriberRequest(onResultReceivedEvent, config);
}

void NlsClient::releaseTranscriberRequest(SpeechTranscriberRequest* request) {
    if (request) {
        delete request;
        request = NULL;
        LOG_DEBUG("released the SpeechTranscriberRequest");
    }
}

SpeechSynthesizerRequest* NlsClient::createSynthesizerRequest(SpeechSynthesizerCallback* onResultReceivedEvent,
															  const char* config){
	if (NULL == onResultReceivedEvent) {
		LOG_ERROR("the callback is NULL");
		return NULL;
	}

	return new SpeechSynthesizerRequest(onResultReceivedEvent, config);

}

void NlsClient::releaseSynthesizerRequest(SpeechSynthesizerRequest* request) {
    if (request) {
        delete request;
        request = NULL;
        LOG_DEBUG("released the SpeechSynthesizerRequest");
    }
}
