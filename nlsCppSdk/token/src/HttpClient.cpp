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

#include "HttpClient.h"
#include <cassert>
#include <vector>
#include <sstream>

namespace AlibabaNlsCommon {

HttpClient::HttpClient() : proxy_() {
}

HttpClient::~HttpClient() {
}

NetworkProxy HttpClient::proxy()const {
  return proxy_;
}

void HttpClient::setProxy(const NetworkProxy &proxy) {
  proxy_ = proxy;
}

}
