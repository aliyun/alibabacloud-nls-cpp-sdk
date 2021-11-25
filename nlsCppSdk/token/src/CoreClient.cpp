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

#include <iostream>
#include "CoreClient.h"
#include "json/json.h"
#include "Signer.h"
#include "CurlHttpClient.h"

namespace AlibabaNlsCommon {

CoreClient::CoreClient(const std::string & servicename,
    const ClientConfiguration &configuration) : serviceName_(servicename),
  configuration_(configuration) {
    httpClient_ = new CurlHttpClient();
    httpClient_->setProxy(configuration.proxy());
}

CoreClient::~CoreClient() {
  delete httpClient_;
}

ClientConfiguration CoreClient::configuration()const {
  return configuration_;
}

std::string CoreClient::serviceName()const {
  return serviceName_;
}

HttpClient::HttpResponseOutcome CoreClient::AttemptRequest(
    const std::string & endpoint,
    const ServiceRequest & request,
    HttpRequest::Method method) const {
  HttpRequest r = buildHttpRequest(endpoint, request, method);
  HttpClient::HttpResponseOutcome outcome = httpClient_->makeRequest(r);

  if (!outcome.isSuccess())
    return outcome;

  if(hasResponseError(outcome.result()))
    return HttpClient::HttpResponseOutcome(buildCoreError(outcome.result()));
  else
    return outcome;
}

Error CoreClient::buildCoreError(const HttpResponse &response) const {
  Json::Reader reader;
  Json::Value value;

  if (!reader.parse(std::string(response.body(), response.bodySize()), value))
    return Error("InvalidResponse", "");

  Error error;
  error.setErrorCode(value["Code"].asString());
  error.setErrorMessage(value["Message"].asString());
  error.setHost(value["HostId"].asString());
  error.setRequestId(value["RequestId"].asString());

  return error;
}

bool CoreClient::hasResponseError(const HttpResponse &response) const {
  return response.statusCode() < 200 || response.statusCode() > 299;
}

}
