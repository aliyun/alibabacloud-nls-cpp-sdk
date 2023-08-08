#pragma once
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

#ifndef _NLSCPPSDK_FILETRANSFER_EXTERN_H_
#define _NLSCPPSDK_FILETRANSFER_EXTERN_H_

NLSAPI(int) FTapplyFileTrans(AlibabaNlsCommon::FileTrans* request) {
  return request->applyFileTrans();
}

NLSAPI(const char*) FTgetErrorMsg(AlibabaNlsCommon::FileTrans* request) {
  return request->getErrorMsg();
}

NLSAPI(const char*) FTgetResult(AlibabaNlsCommon::FileTrans* request) {
  return request->getResult();
}

NLSAPI(void)
FTsetKeySecret(AlibabaNlsCommon::FileTrans* request, const char* KeySecret) {
  int len = -1;
  if (KeySecret) {
    char* str = WCharToByte(KeySecret, &len);
    if (str) {
      std::string keySecret(str);
      delete[] str;
      request->setKeySecret(keySecret);
    }
  }
  return;
}

NLSAPI(void)
FTsetAccessKeyId(AlibabaNlsCommon::FileTrans* request,
                 const char* accessKeyId) {
  int len = -1;
  if (accessKeyId) {
    char* str = WCharToByte(accessKeyId, &len);
    if (str) {
      std::string akId(str);
      delete[] str;
      request->setAccessKeyId(akId);
    }
  }
  return;
}

NLSAPI(void)
FTsetAppKey(AlibabaNlsCommon::FileTrans* request, const char* appKey) {
  int len = -1;
  if (appKey) {
    char* str = WCharToByte(appKey, &len);
    if (str) {
      std::string key(str);
      delete[] str;
      request->setAppKey(key);
    }
  }
  return;
}

NLSAPI(void)
FTsetFileLinkUrl(AlibabaNlsCommon::FileTrans* request,
                 const char* fileLinkUrl) {
  int len = -1;
  if (fileLinkUrl) {
    char* str = WCharToByte(fileLinkUrl, &len);
    if (str) {
      std::string url(str);
      delete[] str;
      request->setFileLinkUrl(url);
    }
  }
  return;
}

NLSAPI(void)
FTsetRegionId(AlibabaNlsCommon::FileTrans* request, const char* regionId) {
  int len = -1;
  if (regionId) {
    char* str = WCharToByte(regionId, &len);
    if (str) {
      std::string id(str);
      delete[] str;
      request->setRegionId(id);
    }
  }
  return;
}

NLSAPI(void)
FTsetAction(AlibabaNlsCommon::FileTrans* request, const char* action) {
  int len = -1;
  if (action) {
    char* str = WCharToByte(action, &len);
    if (str) {
      std::string act(str);
      delete[] str;
      request->setAction(act);
    }
  }
  return;
}

NLSAPI(void)
FTsetDomain(AlibabaNlsCommon::FileTrans* request, const char* domain) {
  int len = -1;
  if (domain) {
    char* str = WCharToByte(domain, &len);
    if (str) {
      std::string id(str);
      delete[] str;
      request->setDomain(id);
    }
  }
  return;
}

NLSAPI(void)
FTsetServerVersion(AlibabaNlsCommon::FileTrans* request, const char* version) {
  int len = -1;
  if (version) {
    char* str = WCharToByte(version, &len);
    if (str) {
      std::string id(str);
      delete[] str;
      request->setServerVersion(id);
    }
  }
  return;
}

NLSAPI(void)
FTsetCustomParam(AlibabaNlsCommon::FileTrans* request, const char* jsonString) {
  int len = -1;
  if (jsonString) {
    char* str = WCharToByte(jsonString, &len);
    if (str) {
      std::string param(str);
      delete[] str;
      request->setCustomParam(param);
    }
  }
  return;
}

NLSAPI(void)
FTsetOutputFormat(AlibabaNlsCommon::FileTrans* request,
                  const char* textFormat) {
  int len = -1;
  if (textFormat) {
    char* str = WCharToByte(textFormat, &len);
    if (str) {
      std::string param(str);
      delete[] str;
      request->setOutputFormat(param);
    }
  }
  return;
}

#endif  // _NLSCPPSDK_FILETRANSFER_EXTERN_H_
