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

#ifndef ALIBABANLS_COMMON_HTTPMESSAGE_H_
#define ALIBABANLS_COMMON_HTTPMESSAGE_H_

#include <map>
#include <string>

namespace AlibabaNlsCommon {

class HttpMessage {
 private:
  struct nocaseLess {
    bool operator() (const std::string & s1, const std::string & s2) const;
  };

 public:
  enum KnownHeader {
    Accept,
    AcceptCharset,
    AcceptEncoding,
    AcceptLanguage,
    Authorization,
    Connection,
    ContentLength,
    ContentMD5,
    ContentType,
    Date,
    Host,
    Server,
    UserAgent
  };

  typedef std::string HeaderNameType;
  typedef std::string HeaderValueType;
  typedef std::map<HeaderNameType, HeaderValueType, nocaseLess> HeaderCollection;
  typedef std::map<HeaderNameType, HeaderValueType, nocaseLess>::const_iterator HeaderCollectionIterator;
  typedef std::map<HeaderNameType, HeaderValueType, nocaseLess>::iterator HeaderCollectionIterator2;

  HttpMessage(const HttpMessage &other);
  HttpMessage& operator=(const HttpMessage &other);
  virtual ~HttpMessage();

  void addHeader(const HeaderNameType &name, const HeaderValueType &value);
  void addHeader(KnownHeader header, const HeaderValueType &value);
  const char* body()const;
  size_t bodySize()const;
  bool hasBody()const;
  HeaderValueType header(const HeaderNameType &name)const;
  HeaderValueType header(KnownHeader header)const;
  HeaderCollection headers()const;
  void removeHeader(const HeaderNameType &name);
  void removeHeader(KnownHeader header);
  void setBody(const char *data, size_t size);
  void setHeader(const HeaderNameType &name, const HeaderValueType &value);
  void setHeader(KnownHeader header, const std::string &value);

 protected:
  HttpMessage();

 private:
  char *body_;
  size_t bodySize_;
  HeaderCollection headers_;
};

}

#endif // !ALIBABANLS_COMMON_HTTPMESSAGE_H_
