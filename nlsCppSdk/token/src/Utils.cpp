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

#include "Utils.h"
#include <curl/curl.h>
#ifdef _MSC_VER
#include <Windows.h>
#include <Rpc.h>
#include <wincrypt.h>
#else
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <uuid/uuid.h>
#endif

namespace AlibabaNlsCommon {

std::string GenerateUuid() {
#ifdef _MSC_VER
  char *data;
  UUID uuidhandle;
  UuidCreate(&uuidhandle);
  UuidToString(&uuidhandle, (RPC_CSTR*)&data);
  std::string uuid(data);
  RpcStringFree((RPC_CSTR*)&data);
  return uuid;
#else
  uuid_t uu;
  uuid_generate(uu);
  char buf[36];
  uuid_unparse(uu, buf);
  return buf;
#endif
}

std::string UrlEncode(const std::string & src) {
  CURL *curl = curl_easy_init();	
  char *output = curl_easy_escape(curl, src.c_str(), src.size());
  std::string result(output);
  curl_free(output);
  curl_easy_cleanup(curl);
  return result;
}

std::string UrlDecode(const std::string & src) {
  CURL *curl = curl_easy_init();
  int outlength = 0;
  char *output = curl_easy_unescape(curl, src.c_str(), src.size(), &outlength);
  std::string result(output, outlength);
  curl_free(output);
  curl_easy_cleanup(curl);
  return result;
}

std::string ComputeContentMD5(const char * data, size_t size) {
#ifdef _MSC_VER
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  BYTE pbHash[16];
  DWORD dwDataLen = 16;

  CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
  CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash);
  CryptHashData(hHash, (BYTE*)(data), size, 0);
  CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &dwDataLen, 0);

  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);

  DWORD dlen = 0;
  CryptBinaryToString(pbHash, dwDataLen,
      CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &dlen);
  char* dest = new char[dlen];
  CryptBinaryToString(pbHash, dwDataLen,
      CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, dest, &dlen);

  std::string ret = std::string(dest, dlen);
  delete dest;
  return ret;
#else
  unsigned char md[MD5_DIGEST_LENGTH] = {0};
  MD5(reinterpret_cast<const unsigned char*>(data), size, (unsigned char*)&md);

  char encodedData[100] = {0};
  EVP_EncodeBlock(reinterpret_cast<unsigned char*>(encodedData),
                  md, MD5_DIGEST_LENGTH);
  return encodedData;
#endif
}

void StringReplace(std::string & src,
                   const std::string & s1,
                   const std::string & s2) {
  std::string::size_type pos =0;
  while ((pos = src.find(s1, pos)) != std::string::npos) {
    src.replace(pos, s1.length(), s2);
    pos += s2.length(); 
  }
}

std::string HttpMethodToString(HttpRequest::Method method) {
  switch (method) {
    case HttpRequest::Head:
      return "HEAD";
      break;
    case HttpRequest::Post:
      return "POST";
      break;
    case HttpRequest::Put:
      return "PUT";
      break;
    case HttpRequest::Delete:
      return "DELETE";
      break;
    case HttpRequest::Connect:
      return "CONNECT";
      break;
    case HttpRequest::Options:
      return "OPTIONS";
      break;
    case HttpRequest::Patch:
      return "PATCH";
      break;
    case HttpRequest::Trace:
      return "TRACE";
      break;
    case HttpRequest::Get:
    default:
      return "GET";
      break;
  }
}

}
