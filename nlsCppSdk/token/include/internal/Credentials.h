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

#ifndef ALIBABANLS_COMMON_CREDENTIAL_H_
#define ALIBABANLS_COMMON_CREDENTIAL_H_

#include <string>

namespace AlibabaNlsCommon {

class Credentials{
 public:
  Credentials(const std::string &accessKeyId,
              const std::string &accessKeySecret,
              const std::string &stsToken="",
              const std::string &sessionToken="");
  ~Credentials();

  std::string accessKeyId () const;
  std::string accessKeySecret () const;
  std::string stsToken () const;
  void setAccessKeyId(const std::string &accessKeyId);
  void setAccessKeySecret(const std::string &accessKeySecret);
  void setStsToken(const std::string &stsToken);
  void setSessionToken (const std::string &sessionToken);
  std::string sessionToken () const;

 private:
  std::string accessKeyId_;
  std::string accessKeySecret_;
  std::string stsToken_;
  std::string sessionToken_;
};

}

#endif // !ALIBABANLS_COMMON_CREDENTIAL_H_
