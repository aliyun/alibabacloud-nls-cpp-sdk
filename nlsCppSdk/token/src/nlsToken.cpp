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
#include "CommonClient.h"
#include "json/json.h"
#include "nlsToken.h"

namespace AlibabaNlsCommon {

NlsToken::NlsToken() {
  domain_ = "nls-meta.cn-shanghai.aliyuncs.com";
  serverVersion_ = "2019-02-28";
  serverResourcePath_ = "/pop/2019-02-28/tokens";
  accessKeyId_ = "";
  accessKeySecret_ = "";
  regionId_ = "cn-shanghai";
  action_ = "CreateToken";
}

NlsToken::~NlsToken() {}

int NlsToken::paramCheck() {
  if (accessKeySecret_.empty()) {
    errorMsg_ = "AccessKeySecret is empty.";
    return -(InvalidAkSecret);
  }

  if (accessKeyId_.empty()) {
    errorMsg_ = "AccessKeyId is empty.";
    return -(InvalidAkId);
  }

  if (domain_.empty()) {
    errorMsg_ = "Domain is empty.";
    return -(InvalidDomain);
  }

  if (serverVersion_.empty()) {
    errorMsg_ = "ServerVersion is empty.";
    return -(InvalidServerVersion);
  }

  if (serverResourcePath_.empty()) {
    errorMsg_ = "ServerResourcePath is empty.";
    return -(InvalidServerResource);
  }

  if (action_.empty()) {
    errorMsg_ = "Action is empty.";
    return -(InvalidAction);
  }

  if (regionId_.empty()) {
    errorMsg_ = "RegionId is empty.";
    return -(InvalidRegionId);
  }

  return Success;
}

int NlsToken::applyNlsToken() {
  int retCode = paramCheck();
  if (retCode < 0) {
    return retCode;
  }

  //ClientConfiguration默认区域id为hangzhou
  ClientConfiguration configuration(regionId_);
  //if (!regionId_.empty()) {
  //  configuration.setRegionId(regionId_);
  //}

  CommonClient client(accessKeyId_, accessKeySecret_, "", configuration);

  CommonRequest request(CommonRequest::TokenPattern);
  request.setDomain(domain_);
  request.setVersion(serverVersion_);
  request.setResourcePath(serverResourcePath_);
  request.setHttpMethod(HttpRequest::Post);
  request.setAction(action_);

  //std::cout << "Domain: " << domain_ << std::endl;
  //std::cout << "ServerVersion: " << serverVersion_ << ", ServerResourcePath: " << serverResourcePath_ << std::endl;
  //std::cout << "Action: " << action_ << std::endl;
  //std::cout << "RegionId: " << regionId_ << std::endl;

  CommonClient::CommonResponseOutcome outcome = client.commonResponse(request);
  if (!outcome.isSuccess()) {
    // 异常处理
    errorMsg_ = outcome.error().errorMessage();
    return -(ClientRequestFaild);
  }

  Json::Value root;
  Json::Reader reader;
  std::string result = outcome.result().payload();

  if (!reader.parse(result, root)) {
    std::string tt = "json any failed.";
    errorMsg_ = tt;
    return -(JsonParseFailed);
  }

  if (!root["Token"].isNull()) {
    Json::Value subRoot = root["Token"];

    if (!subRoot["Id"].isNull()) {
      tokenId_ = subRoot["Id"].asString();
    }

    if (!subRoot["ExpireTime"].isNull()) {
      expireTime_ = subRoot["ExpireTime"].asUInt();
    }
  }

  return Success;
}

const char* NlsToken::getErrorMsg() {
  return errorMsg_.c_str();
}

const char* NlsToken::getToken() {
  return tokenId_.c_str();
}

unsigned int NlsToken::getExpireTime() {
  return expireTime_;
}

void NlsToken::setKeySecret(const std::string & KeySecret) {
  accessKeySecret_ = KeySecret;
}

void NlsToken::setAccessKeyId(const std::string & accessKeyId) {
  accessKeyId_ = accessKeyId;
}

void NlsToken::setDomain(const std::string & domain) {
  domain_ = domain;
}

void NlsToken::setServerVersion(const std::string & serverVersion) {
  serverVersion_ = serverVersion;
  serverResourcePath_ = "/pop/";
  serverResourcePath_ += serverVersion_;
  serverResourcePath_ += "/tokens";
}

void NlsToken::setServerResourcePath(const std::string & serverResourcePath) {
  serverResourcePath_ = serverResourcePath;
}

void NlsToken::setRegionId(const std::string & regionId) {
  regionId_ = regionId;
}

void NlsToken::setAction(const std::string & action) {
  action_ = action;
}

}
