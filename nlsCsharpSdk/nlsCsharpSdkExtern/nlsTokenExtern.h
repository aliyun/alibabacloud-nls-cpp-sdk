/*
 * Copyright 2021 Alibaba Group Holding Limited
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

#ifndef _NLSCPPSDK_TOKEN_EXTERN_H_
#define _NLSCPPSDK_TOKEN_EXTERN_H_

static char* WCharToByte(const char* in, int* outLen) {
  int len = MultiByteToWideChar(CP_ACP, 0, in, -1, NULL, 0);
  wchar_t* wstr = new wchar_t[len + 1];
  memset(wstr, 0, len + 1);
  MultiByteToWideChar(CP_ACP, 0, in, -1, wstr, len);
  len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  char* str = new char[len + 1];
  memset(str, 0, len + 1);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
  if (wstr) delete[] wstr;

  *outLen = len;
  return str;
}

NLSAPI(int) NlsApplyNlsToken(AlibabaNlsCommon::NlsToken* token) {
  return token->applyNlsToken();
}

NLSAPI(const char*) NlsGetToken(AlibabaNlsCommon::NlsToken* token) {
  return token->getToken();
}

NLSAPI(const char*) NlsGetErrorMsg(AlibabaNlsCommon::NlsToken* token) {
  return token->getErrorMsg();
}

NLSAPI(uint64_t) NlsGetExpireTime(AlibabaNlsCommon::NlsToken* token) {
  return token->getExpireTime();
}

NLSAPI(void)
NlsSetAccessKeyId(AlibabaNlsCommon::NlsToken* token, const char* accessKeyId) {
  int len = -1;
  if (accessKeyId) {
    char* str = WCharToByte(accessKeyId, &len);
    if (str) {
      std::string akId(str);
      delete[] str;
      token->setAccessKeyId(akId);
    }
  }
  return;
}

NLSAPI(void)
NlsSetKeySecret(AlibabaNlsCommon::NlsToken* token, const char* KeySecret) {
  int len = -1;
  if (KeySecret) {
    char* str = WCharToByte(KeySecret, &len);
    if (str) {
      std::string akSecret(str);
      delete[] str;
      token->setKeySecret(akSecret);
    }
  }
  return;
}

#endif  // _NLSCPPSDK_TOKEN_EXTERN_H_
