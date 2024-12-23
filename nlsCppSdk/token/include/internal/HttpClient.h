/*
 * Copyright 2009-2017 Alibaba Cloud All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ALIBABANLS_COMMON_HTTPCLIENT_H_
#define ALIBABANLS_COMMON_HTTPCLIENT_H_

#include "Error.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "NetworkProxy.h"
#include "Outcome.h"

namespace AlibabaNlsCommon {

class HttpClient {
 public:
  typedef Outcome<Error, HttpResponse> HttpResponseOutcome;

  HttpClient();
  virtual ~HttpClient();

  virtual HttpResponseOutcome makeRequest(const HttpRequest &request) = 0;

  NetworkProxy proxy() const;
  void setProxy(const NetworkProxy &proxy);

 private:
 private:
  NetworkProxy proxy_;
};

}  // namespace AlibabaNlsCommon
#endif
