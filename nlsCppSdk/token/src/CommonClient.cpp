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
#include <ctime>
#include <algorithm>
#include <sstream>
#include <string.h>
#include "CommonClient.h"
#include "SimpleCredentialsProvider.h"
#include "Utils.h"
#include "HmacSha1Signer.h"

#define BuffSize 32
#if defined(__linux__)
#define strcpy strcpy
#define sprintf sprintf
#define WindowsSize
#else
#define strcpy strcpy_s
#define sprintf sprintf_s
#define WindowsSize BuffSize, 
#endif

namespace AlibabaNlsCommon {

const std::string SERVICE_NAME = "Common";

CommonClient::CommonClient(const std::string & accessKeyId,
                           const std::string & accessKeySecret,
                           const std::string & stsToken,
                           const ClientConfiguration & configuration) : CoreClient(SERVICE_NAME, configuration) {

  credentialsProvider_ = new SimpleCredentialsProvider(
      accessKeyId, accessKeySecret, stsToken);

  signer_ = signer_ = new HmacSha1Signer();
}

CommonClient::~CommonClient() {
  if (credentialsProvider_) {
    delete credentialsProvider_;
    credentialsProvider_ = NULL;
  }

  if (signer_) {
    delete signer_;
    signer_ = NULL;
  }
}

CommonClient::JsonOutcome CommonClient::makeRequest(
    const std::string &endpoint,
    const CommonRequest &msg,
    HttpRequest::Method method) const {

  HttpClient::HttpResponseOutcome outcome =
      AttemptRequest(endpoint, msg, method);
  if (outcome.isSuccess())
    return JsonOutcome(std::string(
        outcome.result().body(), outcome.result().bodySize()));
  else
    return JsonOutcome(outcome.error());
}

CommonClient::CommonResponseOutcome CommonClient::commonResponse(
    const CommonRequest & request) const {

  CommonClient::JsonOutcome outcome =
      makeRequest(request.domain(), request, request.httpMethod());

  if (outcome.isSuccess())
    return CommonResponseOutcome(CommonResponse(outcome.result()));
  else
    return CommonResponseOutcome(Error(outcome.error()));
}

HttpRequest CommonClient::buildHttpRequest(const std::string & endpoint,
                                           const ServiceRequest & msg,
                                           HttpRequest::Method method) const {
  return buildHttpRequest(endpoint,
                          dynamic_cast<const CommonRequest& >(msg),
                          method);
}

HttpRequest CommonClient::buildHttpRequest(const std::string & endpoint,
                                           const CommonRequest &msg,
                                           HttpRequest::Method method) const {

  if (msg.requestPattern() == CommonRequest::FileTransPattern) {
    return buildFileTransHttpRequest(endpoint, msg, method);
  } else {
    return buildTokenHttpRpcRequest(endpoint, msg, method);
  }
}

std::string CommonClient::canonicalizedHeaders(
    const HttpMessage::HeaderCollection &headers) const {
  std::map <std::string, std::string> materials;
  HttpMessage::HeaderCollectionIterator p;

  for (p = headers.begin(); p != headers.end(); ++ p) {
    std::string key = p->first;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    if (key.find("x-acs-") != 0)
      continue;

    std::string value = p->second;
    StringReplace(value, "\t", " ");
    StringReplace(value, "\n", " ");
    StringReplace(value, "\r", " ");
    StringReplace(value, "\f", " ");
    materials[key] = value;
  } // for

  if (materials.empty())
    return std::string();

  std::stringstream ss;

  std::map <std::string, std::string>::const_iterator materialsIterator;
  for (materialsIterator = materials.begin(); 
                           materialsIterator != materials.end();
                           ++materialsIterator) {
    ss << materialsIterator->first << ":" << materialsIterator->second << "\n";
  }  // for

  return ss.str();
}

HttpRequest CommonClient::buildTokenHttpRequest(
    const std::string & endpoint,
    const CommonRequest &msg,
    HttpRequest::Method method) const {

  const Credentials credentials = credentialsProvider_->getCredentials();

  Url url;
  url.setScheme("http");
  url.setHost(endpoint);
  url.setPath(msg.resourcePath());

  CommonRequest::ParameterCollection params = msg.headerParameters();
  std::map<std::string, std::string> queryParams;

  CommonRequest::ParameterCollectionIterator p;
  for (p = params.begin(); p != params.end(); ++ p) {
    if (!p->second.empty()) {
      queryParams[p->first] = p->second;
    }
  }

  if (!queryParams.empty()) {
    std::stringstream queryString;
    std::map<std::string, std::string>::iterator p1;
    for (p1 = queryParams.begin(); p1 != queryParams.end(); ++ p1) {
      if (p1->second.empty())
        queryString << "&" << p1->first;
      else
        queryString << "&" << p1->first << "=" << p1->second;
    }

    url.setQuery(queryString.str().substr(1));
  }

  HttpRequest request(url);

  request.setMethod(method);
  request.setHeader("Accept", "application/json");

  std::stringstream ss;
  ss << msg.contentSize();
  request.setHeader("Content-Length", ss.str());
  request.setHeader("Content-Type",
      "application/octet-stream;charset=utf-8"); //"application/octet-stream");
  request.setHeader("Content-MD5",
      ComputeContentMD5(msg.content(), msg.contentSize()));

  std::time_t t = std::time(NULL);
  std::string date;
  char tmbuff[32] = {0};
  char tmWday[5] = {0};
  char tmMday[5] = {0};
  int day, week;
  struct tm gmt;

#if defined(__linux__)
  gmtime_r(&t, &gmt);
#else
  gmtime_s(&gmt, &t);
#endif

	day = gmt.tm_mon;
	week = gmt.tm_wday;

  switch (week) {
    case 0: strcpy(tmWday, "Sun"); break;
    case 1: strcpy(tmWday, "Mon"); break;
    case 2: strcpy(tmWday, "Tue"); break;
    case 3: strcpy(tmWday, "Wed"); break;
    case 4: strcpy(tmWday, "Thu"); break;
    case 5: strcpy(tmWday, "Fri"); break;
    case 6: strcpy(tmWday, "Sat"); break;
    default: break;
  }

  switch (day) {
    case 0: strcpy(tmMday, "Jan"); break;
    case 1: strcpy(tmMday, "Feb"); break;
    case 2: strcpy(tmMday, "Mar"); break;
    case 3: strcpy(tmMday, "Apr"); break;
    case 4: strcpy(tmMday, "May"); break;
    case 5: strcpy(tmMday, "Jun"); break;
    case 6: strcpy(tmMday, "Jul"); break;
    case 7: strcpy(tmMday, "Aug"); break;
    case 8: strcpy(tmMday, "Sep"); break;
    case 9: strcpy(tmMday, "Oct"); break;
    case 10: strcpy(tmMday, "Nov"); break;
    case 11: strcpy(tmMday, "Dec"); break;
    default: break;
  }

  sprintf(tmbuff, WindowsSize"%s, %02d %s %d %02d:%02d:%02d",
      tmWday,
      gmt.tm_mday,
      tmMday,
      1900 + gmt.tm_year,
      gmt.tm_hour,
      gmt.tm_min,
      gmt.tm_sec
      );

  //std::cout<< "Tmp tmbuff:" << tmbuff << std::endl;

  date = tmbuff;
  date += " GMT";
  request.setHeader("Date", date);
  request.setHeader("Host", url.host());
  request.setHeader("x-sdk-client", std::string("CPP/").append("1.3.2"));
  //request.setHeader("x-acs-region-id", configuration().regionId());
  if (!credentials.sessionToken().empty())
    request.setHeader("x-acs-security-token", credentials.sessionToken());

  request.setHeader("x-acs-signature-method", signer_->name());
  //request.setHeader("x-acs-signature-nonce", GenerateUuid());
  request.setHeader("x-acs-signature-version", signer_->version());
  request.setHeader("x-acs-version", msg.version());

  request.setHeader("x-sdk-invoke-type", "common");
  request.setHeader("Cache-Control", "no-cache");
  request.setHeader("Pragma", "no-cache");
  request.setHeader("Connection", "keep-alive");

  std::stringstream plaintext;
  plaintext << HttpMethodToString(method) << "\n"
            << request.header("Accept") << "\n"
            << request.header("Content-MD5") << "\n"
            << request.header("Content-Type") << "\n"
            << request.header("Date") << "\n"
            << canonicalizedHeaders(request.headers());
  if (!url.hasQuery()) {
    plaintext << url.path();
  } else {
    plaintext << url.path() << "?" << url.query();
  }

  std::stringstream sign;
  sign << "acs "
       << credentials.accessKeyId()
       << ":"
       << signer_->generate(plaintext.str(), credentials.accessKeySecret());
  request.setHeader("Authorization", sign.str());

  return request;
}

HttpRequest CommonClient::buildTokenHttpRpcRequest(const std::string & endpoint,
                                                   const CommonRequest &msg,
                                                   HttpRequest::Method method) const {
    const Credentials credentials = credentialsProvider_->getCredentials();

    Url url;
    url.setScheme("http");
    url.setHost(endpoint);
    url.setPath(msg.resourcePath());

    std::map <std::string, std::string> queryParams;

    queryParams["AccessKeyId"] = credentials.accessKeyId();
    queryParams["Action"] = msg.action();
    queryParams["Format"] = "JSON";
    queryParams["RegionId"] = configuration().regionId();
    queryParams["SignatureMethod"] = "HMAC-SHA1";
    queryParams["SignatureNonce"] = GenerateUuid();
    queryParams["SignatureVersion"] = "1.0";

    std::time_t t = std::time(NULL);
    std::string date;
    struct tm gmt;
    char tmbuff[32] = {0};

#if defined(__linux__)
    gmtime_r(&t, &gmt);
#else
    gmtime_s(&gmt, &t);
#endif

    sprintf(tmbuff, WindowsSize"%d-%02d-%02dT%02d:%02d:%02dZ",
            1900 + gmt.tm_year,
            1 + gmt.tm_mon,
            gmt.tm_mday,
            gmt.tm_hour,
            gmt.tm_min,
            gmt.tm_sec
    );

    date = tmbuff;

//    std::cout<< "Timestamp: " << date << std::endl << std::endl;

    queryParams["Timestamp"] = date;

    queryParams["Version"] = msg.version();

//    std::cout<< "Method: " << HttpMethodToString(method) << std::endl << std::endl;

    std::stringstream filePlaintext;
    filePlaintext << HttpMethodToString(method)
                  << "&"
                  << UrlEncode("/")
                  << "&"
                  << UrlEncode(canonicalizedQuery(queryParams));

//    std::cout<< "Signature String: " << filePlaintext.str() << std::endl << std::endl;

    std::string tmp = signer_->generate(filePlaintext.str(), credentials.accessKeySecret() + "&");
    queryParams["Signature"] = tmp;

//    std::cout<< "Signature Result: " << tmp << std::endl << std::endl;

    std::stringstream queryString;
    std::map <std::string, std::string>::iterator p1;
    for(p1 = queryParams.begin(); p1 != queryParams.end(); ++ p1) {
        std::string key = p1->first;
        if (strncmp(key.c_str(), "Task", key.size()) != 0) {
            queryString << "&" << p1->first << "=" << UrlEncode(p1->second);
        }
    }
    url.setQuery(queryString.str().substr(1));

//    std::cout<< "URL: " << url.toString() << std::endl << std::endl;

    HttpRequest request(url);
    request.setMethod(method);
    request.setHeader("Accept", "application/json");

    if (msg.hasContent()) {
        request.setBody(msg.content(), msg.contentSize());

        std::stringstream sLength;
        sLength << msg.contentSize();
        request.setHeader("Content-Length", sLength.str());
        request.setHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
        request.setHeader("Content-MD5", ComputeContentMD5(msg.content(), msg.contentSize()));
    }

//    std::cout<< "Host: " << url.host() << std::endl;

    request.setHeader("Host", url.host());
    request.setHeader("Accept-Encoding", "identity");
    request.setHeader("x-sdk-client", std::string("CPP/").append("1.3.2"));
    request.setHeader("x-sdk-invoke-type", "common");
    request.setHeader("Cache-Control", "no-cache");
    request.setHeader("Pragma", "no-cache");
    request.setHeader("Connection", "keep-alive");

//    std::cout<< "End. " << std::endl;

    return request;
}

std::string CommonClient::canonicalizedQuery(const std::map<std::string, std::string>& params) const {
	if (params.empty())
		return std::string();

	std::stringstream ss;
    std::map <std::string, std::string>::iterator iter;
    std::map<std::string, std::string> tmpParams = params;
    for(iter = tmpParams.begin(); iter != tmpParams.end(); iter++) {
        std::string key = UrlEncode(iter->first);
        StringReplace(key, "+", "%20");
        StringReplace(key, "*", "%2A");
        StringReplace(key, "%7E", "~");
        std::string value = UrlEncode(iter->second);
        StringReplace(value, "+", "%20");
        StringReplace(value, "*", "%2A");
        StringReplace(value, "%7E", "~");
        ss << "&" << key << "=" << value;
    }

//    std::cout<< "Tmp Signal: " << ss.str() << std::endl << std::endl;

	return ss.str().substr(1);
}

HttpRequest CommonClient::buildFileTransHttpRequest(const std::string & endpoint,
                                                    const CommonRequest &msg,
                                                    HttpRequest::Method method) const {

    const Credentials credentials = credentialsProvider_->getCredentials();

    Url url;
    url.setScheme("http");
    url.setHost(endpoint);
    url.setPath(msg.resourcePath());

    std::map <std::string, std::string> queryParams;

    queryParams["AccessKeyId"] = credentials.accessKeyId();
    if (!credentials.stsToken().empty()) {
      queryParams["SecurityToken"] = credentials.stsToken();
    }
    queryParams["Action"] = msg.action();
    queryParams["Format"] = "JSON";
    queryParams["RegionId"] = configuration().regionId();
    queryParams["SignatureMethod"] = "HMAC-SHA1";
    queryParams["SignatureNonce"] = GenerateUuid();
    queryParams["SignatureVersion"] = "1.0";

    if (method == HttpRequest::Post) {
        queryParams["Task"] = msg.task();
//        std::cout<< "Task: " << msg.task() << std::endl << std::endl;
    } else {
        queryParams["TaskId"] = msg.taskId();
//        std::cout<< "TaskId: " << msg.taskId() << std::endl << std::endl;
    }

    std::time_t t = std::time(NULL);
    std::string date;
    struct tm gmt;
    char tmbuff[32] = {0};

#if defined(__linux__)
    gmtime_r(&t, &gmt);
#else
    gmtime_s(&gmt, &t);
#endif

    sprintf(tmbuff, WindowsSize"%d-%02d-%02dT%02d:%02d:%02dZ",
            1900 + gmt.tm_year,
            1 + gmt.tm_mon,
            gmt.tm_mday,
            gmt.tm_hour,
            gmt.tm_min,
            gmt.tm_sec
    );

    date = tmbuff;

//    std::cout<< "Timestamp: " << date << std::endl << std::endl;

    queryParams["Timestamp"] = date;
    queryParams["Version"] = msg.version();

//    std::cout<< "Method: " << HttpMethodToString(method) << std::endl << std::endl;

    std::stringstream filePlaintext;
    filePlaintext << HttpMethodToString(method)
                  << "&"
                  << UrlEncode(url.path())
                  << "&"
                  << UrlEncode(canonicalizedQuery(queryParams));

//    std::cout<< "Signature String: " << filePlaintext.str() << std::endl << std::endl;

    std::string tmp = signer_->generate(filePlaintext.str(), credentials.accessKeySecret() + "&");
    queryParams["Signature"] = tmp;

//    std::cout<< "Signature Result: " << tmp << std::endl << std::endl;

    std::stringstream queryString;
    std::map <std::string, std::string>::iterator p1;
    for(p1 = queryParams.begin(); p1 != queryParams.end(); ++ p1) {
        std::string key = p1->first;
        if (strncmp(key.c_str(), "Task", key.size()) != 0) {
            queryString << "&" << p1->first << "=" << UrlEncode(p1->second);
        }
    }
    url.setQuery(queryString.str().substr(1));

//    std::cout<< "URL: " << url.toString() << std::endl << std::endl;

    HttpRequest request(url);
    request.setMethod(method);
    request.setHeader("Accept", "application/json");

    if (msg.hasContent()) {
        std::string body = "Task=" + UrlEncode(msg.task());
        request.setBody(body.c_str(), body.size());
        std::stringstream sLength;
        sLength << body.size();
        request.setHeader("Content-Length", sLength.str());
        request.setHeader("Content-Type", "application/x-www-form-urlencoded;charset=utf-8");
        request.setHeader("Content-MD5", ComputeContentMD5(msg.content(), msg.contentSize()));
    }

//    std::cout<< "Host: " << url.host() << std::endl;

    request.setHeader("Host", url.host());
    request.setHeader("Accept-Encoding", "identity");
    request.setHeader("x-sdk-client", std::string("CPP/").append("1.3.2"));
    request.setHeader("x-sdk-invoke-type", "common");
    request.setHeader("Cache-Control", "no-cache");
    request.setHeader("Pragma", "no-cache");
    request.setHeader("Connection", "keep-alive");

//    std::cout<< "End. " << std::endl;

    return request;
}

}
