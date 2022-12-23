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

#ifdef _MSC_VER
#include <windows.h>
#include <process.h>
#else
#if defined(__ANDROID__) || defined(__linux__)
#include <unistd.h>
#ifndef __ANDRIOD__
#include <iconv.h>
#endif
#endif
#include <pthread.h>
#endif

#include <iostream>
#include <string.h>
#include "CommonClient.h"
#include "json/json.h"
#include "FileTrans.h"
#include "text_utils.h"
#include "nlog.h"

namespace AlibabaNlsCommon {

using namespace AlibabaNls;

FileTrans::FileTrans() {
  accessKeySecret_ = "";
  accessKeyId_ = "";
  appKey_ = "";
  stsToken_ = "";
  fileLink_ = "";
  regionId_ = "";
  endpointName_ = "";
  serverResourcePath_ = "";

  domain_ = "filetrans.cn-shanghai.aliyuncs.com";
  serverVersion_ = "2018-08-17";
  action_ = "SubmitTask";
  product_ = "nls-filetrans";

#if defined(_MSC_VER)
  outputFormat_ = "GBK";
#else
  outputFormat_ = "UTF-8";
#endif

  eventHandler_ = NULL;
  paramHandler_ = NULL;

  resultResponse_.statusCode = 0;
  resultResponse_.taskId = "";
  resultResponse_.event = TaskUnknown;
  resultResponse_.errorMsg = "";
  resultResponse_.result = "";
}

FileTrans::~FileTrans() {
}

int FileTrans::paramCheck() {
  if (accessKeySecret_.empty()) {
    resultResponse_.errorMsg = "AccessKeySecret is empty.";
    resultResponse_.event = TaskFailed;
    return -(InvalidAkSecret);
  }

  if (accessKeyId_.empty()) {
    resultResponse_.errorMsg = "AccessKeyId is empty.";
    resultResponse_.event = TaskFailed;
    return -(InvalidAkId);
  }

  if (appKey_.empty()) {
    resultResponse_.errorMsg = "AppKey is empty.";
    resultResponse_.event = TaskFailed;
    return -(InvalidAppKey);
  }

  if (fileLink_.empty()) {
    resultResponse_.errorMsg = "FileLink is empty.";
    resultResponse_.event = TaskFailed;
    return -(InvalidFileLink);
  }

  if (domain_.empty()) {
    resultResponse_.errorMsg = "Domain is empty.";
    resultResponse_.event = TaskFailed;
    return -(InvalidDomain);
  }

  if (serverVersion_.empty()) {
    resultResponse_.errorMsg = "ServerVersion is empty.";
    resultResponse_.event = TaskFailed;
    return -(InvalidServerVersion);
  }

  if (action_.empty()) {
    resultResponse_.errorMsg = "Action is empty.";
    resultResponse_.event = TaskFailed;
    return -(InvalidAction);
  }

  return Success;
}

#if defined(_MSC_VER)
unsigned __stdcall applyResultRequestThread(LPVOID arg) {
#else
void* applyResultRequestThread(void* arg) {
#endif
  FileTrans* async = (FileTrans*)arg;
  struct resultRequest param = async->getRequestParams();
  async->applyResultRequest(param);
  return NULL;
}

int FileTrans::applyResultRequest(struct resultRequest param) {
  int retCode = 0;
  std::string errorMsg = "";
  std::string tmpErrorMsg = "";
  CommonClient* client = (CommonClient*)param.client;
  std::string taskId = param.taskId;
  std::string domain = param.domain;
  std::string serverVersion = param.serverVersion;
  std::string result_str = "";

  CommonRequest resultRequest(CommonRequest::FileTransPattern);
  resultRequest.setDomain(domain);
  resultRequest.setVersion(serverVersion);
  resultRequest.setHttpMethod(HttpRequest::Get);
  resultRequest.setAction("GetTaskResult");
  resultRequest.setTaskId(taskId);

  resultResponse_.taskId = taskId;

  Json::Value::UInt statusCode;
  Json::Value resultJson;
  Json::Reader resultReader;
  std::string resultString;

  CommonClient::CommonResponseOutcome resultOutcome;

  do {
    resultOutcome = client->commonResponse(resultRequest);
    if (!resultOutcome.isSuccess()) {
      // 异常处理
      resultResponse_.errorMsg = resultOutcome.error().errorMessage();
      resultResponse_.event = TaskFailed;
      retCode = -(ClientRequestFaild);
      break;
    }

    resultJson.clear();
    resultString.clear();
    resultString = resultOutcome.result().payload();

    if (!resultReader.parse(resultString, resultJson)) {
      tmpErrorMsg = "Json any failed: ";
      tmpErrorMsg += resultString;
      resultResponse_.errorMsg = tmpErrorMsg;
      resultResponse_.event = TaskFailed;
      retCode = -(JsonParseFailed);
      break;
    }

    if (!resultJson["StatusCode"].isNull()) {
      statusCode = resultJson["StatusCode"].asUInt();
      if ((statusCode == 21050001) || (statusCode == 21050002)) {
#if defined(_MSC_VER)
        Sleep(100);
#else
        usleep(100 * 1000);
#endif
      } else if ((statusCode == 21050000) || (statusCode == 21050003)){
        resultResponse_.result = resultString;
        resultResponse_.statusCode = statusCode;
        resultResponse_.event = TaskCompleted;
        LOG_DEBUG("task id: %s, result: %s",
            taskId.c_str(), result_str.c_str());
        break;
      } else {
        resultResponse_.errorMsg = resultJson["StatusText"].asCString();
        resultResponse_.statusCode = statusCode;
        resultResponse_.event = TaskFailed;
        retCode = -(ErrorStatusCode);
        break;
      }
    } else {
      resultResponse_.errorMsg = resultString;
      resultResponse_.statusCode = statusCode;
      resultResponse_.event = TaskFailed;
      retCode = -(ErrorStatusCode);
      break;
    }
    LOG_DEBUG("task id: %s, statusCode: %d", taskId.c_str(), statusCode);
  } while((statusCode == 21050001) || (statusCode == 21050002));

  if (eventHandler_) {
    eventHandler_(this, paramHandler_);
  }

  delete client;

  return retCode;
}

int FileTrans::applyFileTrans(bool sync) {
  int retCode = Success;
  std::string tmpErrorMsg;
  Json::Value::UInt statusCode;
  Json::Value root;
  Json::Reader reader;
  Json::Value::iterator iter;
  Json::Value::Members members;
  Json::FastWriter writer;

  LOG_INFO("NLS(FT) Initialize with version %s", utility::TextUtils::GetVersion().c_str());
  LOG_INFO("NLS(FT) Git SHA %s", utility::TextUtils::GetGitCommitInfo());

  retCode = paramCheck();
  if (retCode < 0) {
    return retCode;
  }

  if (!customParam_.empty()) {
    if (!reader.parse(customParam_.c_str(), root)) {
      return -(JsonParseFailed);
    }

    if (!root.isObject()) {
      return -(JsonObjectError);
    }
  }
  root["app_key"] = appKey_.c_str();
  root["file_link"] = fileLink_.c_str();

  //ClientConfiguration默认区域id为hangzhou
  ClientConfiguration configuration("cn-shanghai");
  if (!regionId_.empty()) {
    configuration.setRegionId(regionId_);
  }

  CommonClient* client = new CommonClient(
      accessKeyId_, accessKeySecret_, stsToken_, configuration);

  CommonRequest taskRequest(CommonRequest::FileTransPattern);
  taskRequest.setDomain(domain_);
  taskRequest.setVersion(serverVersion_);
  taskRequest.setHttpMethod(HttpRequest::Post);
  taskRequest.setAction("SubmitTask");

  std::string taskContent = writer.write(root);
  LOG_DEBUG("taskContent: %s", taskContent.c_str());

  taskRequest.setTask(taskContent);

  CommonClient::CommonResponseOutcome outcome =
      client->commonResponse(taskRequest);
  if (!outcome.isSuccess()) {
    // 异常处理
    resultResponse_.event = TaskFailed;
    resultResponse_.errorMsg = outcome.error().errorMessage();
    delete client;
    return -(ClientRequestFaild);
  }

  Json::Value requestJson;
  Json::Reader requestReader;
  std::string requestString = outcome.result().payload();

  // 这里已经有taskId了
  LOG_DEBUG("Request: %s", requestString.c_str());

  if (!requestReader.parse(requestString, requestJson)) {
    tmpErrorMsg = "Json any failed: ";
    tmpErrorMsg += requestString;
    resultResponse_.errorMsg = tmpErrorMsg;
    delete client;
    return -(JsonParseFailed);
  }

  if (!requestJson["StatusCode"].isNull()) {
    statusCode = requestJson["StatusCode"].asUInt();
    if ((statusCode != 21050000) && (statusCode != 21050003)) {
      resultResponse_.event = TaskFailed;
      resultResponse_.statusCode = statusCode;
      resultResponse_.errorMsg = requestJson["StatusText"].asCString();
      delete client;
      return -(ErrorStatusCode);
    }
  } else {
    resultResponse_.errorMsg = "Json anly failed.";
    resultResponse_.event = TaskFailed;
    delete client;
    return -(JsonParseFailed);
  }

  std::string taskId = requestJson["TaskId"].asCString();

  requestParams_.client = (void *)client;
  requestParams_.taskId = taskId;
  requestParams_.domain = domain_;
  requestParams_.serverVersion = serverVersion_;
  requestParams_.response = &resultResponse_;

  if (sync) {
    retCode = applyResultRequest(requestParams_);

    /* client will be deleted in applyResultRequest */
  } else {
  #if defined(_MSC_VER)
    unsigned thread_id;
    HANDLE threadHandle = (HANDLE)_beginthreadex(NULL, 0, applyResultRequestThread, (LPVOID)this, 0, &thread_id);
    CloseHandle(threadHandle);
  #else
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, applyResultRequestThread, (void*)this);
    pthread_detach(thread_id);
  #endif
    retCode = Success;

    /* client will be deleted when TaskFailed or TaskCompleted */
  }

  return retCode;
}

int FileTrans::getEvent() {
  return (int)resultResponse_.event;
}

const char* FileTrans::getErrorMsg() {
  if (resultResponse_.errorMsg.length() > 2048) {
    std::string part_errorMsg(resultResponse_.errorMsg.c_str(), 2048);
    LOG_DEBUG("file transfer get part error:%s", part_errorMsg.c_str());
  } else {
    LOG_DEBUG("file transfer get error:%s", resultResponse_.errorMsg.c_str());
  }
  if ("GBK" == outputFormat_) {
    resultResponse_.errorMsg = utf8ToGbk(resultResponse_.errorMsg);
  }
  return resultResponse_.errorMsg.c_str();
}

const char* FileTrans::getResult() {
  if (resultResponse_.result.length() > 2048) {
    std::string part_result(resultResponse_.result.c_str(), 2048);
    LOG_DEBUG("file transfer get part result:%s", part_result.c_str());
  } else {
    LOG_DEBUG("file transfer get result:%s", resultResponse_.result.c_str());
  }
  if ("GBK" == outputFormat_) {
    resultResponse_.result = utf8ToGbk(resultResponse_.result);
  }
  return resultResponse_.result.c_str();
}

const char* FileTrans::getTaskId() {
  if (resultResponse_.taskId.length() > 0) {
    LOG_DEBUG("current request taskId:%s", resultResponse_.taskId.c_str());
  }
  return resultResponse_.taskId.c_str();
}

struct resultRequest FileTrans::getRequestParams() {
  return requestParams_;
}

void FileTrans::setKeySecret(const std::string & KeySecret) {
  accessKeySecret_ = KeySecret;
}

void FileTrans::setAccessKeyId(const std::string & accessKeyId) {
  accessKeyId_ = accessKeyId;
}

void FileTrans::setStsToken(const std::string & stsToken) {
  stsToken_ = stsToken;
}

void FileTrans::setDomain(const std::string & domain) {
  domain_ = domain;
}

void FileTrans::setServerVersion(const std::string & serverVersion) {
  serverVersion_ = serverVersion;
}

void FileTrans::setAppKey(const std::string & appKey) {
  appKey_ = appKey;
}

void FileTrans::setFileLinkUrl(const std::string & fileLinkUrl) {
  fileLink_ = fileLinkUrl;
}

void FileTrans::setRegionId(const std::string & regionId) {
  regionId_ = regionId;
}

void FileTrans::setAction(const std::string & action) {
  action_ = action;
}

void FileTrans::setCustomParam(const std::string & customJsonString) {
  customParam_ = customJsonString;
}

void FileTrans::setOutputFormat(const std::string & textFormat) {
  outputFormat_ = textFormat;
}

void FileTrans::setEventListener(
    FileTransCallbackMethod event, void* param) {
  eventHandler_ = event;
  paramHandler_ = param;
}

#if defined(__ANDROID__) || defined(__linux__)
int FileTrans::codeConvert(char *from_charset, char *to_charset,
                           char *inbuf, size_t inlen,
                           char *outbuf, size_t outlen) {
#if defined(__ANDRIOD__)
  outbuf = inbuf;
#else
  iconv_t cd;
  char **pin = &inbuf;
  char **pout = &outbuf;

  cd = iconv_open(to_charset, from_charset);
  if (cd == 0) {
    return -(IconvOpenFailed);
  }

  memset(outbuf, 0, outlen);
  if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t)-1) {
    return -(IconvFailed);
  }

  iconv_close(cd);
#endif
  return Success;
}
#endif

std::string FileTrans::utf8ToGbk(const std::string & strUTF8) {
#if defined(__ANDROID__) || defined(__linux__)
  const char *msg = strUTF8.c_str();
  size_t inputLen = strUTF8.length();
  size_t outputLen = inputLen * 20;

  char *outbuf = new char[outputLen + 1];
  memset(outbuf, 0x0, outputLen + 1);

  char *inbuf = new char[inputLen + 1];
  memset(inbuf, 0x0, inputLen + 1);
  strncpy(inbuf, msg, inputLen);

  int res = codeConvert(
      (char *)"UTF-8", (char *)"GBK", inbuf, inputLen, outbuf, outputLen);
  if (res < 0) {
    LOG_ERROR("ENCODE: convert to utf8 error :%d .", res);
    return NULL;
  }

  std::string strTemp(outbuf);

  delete [] outbuf;
  outbuf = NULL;
  delete [] inbuf;
  inbuf = NULL;

  return strTemp;

#elif defined (_MSC_VER)

  int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
  unsigned short* wszGBK = new unsigned short[len + 1];
  memset(wszGBK, 0, len * 2 + 2);

  MultiByteToWideChar(CP_UTF8, 0,
                      (char*)strUTF8.c_str(), -1,
                      (wchar_t*)wszGBK, len);

  len = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszGBK, -1,
                            NULL, 0, NULL, NULL);

  char* szGBK = new char[len + 1];
  memset(szGBK, 0, len + 1);
  WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszGBK, -1, szGBK, len, NULL, NULL);

  std::string strTemp(szGBK);
  delete[] szGBK;
  delete[] wszGBK;

  return strTemp;

#else

  return strUTF8;

#endif
}

}
