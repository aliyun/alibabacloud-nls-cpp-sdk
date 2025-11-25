/*
 * Copyright 2025 Alibaba Group Holding Limited
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

#include "dashToken.h"

#include <iostream>
#include <sstream>

#include "CommonClient.h"
#include "json/json.h"
#include "nlog.h"

namespace AlibabaNlsCommon {

using namespace AlibabaNls;
using namespace utility;

DashToken::DashToken() {
  domain_ = "dashscope.aliyuncs.com";
  serverResourcePath_ = "/api/v1/tokens";
  apiKey_ = "";
  expireInSeconds_ = 60; /* 有效期为60秒，支持设置超时时间范围为[1, 1800]秒 */
  expireTime_ = 0; /* 过期UNIX时间戳 */
}

DashToken::~DashToken() {}

int DashToken::paramCheck() {
  if (apiKey_.empty()) {
    errorMsg_ = "APIKey is empty.";
    return -(InvalidApiKey);
  }

  if (serverResourcePath_.empty()) {
    errorMsg_ = "ServerResourcePath is empty.";
    return -(InvalidServerResource);
  }

  return Success;
}

int DashToken::applyDashToken() {
  int retCode = paramCheck();
  if (retCode < 0) {
    return retCode;
  }

  ClientConfiguration configuration("");
  CommonClient client("", "", "", configuration, apiKey_, expireInSeconds_);

  CommonRequest request(CommonRequest::DashPattern);
  request.setDomain(domain_);
  request.setResourcePath(serverResourcePath_);
  request.setHttpMethod(HttpRequest::Post);

  CommonClient::CommonResponseOutcome outcome = client.commonResponse(request);
  if (!outcome.isSuccess()) {
    // 异常处理
    errorMsg_ = outcome.error().errorMessage();
    return -(ClientRequestFaild);
  }

  try {
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string result = outcome.result().payload();
    std::istringstream iss(result);

    if (!Json::parseFromStream(reader, iss, &root, NULL)) {
      std::string tt = "json any failed.";
      errorMsg_ = tt;
      return -(JsonParseFailed);
    } else {
      LOG_DEBUG("Dash token result: %s", result.c_str());
    }

    if (!root["token"].isNull()) {
      tokenId_ = root["token"].asString();
    }
    if (!root["expires_at"].isNull()) {
      expireTime_ = root["expires_at"].asUInt();
    }
  } catch (const std::exception& e) {
    return -(JsonParseFailed);
  }

  return Success;
}

const char* DashToken::getErrorMsg() { return errorMsg_.c_str(); }

const char* DashToken::getToken() { return tokenId_.c_str(); }

unsigned int DashToken::getExpireTime() { return expireTime_; }

void DashToken::setAPIKey(const std::string& apiKey) { apiKey_ = apiKey; }

void DashToken::setExpireInSeconds(unsigned int seconds) {
  expireInSeconds_ = seconds;
}

void DashToken::setServerResourcePath(const std::string& serverResourcePath) {
  serverResourcePath_ = serverResourcePath;
}

}  // namespace AlibabaNlsCommon
