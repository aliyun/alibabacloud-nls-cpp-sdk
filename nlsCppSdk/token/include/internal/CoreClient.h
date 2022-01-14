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

#ifndef ALIBABANLS_COMMON_CORECLIENT_H_
#define ALIBABANLS_COMMON_CORECLIENT_H_

#include <functional>
#include <memory>
#include "ClientConfiguration.h"
#include "HttpClient.h"
#include "HttpResponse.h"
#include "HttpRequest.h"
#include "Outcome.h"
#include "ServiceRequest.h"
#include "CurlHttpClient.h"

namespace AlibabaNlsCommon {

class CoreClient {
 public:
  CoreClient(const std::string & servicename,
             const ClientConfiguration &configuration);
  virtual ~CoreClient();

  ClientConfiguration configuration() const;
  std::string serviceName() const;

 protected:
  HttpClient::HttpResponseOutcome AttemptRequest(const std::string & endpoint,
      const ServiceRequest &request,
      HttpRequest::Method method) const;

  Error buildCoreError(const HttpResponse &response) const;

  bool hasResponseError(const HttpResponse &response) const;

  virtual HttpRequest buildHttpRequest(const std::string & endpoint,
      const ServiceRequest &msg,
      HttpRequest::Method method) const = 0;

 private:
  std::string serviceName_;
  ClientConfiguration configuration_;
  CurlHttpClient *httpClient_;

};

}

#endif
