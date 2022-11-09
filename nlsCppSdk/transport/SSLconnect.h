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
#ifndef NLS_SDK_SSL_CONNECT_H
#define NLS_SDK_SSL_CONNECT_H

#include <stdint.h>
#include <string>
#include "openssl/ssl.h"
#include "error.h"

namespace AlibabaNls {

#define MAX_SSL_ERROR_LENGTH 256

class SSLconnect {

  static SSL_CTX* _sslCtx;

 public:
  SSLconnect();
  ~SSLconnect();

  static int init();
  static void destroy();

  int sslHandshake(int socketFd, const char* hostname); //hostname暂不使用
  int sslWrite(const uint8_t * buffer, size_t len);
  int sslRead(uint8_t * buffer, size_t len);
  void sslClose();

  const char* getFailedMsg();

 private:
  SSL* _ssl;
  char _errorMsg[MAX_SSL_ERROR_LENGTH];
};

} // namespace AlibabaNls

#endif // NLS_SDK_SSL_CONNECT_H
