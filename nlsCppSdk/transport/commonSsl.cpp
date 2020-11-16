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
#include <stdio.h>
#include "openssl/err.h"
#include "log.h"
#include "utility.h"
#include "connectNode.h"
#include "commonSsl.h"

namespace AlibabaNls {

using std::string;
using namespace utility;

#if OPENSSL_VERSION_NUMBER < 0x10100000L

#include "openssl/crypto.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/conf.h"
#include "openssl/engine.h"

#if defined(_WIN32)
#include <windows.h>
#define MUTEX_TYPE HANDLE
#define MUTEX_SETUP(x) (x) = CreateMutex(NULL, FALSE, NULL)
#define MUTEX_CLEANUP(x) CloseHandle(x)
#define MUTEX_LOCK(x) WaitForSingleObject((x), INFINITE)
#define MUTEX_UNLOCK(x) ReleasReleaseMutexeMutex(x)
#define THREAD_ID GetCurrentThreadId()
#else
#include <pthread.h>
#define MUTEX_TYPE       pthread_mutex_t
#define MUTEX_SETUP(x)   pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))
#define THREAD_ID pthread_self()
#endif

static MUTEX_TYPE *mutex_buf = NULL;

static void lockingCallbackFunction(int mode, int n, const char *file, int line) {
    if (mode & CRYPTO_LOCK) {
        MUTEX_LOCK(mutex_buf[n]);
    } else {
        MUTEX_UNLOCK(mutex_buf[n]);
    }
}

static unsigned long idCallbackFunction(void) {
    return ((unsigned long)THREAD_ID);
}

int threadsSetup(void) {
    int i;

    mutex_buf = (MUTEX_TYPE*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
    if (!mutex_buf) {
        return 0;
    }

    for (i = 0; i < CRYPTO_num_locks(); i++) {
        MUTEX_SETUP(mutex_buf[i]);
    }

    CRYPTO_set_id_callback(idCallbackFunction);
    CRYPTO_set_locking_callback(lockingCallbackFunction);

    return 1;
}

int threadsCleanup(void) {
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

#endif /*OPENSSL_VERSION_NUMBER < 0x10100000L */

SSL_CTX* SslConnect::_sslCtx = NULL;

//#if defined(_MSC_VER)
//pthread_mutex_t SslConnect::_sslMutex = PTHREAD_MUTEX_INITIALIZER;
//#endif

SslConnect::SslConnect() {
    _ssl = NULL;
}

SslConnect::~SslConnect() {

//    LOG_DEBUG("destroy SslConnect done..");
    sslClose();
}

int SslConnect::init() {

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    LOG_DEBUG("SslConnect::init() 0x10100000L.");
    AlibabaNls::threadsSetup();
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
#endif /*OPENSSL_VERSION_NUMBER < 0x10100000L */

    _sslCtx = SSL_CTX_new(SSLv23_client_method());
    if (_sslCtx == NULL) {
        LOG_ERROR("SSL: couldn't create a context!");
        exit(1);
    }

    SSL_CTX_set_mode(_sslCtx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_AUTO_RETRY);

    LOG_DEBUG("SslConnect::init() done.");

    return 0;
}

void SslConnect::destroy() {

    if (_sslCtx) {
//        LOG_DEBUG("_sslCtx free.");
        SSL_CTX_free(_sslCtx);
        _sslCtx = NULL;
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    AlibabaNls::threadsCleanup();

    CONF_modules_free();
    ENGINE_cleanup();
    CONF_modules_unload(1);
    ERR_free_strings();
    EVP_cleanup();

#ifdef EVENT__HAVE_ERR_REMOVE_THREAD_STATE
    ERR_remove_thread_state(NULL);
#else
    ERR_remove_state(0);
#endif
    CRYPTO_cleanup_all_ex_data();

    SSL_COMP_free_compression_methods();
#endif /*OPENSSL_VERSION_NUMBER < 0x10100000L */

    LOG_DEBUG("SslConnect::destroy() done.");
}

int SslConnect::sslHandshake(int socketFd, const char* hostname) {

//    LOG_DEBUG("begin sslHandshake.");

    if (_sslCtx == NULL) {
        return -1;
    }

//    LOG_DEBUG("sslHandshake 000.");

    int ret;
    if (_ssl == NULL) {
        _ssl = SSL_new(_sslCtx);
        if (_ssl == NULL) {
            memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
            ERR_error_string_n(ERR_get_error(), _errorMsg, MAX_SSL_ERROR_LENGTH);
            LOG_ERROR("Ssl SSL_new failed:%s.", _errorMsg);
            return -1;
        }

        ret = SSL_set_fd(_ssl, socketFd);
        if (ret == 0) {
            memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
            ERR_error_string_n(ERR_get_error(), _errorMsg, MAX_SSL_ERROR_LENGTH);
            LOG_ERROR("Ssl set_fd failed:%s.", _errorMsg);
            return -1;
        }

//        LOG_DEBUG("sslHandshake 001.");

        SSL_set_mode(_ssl, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER | SSL_MODE_AUTO_RETRY);

//        SSL_set_mode(_ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);
//        SSL_set_mode(_ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

        SSL_set_connect_state(_ssl);
    }

//#if OPENSSL_VERSION_NUMBER < 0x10100000L
//#if defined(_WIN32)
//    pthread_mutex_lock(&_sslMutex);
//#endif
//#endif /*OPENSSL_VERSION_NUMBER < 0x10100000L */

    int sslError;
    ret = SSL_connect(_ssl);
    if (ret < 0) {
            sslError = SSL_get_error(_ssl, ret);

            /*err == SSL_ERROR_ZERO_RETURN "SSL_connect: close notify received from peer"*/
            //sslError == SSL_ERROR_WANT_X509_LOOKUP
            // SSL_ERROR_SYSCALL
            if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE) {
//                LOG_DEBUG("sslHandshake continue.");
                return sslError;
            } else if (sslError == SSL_ERROR_SYSCALL) {
                int errno_code = getLastErrorCode();
                LOG_INFO("Ssl connect failed:%d.", errno_code);
                if (NLS_ERR_CONNECT_RETRIABLE(errno_code) || NLS_ERR_RW_RETRIABLE(errno_code)) {
                    return SSL_ERROR_WANT_READ;
                } else {
                    return -1;
                }
            } else {
                memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
                ERR_error_string_n(ERR_get_error(), _errorMsg, MAX_SSL_ERROR_LENGTH);
                LOG_ERROR("Ssl connect failed:%s.", _errorMsg);
                sslClose();
                return -1;
            }
    } else {
        LOG_DEBUG("sslHandshake success.");
        return 0;
    }

//#if OPENSSL_VERSION_NUMBER < 0x10100000L
//#if defined(_WIN32)
//    pthread_mutex_unlock(&_sslMutex);
//#endif
//#endif /*OPENSSL_VERSION_NUMBER < 0x10100000L */

}

int SslConnect::sslWrite(const uint8_t * buffer, size_t len) {
    int wLen = SSL_write(_ssl, (void *)buffer, (int)len);
    if (wLen < 0) {
        int eCode = SSL_get_error(_ssl, wLen);
        if (eCode == SSL_ERROR_WANT_READ ||
            eCode == SSL_ERROR_WANT_WRITE) {
            LOG_DEBUG("Write could not complete. Will be invoked later.");
            return 0;
        } else if (eCode == SSL_ERROR_SYSCALL) {
            int errno_code = getLastErrorCode();
            LOG_INFO("SSL_write failed:%d.", errno_code);
            if (NLS_ERR_CONNECT_RETRIABLE(errno_code) || NLS_ERR_RW_RETRIABLE(errno_code)) {
                return 0;
            } else {
                memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
                ERR_error_string_n(ERR_get_error(), _errorMsg, MAX_SSL_ERROR_LENGTH);
                LOG_ERROR("SSL_ERROR_SYSCALL Ssl write failed:%s.", _errorMsg);
                return -1;
            }
        } else {
            memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
            ERR_error_string_n(ERR_get_error(), _errorMsg, MAX_SSL_ERROR_LENGTH);
            LOG_ERROR("Ssl write failed:%s.", _errorMsg);
            return -1;
        }
    } else {
        return wLen;
    }
}

int SslConnect::sslRead(uint8_t *  buffer, size_t len) {
    int rLen = SSL_read(_ssl, (void *)buffer, (int)len);
    if (rLen <= 0) {
        int eCode = SSL_get_error(_ssl, rLen);
        if (eCode == SSL_ERROR_WANT_READ ||
            eCode == SSL_ERROR_WANT_WRITE ||
            eCode == SSL_ERROR_WANT_X509_LOOKUP) {
//            LOG_DEBUG("Read could not complete. Will be invoked later.");
            return 0;
        } else if (eCode == SSL_ERROR_SYSCALL) {
            int errno_code = getLastErrorCode();
            LOG_INFO("SSL_read failed:%d.", errno_code);
            if (NLS_ERR_CONNECT_RETRIABLE(errno_code) || NLS_ERR_RW_RETRIABLE(errno_code)) {
                return 0;
            } else {
                memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
                ERR_error_string_n(eCode, _errorMsg, MAX_SSL_ERROR_LENGTH);
                LOG_ERROR("SSL_ERROR_SYSCALL Read failed:%d, %s.", eCode, _errorMsg);
                return -1;
            }
        } else {
            memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
            ERR_error_string_n(eCode, _errorMsg, MAX_SSL_ERROR_LENGTH);
            LOG_ERROR("Read failed:%d, %s.", eCode, _errorMsg);
            return -1;
        }
    } else {
        return rLen;
    }
}

void SslConnect::sslClose() {
    if (_ssl) {
        LOG_DEBUG("ssl connect close.");

        SSL_shutdown(_ssl);
        SSL_free(_ssl);
        _ssl = NULL;
    }
}

const char* SslConnect::getFailedMsg() {
    return _errorMsg;
}

} //AlibabaNls
