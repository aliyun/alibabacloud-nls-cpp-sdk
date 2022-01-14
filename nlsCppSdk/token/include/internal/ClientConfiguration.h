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

#ifndef ALIBABANLS_COMMON_CLIENTCONFIGURATION_H_
#define ALIBABANLS_COMMON_CLIENTCONFIGURATION_H_

#include <memory>
#include <string>
#include "CredentialsProvider.h"
#include "NetworkProxy.h"
#include "Signer.h"

namespace AlibabaNlsCommon {

class ClientConfiguration {
 public:
  explicit ClientConfiguration(const std::string &regionId = "cn-hangzhou",
                               const NetworkProxy &proxy = NetworkProxy());
  ~ClientConfiguration();

  std::string endpoint()const;
  NetworkProxy proxy()const;
  std::string regionId()const;
  void setEndpoint(const std::string &endpoint);
  void setProxy(const NetworkProxy &proxy);
  void setRegionId(const std::string &regionId);

 private:
  std::string endpoint_;
  NetworkProxy proxy_;
  std::string regionId_;
};

}

#endif // !ALIBABANLS_COMMON_CLIENTCONFIGURATION_H_
