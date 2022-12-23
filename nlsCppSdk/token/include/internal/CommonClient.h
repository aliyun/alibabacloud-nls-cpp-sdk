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

#ifndef ALIBABANLS_COMMON_COMMONCLIENT_H_
#define ALIBABANLS_COMMON_COMMONCLIENT_H_

#include "AsyncCallerContext.h"
#include "ClientConfiguration.h"
#include "CoreClient.h"
#include "CommonRequest.h"
#include "CommonResponse.h"
#include "CredentialsProvider.h"

namespace AlibabaNlsCommon {

class CommonClient : public CoreClient {
 public:
  typedef Outcome<Error, CommonResponse> CommonResponseOutcome;

  typedef Outcome<Error, std::string> JsonOutcome;

  CommonClient(const std::string &accessKeyId,
               const std::string &accessKeySecret,
               const std::string &stsToken,
               const ClientConfiguration &configuration);
  ~CommonClient();

  CommonResponseOutcome commonResponse(const CommonRequest &request)const;

 protected:
  virtual HttpRequest buildHttpRequest(const std::string & endpoint,
                                       const ServiceRequest & msg,
                                       HttpRequest::Method method) const;

  HttpRequest buildHttpRequest(const std::string & endpoint,
                               const CommonRequest &msg,
                               HttpRequest::Method method) const;

  HttpRequest buildTokenHttpRequest(const std::string & endpoint,
                                    const CommonRequest &msg,
                                    HttpRequest::Method method) const;

  HttpRequest buildTokenHttpRpcRequest(const std::string & endpoint,
                                       const CommonRequest &msg,
                                       HttpRequest::Method method) const;

  HttpRequest buildFileTransHttpRequest(const std::string & endpoint,
                                        const CommonRequest &msg,
                                        HttpRequest::Method method) const;

  JsonOutcome makeRequest(
      const std::string &endpoint,
      const CommonRequest &msg,
      HttpRequest::Method method = HttpRequest::Get ) const;

 private:
  std::string canonicalizedQuery(
      const std::map <std::string, std::string> &params) const;
  std::string canonicalizedHeaders(
      const HttpMessage::HeaderCollection &headers) const;

  CredentialsProvider* credentialsProvider_;
  Signer* signer_;
};

}

#endif // !ALIBABANLS_COMMON_COMMONCLIENT_H_
