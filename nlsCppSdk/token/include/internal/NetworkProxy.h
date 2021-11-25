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

#ifndef ALIBABANLS_COMMON_NETWORKPROXY_H_
#define ALIBABANLS_COMMON_NETWORKPROXY_H_

#include <stdint.h>
#include <string>

namespace AlibabaNlsCommon {

class  NetworkProxy{
 public:
  enum Type {
    None = 0,
    Http,	
    Socks5
  };

  NetworkProxy(Type type = None,
               const std::string &hostName = "",
               uint16_t port = 0,
               const std::string &user = "",
               const std::string &password = "");
  ~NetworkProxy();

  std::string hostName() const;
  std::string password() const;
  uint16_t port() const;
  void setHostName(const std::string &hostName);
  void setPassword(const std::string &password);
  void setPort(uint16_t port);
  void setType(Type type);
  void setUser(const std::string &user);
  Type type() const;
  std::string user() const;

 private:
  std::string hostName_;
  std::string password_;
  uint16_t port_;
  Type type_;
  std::string user_;
};


}
#endif // !ALIBABANLS_COMMON_NETWORKPROXY_H_
