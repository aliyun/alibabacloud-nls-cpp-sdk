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

#include "Credentials.h"

namespace AlibabaNlsCommon {

Credentials::Credentials(const std::string &accessKeyId,
    const std::string &accessKeySecret,
    const std::string &stsToken,
    const std::string &sessionToken) : accessKeyId_(accessKeyId),
  accessKeySecret_(accessKeySecret),
  stsToken_(stsToken),
  sessionToken_(sessionToken) {
}

Credentials::~Credentials() {
}

std::string Credentials::accessKeyId () const {
  return accessKeyId_;
}

std::string Credentials::accessKeySecret () const {
  return accessKeySecret_;
}

std::string Credentials::stsToken () const {
  return stsToken_;
}

void Credentials::setAccessKeyId(const std::string &accessKeyId) {
  accessKeyId_ = accessKeyId;
}

void Credentials::setAccessKeySecret(const std::string &accessKeySecret) {
  accessKeySecret_ = accessKeySecret;
}

void Credentials::setStsToken(const std::string &stsToken) {
  stsToken_ = stsToken;
}

void Credentials::setSessionToken (const std::string &sessionToken) {
  sessionToken_ = sessionToken;
}

std::string Credentials::sessionToken () const {
  return sessionToken_;
}

}
