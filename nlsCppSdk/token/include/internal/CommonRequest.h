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

#ifndef ALIBABANLS_COMMON_COMMONREQUEST_H_
#define ALIBABANLS_COMMON_COMMONREQUEST_H_

#include <string>
#include "ServiceRequest.h"
#include "HttpRequest.h"

namespace AlibabaNlsCommon {

class CommonRequest : public ServiceRequest {
 public:
  enum RequestPattern {
    TokenPattern,
    FileTransPattern
  };

  explicit CommonRequest(RequestPattern pattern = TokenPattern);
  ~CommonRequest();

  std::string domain() const;
  ParameterValueType headerParameter(const ParameterNameType &name) const;
  ParameterCollection headerParameters() const;
  HttpRequest::Method httpMethod() const;
  ParameterValueType queryParameter(const ParameterNameType &name) const;
  ParameterCollection queryParameters() const;

  using ServiceRequest::setContent;
  void setDomain(const std::string &domain);
  void setHeaderParameter(const ParameterNameType &name,
                          const ParameterValueType &value);
  void setHttpMethod(HttpRequest::Method method);
  void setQueryParameter(const ParameterNameType &name,
                         const ParameterValueType &value);
  using ServiceRequest::setResourcePath;
  void setRequestPattern(RequestPattern pattern);
  using ServiceRequest::setVersion;
  RequestPattern requestPattern() const;

  using ServiceRequest::setAction;
  using ServiceRequest::setTask;
  using ServiceRequest::setTaskId;

 protected:
  using ServiceRequest::product;

 private:
  std::string domain_;
  RequestPattern requestPattern_;
  ParameterCollection queryParams_;
  ParameterCollection headerParams_;
  HttpRequest::Method httpMethod_;
};

}
#endif // !ALIBABANLS_COMMON_COMMONREQUEST_H_
