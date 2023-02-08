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

#include <stdio.h>
#include <string.h>
#include "openssl/err.h"

#include "nlsGlobal.h"
#include "SSLconnect.h"
#include "connectNode.h"
#include "nlog.h"
#include "utility.h"

namespace AlibabaNls {

SSL_CTX* SSLconnect::_sslCtx = NULL;

SSLconnect::SSLconnect() {
  _ssl = NULL;
}

SSLconnect::~SSLconnect() {
  sslClose();
  LOG_DEBUG("Destroy SSLconnect done.");
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

  SSL_CTX_set_mode(_sslCtx,
      SSL_MODE_ENABLE_PARTIAL_WRITE |
      SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
      SSL_MODE_AUTO_RETRY);

  LOG_DEBUG("SSLconnect::init() done.");
  return 0;
}

void SSLconnect::destroy() {
  if (_sslCtx) {
    // LOG_DEBUG("free _sslCtx.");
    SSL_CTX_free(_sslCtx);
    _sslCtx = NULL;
  }

  LOG_DEBUG("SSLconnect::destroy() done.");
}

int SSLconnect::sslHandshake(int socketFd, const char* hostname) {
  // LOG_DEBUG("Begin sslHandshake.");
  if (_sslCtx == NULL) {
    return -(SslCtxEmpty);
  }

  int ret;
  if (_ssl == NULL) {
    _ssl = SSL_new(_sslCtx);
    if (_ssl == NULL) {
      memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
      const char *SSL_new_ret = "return of SSL_new: ";
      memcpy(_errorMsg, SSL_new_ret, strnlen(SSL_new_ret, 24));
      ERR_error_string_n(ERR_get_error(),
                         _errorMsg + strnlen(SSL_new_ret, 24),
                         MAX_SSL_ERROR_LENGTH);
      LOG_ERROR("Invoke SSL_new failed:%s.", _errorMsg);
      return -(SslNewFailed);
    }

    ret = SSL_set_fd(_ssl, socketFd);
    if (ret == 0) {
      memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
      const char *SSL_set_fd_ret = "return of SSL_set_fd: ";
      memcpy(_errorMsg, SSL_set_fd_ret, strnlen(SSL_set_fd_ret, 24));
      ERR_error_string_n(ERR_get_error(),
                         _errorMsg + strnlen(SSL_set_fd_ret, 24),
                         MAX_SSL_ERROR_LENGTH);
      LOG_ERROR("Invoke SSL_set_fd failed:%s.", _errorMsg);
      return -(SslSetFailed);
    }

    SSL_set_mode(_ssl,
        SSL_MODE_ENABLE_PARTIAL_WRITE |
        SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
        SSL_MODE_AUTO_RETRY);

    SSL_set_connect_state(_ssl);
  } else {
    // LOG_DEBUG("SSL has existed.");
  }

  int sslError;
  ret = SSL_connect(_ssl);
  if (ret < 0) {
    sslError = SSL_get_error(_ssl, ret);

    /* err == SSL_ERROR_ZERO_RETURN 
       "SSL_connect: close notify received from peer" */
    // sslError == SSL_ERROR_WANT_X509_LOOKUP
    // SSL_ERROR_SYSCALL
    if (sslError == SSL_ERROR_WANT_READ || sslError == SSL_ERROR_WANT_WRITE) {
      // LOG_DEBUG("sslHandshake continue.");
      return sslError;
    } else if (sslError == SSL_ERROR_SYSCALL) {
      int errno_code = utility::getLastErrorCode();
      LOG_INFO("SSL connect error_syscall failed, errno:%d.", errno_code);

      if (NLS_ERR_CONNECT_RETRIABLE(errno_code) ||
          NLS_ERR_RW_RETRIABLE(errno_code)) {
        return SSL_ERROR_WANT_READ;
      } else if (errno_code == 0) {
        LOG_DEBUG("SSL connect syscall success.");
        return Success;
      } else {
        return -(SslConnectFailed);
      }
    } else {
      memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
      const char *SSL_connect_ret = "return of SSL_connect: ";
      memcpy(_errorMsg, SSL_connect_ret, strnlen(SSL_connect_ret, 64));
      ERR_error_string_n(ERR_get_error(),
                         _errorMsg + strnlen(SSL_connect_ret, 64),
                         MAX_SSL_ERROR_LENGTH);
      LOG_ERROR("SSL connect failed:%s.", _errorMsg);
      sslClose();
      return -(SslConnectFailed);
    }
  } else {
    // LOG_DEBUG("sslHandshake success.");
    return Success;
  }
}

int SSLconnect::sslWrite(const uint8_t * buffer, size_t len) {
  int wLen = SSL_write(_ssl, (void *)buffer, (int)len);
  if (wLen < 0) {
    int eCode = SSL_get_error(_ssl, wLen);
    if (eCode == SSL_ERROR_WANT_READ ||
      eCode == SSL_ERROR_WANT_WRITE) {
      LOG_DEBUG("Write could not complete. Will be invoked later.");
      return 0;
    } else if (eCode == SSL_ERROR_SYSCALL) {
      int errno_code = utility::getLastErrorCode();
      LOG_INFO("SSL_write error_syscall failed, errno:%d.", errno_code);

      if (NLS_ERR_CONNECT_RETRIABLE(errno_code) ||
          NLS_ERR_RW_RETRIABLE(errno_code)) {
        return 0;
      } else if (errno_code == 0) {
        LOG_DEBUG("SSL_write syscall success.");
        return 0;
      } else {
        memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
        const char *SSL_write_ret = "return of SSL_write: ";
        memcpy(_errorMsg, SSL_write_ret, strnlen(SSL_write_ret, 64));
        ERR_error_string_n(ERR_get_error(),
                           _errorMsg + strnlen(SSL_write_ret, 64),
                           MAX_SSL_ERROR_LENGTH);
        LOG_ERROR("SSL_ERROR_SYSCALL Ssl write failed:%s.", _errorMsg);
        return -(SslWriteFailed);
      }
    } else {
      memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
      const char *SSL_write_ret = "return of SSL_write: ";
      memcpy(_errorMsg, SSL_write_ret, strnlen(SSL_write_ret, 64));
      ERR_error_string_n(ERR_get_error(),
                         _errorMsg + strnlen(SSL_write_ret, 64),
                         MAX_SSL_ERROR_LENGTH);
      LOG_ERROR("Ssl write failed:%s.", _errorMsg);
      return -(SslWriteFailed);
    }
  } else {
    return wLen;
  }
}

int SSLconnect::sslRead(uint8_t *  buffer, size_t len) {
  int rLen = SSL_read(_ssl, (void *)buffer, (int)len);
  if (rLen <= 0) {
    int eCode = SSL_get_error(_ssl, rLen);
    int errno_code = utility::getLastErrorCode();
    //LOG_WARN("Read maybe failed, get_error:%d", eCode);
    if (eCode == SSL_ERROR_WANT_READ ||
        eCode == SSL_ERROR_WANT_WRITE ||
        eCode == SSL_ERROR_WANT_X509_LOOKUP) {
      //LOG_DEBUG("Read could not complete. Will be invoked later.");
      return 0;
    } else if (eCode == SSL_ERROR_SYSCALL) {
      LOG_INFO("SSL_read error_syscall failed, errno:%d.", errno_code);

      if (NLS_ERR_CONNECT_RETRIABLE(errno_code) ||
          NLS_ERR_RW_RETRIABLE(errno_code)) {
        return 0;
      } else if (errno_code == 0) {
        LOG_DEBUG("SSL_write syscall success.");
        return 0;
      } else {
        memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
        const char *SSL_read_ret = "return of SSL_read: ";
        memcpy(_errorMsg, SSL_read_ret, strnlen(SSL_read_ret, 64));
        ERR_error_string_n(eCode,
                           _errorMsg + strnlen(SSL_read_ret, 64),
                           MAX_SSL_ERROR_LENGTH);
        LOG_ERROR("SSL_ERROR_SYSCALL Read failed:errno(%d) ecode(%d), %s.",
            errno_code, eCode, _errorMsg);
        return -(SslReadSysError);
      }
    } else {
      memset(_errorMsg, 0x0, MAX_SSL_ERROR_LENGTH);
//      ERR_error_string_n(eCode, _errorMsg, MAX_SSL_ERROR_LENGTH);
      const char *SSL_read_ret = "return of SSL_read: ";
      memcpy(_errorMsg, SSL_read_ret, strnlen(SSL_read_ret, 64));
      ERR_error_string_n(ERR_get_error(),
                         _errorMsg + strnlen(SSL_read_ret, 64),
                         MAX_SSL_ERROR_LENGTH);
      LOG_ERROR("Read failed:errno(%d) ecode(%d), %s.",
          errno_code, eCode, _errorMsg);
      return -(SslReadFailed);
    }
  } else {
    return rLen;
  }
}

/*
 * Description: 关闭TLS/SSL连接
 * Return:
 * Others:
 */
void SSLconnect::sslClose() {
  if (_ssl) {
    LOG_DEBUG("ssl connect close.");

    SSL_shutdown(_ssl);
    SSL_free(_ssl);
    _ssl = NULL;
  }
}

const char* SSLconnect::getFailedMsg() {
  return _errorMsg;
}

} //AlibabaNls
