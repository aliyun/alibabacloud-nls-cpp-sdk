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

#include "SSLconnect.h"

#include <stdio.h>
#include <string.h>

#include "connectNode.h"
#include "nlog.h"
#include "nlsGlobal.h"
#include "openssl/err.h"
#include "utility.h"

namespace AlibabaNls {

SSL_CTX *SSLconnect::_sslCtx = NULL;

SSLconnect::SSLconnect() : _ssl(NULL), _sslTryAgain(0), _errorMsg() {
#if defined(_MSC_VER)
  _mtxSSL = CreateMutex(NULL, FALSE, NULL);
#else
  pthread_mutex_init(&_mtxSSL, NULL);
#endif
  LOG_DEBUG("Create SSLconnect:%p.", this);
}

SSLconnect::~SSLconnect() {
  sslClose();
  _sslTryAgain = 0;
#if defined(_MSC_VER)
  CloseHandle(_mtxSSL);
#else
  pthread_mutex_destroy(&_mtxSSL);
#endif
  LOG_DEBUG("SSL(%p) Destroy SSLconnect done.", this);
}

int SSLconnect::init() {
  if (_sslCtx == NULL) {
    _sslCtx = SSL_CTX_new(SSLv23_client_method());
    if (_sslCtx == NULL) {
      LOG_ERROR("SSL: couldn't create a context!");
      exit(1);
    }
  }

  SSL_CTX_set_verify(_sslCtx, SSL_VERIFY_NONE, NULL);

  SSL_CTX_set_mode(_sslCtx, SSL_MODE_ENABLE_PARTIAL_WRITE |
                                SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                                SSL_MODE_AUTO_RETRY);

  LOG_DEBUG("SSLconnect::init() done.");
  return Success;
}

void SSLconnect::destroy() {
  if (_sslCtx) {
    // LOG_DEBUG("free _sslCtx.");
    SSL_CTX_free(_sslCtx);
    _sslCtx = NULL;
  }

  LOG_DEBUG("SSLconnect::destroy() done.");
}

int SSLconnect::sslHandshake(int socketFd, const char *hostname) {
  // LOG_DEBUG("Begin sslHandshake.");
  if (_sslCtx == NULL) {
    LOG_ERROR("SSL(%p) _sslCtx has been released.", this);
    return -(SslCtxEmpty);
  }

  MUTEX_LOCK(_mtxSSL);

  int ret;
  if (_ssl == NULL) {
    _ssl = SSL_new(_sslCtx);
    if (_ssl == NULL) {
      memset(_errorMsg, 0x0, MaxSslErrorLength);
      const char *SSL_new_ret = "return of SSL_new: ";
      const int SSL_new_str_size = strnlen(SSL_new_ret, 24);
      memcpy(_errorMsg, SSL_new_ret, SSL_new_str_size);
      ERR_error_string_n(ERR_get_error(), _errorMsg + SSL_new_str_size,
                         MaxSslErrorLength - SSL_new_str_size - 1);
      LOG_ERROR("SSL(%p) Invoke SSL_new failed:%s.", this, _errorMsg);
      MUTEX_UNLOCK(_mtxSSL);
      return -(SslNewFailed);
    } else {
      if (hostname) {
        if (!SSL_set_tlsext_host_name(_ssl, hostname)) {
          LOG_ERROR("Error setting SNI host name");
        } else {
          LOG_INFO("Set SNI %s success", hostname);
        }
      }
    }

    ret = SSL_set_fd(_ssl, socketFd);
    if (ret == 0) {
      memset(_errorMsg, 0x0, MaxSslErrorLength);
      const char *SSL_set_fd_ret = "return of SSL_set_fd: ";
      const int SSL_set_fd_str_size = strnlen(SSL_set_fd_ret, 24);
      memcpy(_errorMsg, SSL_set_fd_ret, SSL_set_fd_str_size);
      ERR_error_string_n(ERR_get_error(), _errorMsg + SSL_set_fd_str_size,
                         MaxSslErrorLength - SSL_set_fd_str_size - 1);
      LOG_ERROR("SSL(%p) Invoke SSL_set_fd failed:%s.", this, _errorMsg);
      MUTEX_UNLOCK(_mtxSSL);
      return -(SslSetFailed);
    }

    SSL_set_mode(_ssl, SSL_MODE_ENABLE_PARTIAL_WRITE |
                           SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                           SSL_MODE_AUTO_RETRY);

    SSL_set_connect_state(_ssl);
  } else {
    // LOG_DEBUG("SSL has existed.");
  }

  ret = SSL_connect(_ssl);
  if (ret < 0) {
    int sslError = SSL_get_error(_ssl, ret);

    /* err == SSL_ERROR_ZERO_RETURN
       "SSL_connect: close notify received from peer" */
    // sslError == SSL_ERROR_WANT_X509_LOOKUP
    // SSL_ERROR_SYSCALL
    if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE) {
      // LOG_DEBUG("sslHandshake continue.");
      MUTEX_UNLOCK(_mtxSSL);
      return sslError;
    } else if (sslError == SSL_ERROR_SYSCALL) {
      int errno_code = utility::getLastErrorCode();
      LOG_INFO("SSL(%p) SSL connect error_syscall failed, errno:%d.", this,
               errno_code);

      if (NLS_ERR_CONNECT_RETRIABLE(errno_code) ||
          NLS_ERR_RW_RETRIABLE(errno_code)) {
        MUTEX_UNLOCK(_mtxSSL);
        return SSL_ERROR_WANT_READ;
      } else if (errno_code == 0) {
        LOG_DEBUG("SSL(%p) SSL connect syscall success.", this);
        MUTEX_UNLOCK(_mtxSSL);
        return Success;
      } else {
        MUTEX_UNLOCK(_mtxSSL);
        return -(SslConnectFailed);
      }
    } else {
      memset(_errorMsg, 0x0, MaxSslErrorLength);
      const char *SSL_connect_ret = "return of SSL_connect: ";
      const int SSL_connect_str_size = strnlen(SSL_connect_ret, 64);
      memcpy(_errorMsg, SSL_connect_ret, SSL_connect_str_size);
      ERR_error_string_n(ERR_get_error(), _errorMsg + SSL_connect_str_size,
                         MaxSslErrorLength - SSL_connect_str_size - 1);
      LOG_ERROR("SSL(%p) SSL connect failed:%s.", this, _errorMsg);
      MUTEX_UNLOCK(_mtxSSL);
      this->sslClose();
      return -(SslConnectFailed);
    }
  } else {
    // LOG_DEBUG("sslHandshake success.");
    MUTEX_UNLOCK(_mtxSSL);
    return Success;
  }

  MUTEX_UNLOCK(_mtxSSL);
  return Success;
}

int SSLconnect::sslWrite(const uint8_t *buffer, size_t len) {
  MUTEX_LOCK(_mtxSSL);

  if (_ssl == NULL) {
    LOG_ERROR("SSL(%p) ssl has been closed.", this);
    MUTEX_UNLOCK(_mtxSSL);
    return -(SslWriteFailed);
  }

  int wLen = SSL_write(_ssl, (void *)buffer, (int)len);
  if (wLen < 0) {
    int sslError = SSL_get_error(_ssl, wLen);
    int errno_code = utility::getLastErrorCode();
    char sslErrMsg[MaxSslErrorLength] = {0};
    const char *SSL_write_ret = "return of SSL_write: ";
    const int SSL_write_str_size = strnlen(SSL_write_ret, 64);
    if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE) {
      LOG_DEBUG("SSL(%p) Write could not complete. Will be invoked later.",
                this);
      MUTEX_UNLOCK(_mtxSSL);
      return 0;
    } else if (sslError == SSL_ERROR_SYSCALL) {
      LOG_INFO("SSL(%p) SSL_write error_syscall failed, errno:%d.", this,
               errno_code);

      if (NLS_ERR_CONNECT_RETRIABLE(errno_code) ||
          NLS_ERR_RW_RETRIABLE(errno_code)) {
        MUTEX_UNLOCK(_mtxSSL);
        return 0;
      } else if (errno_code == 0) {
        LOG_DEBUG("SSL(%p) SSL_write syscall success.", this);
        MUTEX_UNLOCK(_mtxSSL);
        return 0;
#ifdef _MSC_VER
      } else if (errno_code == WSAECONNRESET) {
#else
      } else if (errno_code == ECONNRESET) {
#endif
        memset(_errorMsg, 0x0, MaxSslErrorLength);
        memcpy(sslErrMsg, SSL_write_ret, SSL_write_str_size);
        ERR_error_string_n(ERR_get_error(), sslErrMsg + SSL_write_str_size,
                           MaxSslErrorLength - SSL_write_str_size - 1);
        snprintf(_errorMsg, MaxSslErrorLength,
                 "%s. It's mean the remote end was "
                 "closed because of bad network. errno_code:%d, ssl_eCode:%d.",
                 sslErrMsg, errno_code, sslError);
        LOG_ERROR("SSL(%p) SSL_ERROR_SYSCALL Write failed, %s.", this,
                  _errorMsg);
        MUTEX_UNLOCK(_mtxSSL);
        return -(SslWriteFailed);
      } else {
        memset(_errorMsg, 0x0, MaxSslErrorLength);
        memcpy(sslErrMsg, SSL_write_ret, SSL_write_str_size);
        ERR_error_string_n(ERR_get_error(), sslErrMsg + SSL_write_str_size,
                           MaxSslErrorLength - SSL_write_str_size - 1);
        snprintf(_errorMsg, MaxSslErrorLength,
                 "%s. errno_code:%d ssl_eCode:%d.", sslErrMsg, errno_code,
                 sslError);
        LOG_ERROR("SSL(%p) SSL_ERROR_SYSCALL Write failed: %s.", this,
                  _errorMsg);
        MUTEX_UNLOCK(_mtxSSL);
        return -(SslWriteFailed);
      }
    } else {
      memset(_errorMsg, 0x0, MaxSslErrorLength);
      memcpy(sslErrMsg, SSL_write_ret, SSL_write_str_size);
      ERR_error_string_n(ERR_get_error(), sslErrMsg + SSL_write_str_size,
                         MaxSslErrorLength - SSL_write_str_size - 1);
      if (sslError == SSL_ERROR_ZERO_RETURN && errno_code == 0) {
        snprintf(
            _errorMsg, MaxSslErrorLength,
            "%s. errno_code:%d ssl_eCode:%d. It's mean this connection was "
            "closed or shutdown because of bad network.",
            sslErrMsg, errno_code, sslError);
      } else {
        snprintf(_errorMsg, MaxSslErrorLength,
                 "%s. errno_code:%d ssl_eCode:%d.", sslErrMsg, errno_code,
                 sslError);
      }
      LOG_ERROR("SSL(%p) SSL_write failed: %s.", this, _errorMsg);
      MUTEX_UNLOCK(_mtxSSL);
      return -(SslWriteFailed);
    }
  }

  MUTEX_UNLOCK(_mtxSSL);
  return wLen;
}

int SSLconnect::sslRead(uint8_t *buffer, size_t len) {
  MUTEX_LOCK(_mtxSSL);

  if (_ssl == NULL) {
    LOG_ERROR("SSL(%p) ssl has been closed.", this);
    MUTEX_UNLOCK(_mtxSSL);
    return -(SslReadFailed);
  }

  int rLen = SSL_read(_ssl, (void *)buffer, (int)len);
  if (rLen <= 0) {
    int sslError = SSL_get_error(_ssl, rLen);
    int errno_code = utility::getLastErrorCode();
    char sslErrMsg[MaxSslErrorLength] = {0};
    const char *SSL_read_ret = "return of SSL_read: ";
    const int SSL_read_str_size = strnlen(SSL_read_ret, 64);
    // LOG_WARN("Read maybe failed, get_ssl_error:%d", sslError);
    if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE ||
        sslError == SSL_ERROR_WANT_X509_LOOKUP) {
      // LOG_DEBUG("SSL(%p) Read could not complete. Will be invoked later.",
      // this);
      MUTEX_UNLOCK(_mtxSSL);
      return 0;
    } else if (sslError == SSL_ERROR_SYSCALL) {
      LOG_INFO("SSL(%p) SSL_read error_syscall failed, errno:%d.", this,
               errno_code);

      if (NLS_ERR_CONNECT_RETRIABLE(errno_code) ||
          NLS_ERR_RW_RETRIABLE(errno_code)) {
        LOG_WARN("SSL(%p) Retry read...", this);
        MUTEX_UNLOCK(_mtxSSL);
        return 0;
      } else if (errno_code == 0) {
        LOG_DEBUG("SSL(%p) SSL_read syscall success.", this);
        MUTEX_UNLOCK(_mtxSSL);
        return 0;
#ifdef _MSC_VER
      } else if (errno_code == WSAECONNRESET) {
#else
      } else if (errno_code == ECONNRESET) {
#endif
        memset(_errorMsg, 0x0, MaxSslErrorLength);
        memcpy(sslErrMsg, SSL_read_ret, SSL_read_str_size);
        ERR_error_string_n(ERR_get_error(), sslErrMsg + SSL_read_str_size,
                           MaxSslErrorLength - SSL_read_str_size - 1);
        snprintf(_errorMsg, MaxSslErrorLength,
                 "%s. It's mean the remote end was "
                 "closed because of bad network. errno_code:%d, ssl_eCode:%d.",
                 sslErrMsg, errno_code, sslError);
        LOG_ERROR("SSL(%p) SSL_ERROR_SYSCALL Read failed, %s.", this,
                  _errorMsg);
        MUTEX_UNLOCK(_mtxSSL);
        return -(SslReadSysError);
      } else {
        memset(_errorMsg, 0x0, MaxSslErrorLength);
        memcpy(sslErrMsg, SSL_read_ret, SSL_read_str_size);
        ERR_error_string_n(ERR_get_error(), sslErrMsg + SSL_read_str_size,
                           MaxSslErrorLength - SSL_read_str_size - 1);
        snprintf(_errorMsg, MaxSslErrorLength,
                 "%s. errno_code:%d, ssl_eCode:%d.", sslErrMsg, errno_code,
                 sslError);
        LOG_ERROR("SSL(%p) SSL_ERROR_SYSCALL Read failed, %s.", this,
                  _errorMsg);
        MUTEX_UNLOCK(_mtxSSL);
        return -(SslReadSysError);
      }
    } else {
      memset(_errorMsg, 0x0, MaxSslErrorLength);
      memcpy(sslErrMsg, SSL_read_ret, strnlen(SSL_read_ret, 64));
      ERR_error_string_n(ERR_get_error(), sslErrMsg + SSL_read_str_size,
                         MaxSslErrorLength - SSL_read_str_size - 1);
      if (sslError == SSL_ERROR_ZERO_RETURN && errno_code == 0 &&
          ++_sslTryAgain <= MaxSslTryAgain) {
        snprintf(
            _errorMsg, MaxSslErrorLength,
            "%s. errno_code:%d ssl_eCode:%d. It's mean this connection was "
            "closed or shutdown because of bad network, Try again ...",
            sslErrMsg, errno_code, sslError);
        LOG_WARN("SSL(%p) SSL_read failed: %s.", this, _errorMsg);
        MUTEX_UNLOCK(_mtxSSL);
        return 0;
      } else {
        snprintf(_errorMsg, MaxSslErrorLength,
                 "%s. errno_code:%d ssl_eCode:%d.", sslErrMsg, errno_code,
                 sslError);
      }
      LOG_ERROR("SSL(%p) SSL_read failed: %s.", this, _errorMsg);
      MUTEX_UNLOCK(_mtxSSL);
      return -(SslReadFailed);
    }
  }

  _sslTryAgain = 0;

  MUTEX_UNLOCK(_mtxSSL);
  return rLen;
}

/**
 * @brief: 关闭TLS/SSL连接
 * @return:
 */
void SSLconnect::sslClose() {
  MUTEX_LOCK(_mtxSSL);

  if (_ssl) {
    LOG_INFO("SSL(%p) ssl connect close.", this);

    SSL_shutdown(_ssl);
    SSL_free(_ssl);
    _ssl = NULL;
  } else {
    LOG_DEBUG("SSL connect has closed.");
  }

  MUTEX_UNLOCK(_mtxSSL);
}

const char *SSLconnect::getFailedMsg() { return _errorMsg; }

}  // namespace AlibabaNls
