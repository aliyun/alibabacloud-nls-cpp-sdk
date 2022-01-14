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
#include <string.h>
#include <algorithm>
#include "HttpMessage.h"

#if defined(_WIN32) && defined(_MSC_VER)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

namespace AlibabaNlsCommon {

std::string KnownHeaderMapper[] = {
  "Accept",
  "Accept-Charset",
  "Accept-Encoding",
  "Accept-Language",
  "Authorization",
  "Connection",
  "Content-Length",
  "Content-MD5",
  "Content-Type",
  "Date",
  "Host",
  "Server",
  "User-Agent"
};

HttpMessage::HttpMessage() : body_(NULL),
  bodySize_(0),
  headers_() {
}

HttpMessage::HttpMessage(const HttpMessage &other) : body_(NULL),
  bodySize_(other.bodySize_),
  headers_(other.headers_) {
    setBody(other.body_, other.bodySize_);
}

HttpMessage& HttpMessage::operator=(const HttpMessage &other) {
  if (this != &other) {
    if (body_) {
      free(body_);
    }
    body_ = NULL;
    bodySize_ = 0;
    headers_ = other.headers_;
    setBody(other.body_, other.bodySize_);
  }
  return *this;
}

void HttpMessage::addHeader(const HeaderNameType & name,
                            const HeaderValueType & value) {
  setHeader(name, value);
}

void HttpMessage::addHeader(KnownHeader header,
                            const HeaderValueType & value) {
  setHeader(header, value);
}

HttpMessage::HeaderValueType HttpMessage::header(
    const HeaderNameType & name) const {
  HeaderCollectionIterator it = headers_.find(name);
  if (it != headers_.end()) {
    return it->second;
  } else {
    return std::string();
  }
}

HttpMessage::HeaderCollection HttpMessage::headers() const {
  return headers_;
}

void HttpMessage::removeHeader(const HeaderNameType & name) {
  headers_.erase(name);
}

void HttpMessage::removeHeader(KnownHeader header) {
	removeHeader(KnownHeaderMapper[header]);
}

void HttpMessage::setHeader(const HeaderNameType & name,
                            const HeaderValueType & value) {
  headers_[name] = value;
}

void HttpMessage::setHeader(KnownHeader header, const std::string & value) {
  setHeader(KnownHeaderMapper[header], value);
}

HttpMessage::~HttpMessage() {
  //setBody(NULL, 0);
  if (body_) {
    free(body_);
    body_ = NULL;
  }
  bodySize_ = 0;
}

const char* HttpMessage::body() const {
  return body_;
}

size_t HttpMessage::bodySize() const {
  return bodySize_;
}

bool HttpMessage::hasBody() const {
	return (bodySize_ != 0);
}

HttpMessage::HeaderValueType HttpMessage::header(KnownHeader header) const {
  return this->header(KnownHeaderMapper[header]);
}

void HttpMessage::setBody(const char *data, size_t size) {
  if (size && data) {
    if (!body_) {
      body_ = (char* )malloc(sizeof(char)*size + 1);
      if (!body_) {
        return ;
      }
      memset(body_, 0x0, sizeof(char)*size + 1);
#if defined(_WIN32)
      strncpy_s(body_, size + 1, data, size);
#else
      strncpy(body_, data, size);
#endif
      bodySize_ = (size + 1);
    } else {
      body_ = (char* )realloc(body_, bodySize_ + size);
      if (!body_) {
        free(body_);
        body_ = NULL;
        bodySize_ = 0;
        return ;
      }
      memset(body_ + bodySize_, 0x0, size);

#if defined(_WIN32)
      strcat_s(body_, bodySize_ + size, data);
#else
      strncat(body_, data, size);
#endif
      bodySize_ += size;
    }

    //std::cout << "BodySize: " << bodySize_ << " __ " << size << std::endl;
  }
}

bool HttpMessage::nocaseLess::operator()(
    const std::string & s1, const std::string & s2) const {
  return strcasecmp(s1.c_str(), s2.c_str()) < 0;
}

}
