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

#ifndef ALIBABANLS_COMMON_ERROR_H_
#define ALIBABANLS_COMMON_ERROR_H_

#include <string>

namespace AlibabaNlsCommon {

class Error {
 public:
  Error() {};
  Error(std::string code, const std::string message);
  ~Error() {};

  std::string errorCode()const;
  std::string errorMessage() const;
  std::string host() const;
  std::string requestId() const;
  void setErrorCode(const std::string &code);
  void setErrorMessage(const std::string& message);
  void setHost(const std::string& host);
  void setRequestId(const std::string& request);

 private:
  std::string errorCode_;
  std::string message_;
  std::string host_;
  std::string requestId_;
};

}

#endif // !ALIBABANLS_COMMON_ERROR_H_
