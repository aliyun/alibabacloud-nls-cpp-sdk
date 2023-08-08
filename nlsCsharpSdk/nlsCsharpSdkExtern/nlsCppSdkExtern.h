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

#ifndef _NLSCPPSDK_EXTERN_H_
#define _NLSCPPSDK_EXTERN_H_

NLSAPI(const char*) NlsGetVersion() {
  const char* version = AlibabaNls::NlsClient::getInstance(true)->getVersion();
  return version;
}

NLSAPI(int)
NlsSetLogConfig(const char* logFileName, int logLevel, int logFileSize,
                int logFileNum) {
  return AlibabaNls::NlsClient::getInstance()->setLogConfig(
      logFileName, (AlibabaNls::LogLevel)logLevel, logFileSize, logFileNum);
}

NLSAPI(void) NlsSetAddrInFamily(const char* aiFamily) {
  return AlibabaNls::NlsClient::getInstance()->setAddrInFamily(aiFamily);
}

NLSAPI(void) NlsSetDirectHost(const char* ip) {
  return AlibabaNls::NlsClient::getInstance()->setDirectHost(ip);
}

NLSAPI(int) NlsCalculateUtf8Chars(const char* text) {
  return AlibabaNls::NlsClient::getInstance()->calculateUtf8Chars(text);
}

NLSAPI(void) NlsStartWorkThread(int threadsNumber) {
  if (stEvent == NULL) {
    stEvent = new NLS_EVENT_STRUCT();
  } else {
    CleanNlsEvent(stEvent);
  }

  if (srEvent == NULL) {
    srEvent = new NLS_EVENT_STRUCT();
  } else {
    CleanNlsEvent(srEvent);
  }

  if (syEvent == NULL) {
    syEvent = new NLS_EVENT_STRUCT();
  } else {
    CleanNlsEvent(syEvent);
  }

  return AlibabaNls::NlsClient::getInstance()->startWorkThread(threadsNumber);
}

NLSAPI(void) NlsReleaseInstance() {
  AlibabaNls::NlsClient::getInstance()->releaseInstance();

  if (stEvent) {
    delete stEvent;
    stEvent = NULL;
  }
  if (srEvent) {
    delete srEvent;
    srEvent = NULL;
  }
  if (syEvent) {
    delete syEvent;
    syEvent = NULL;
  }
  return;
}

NLSAPI(AlibabaNls::SpeechTranscriberRequest*) NlsCreateTranscriberRequest() {
  const char* sdkName = "csharp";
  return AlibabaNls::NlsClient::getInstance()->createTranscriberRequest(
      sdkName);
}

NLSAPI(void)
NlsReleaseTranscriberRequest(AlibabaNls::SpeechTranscriberRequest* request) {
  return AlibabaNls::NlsClient::getInstance()->releaseTranscriberRequest(
      request);
}

NLSAPI(AlibabaNls::SpeechRecognizerRequest*) NlsCreateRecognizerRequest() {
  const char* sdkName = "csharp";
  return AlibabaNls::NlsClient::getInstance()->createRecognizerRequest(sdkName);
}

NLSAPI(void)
NlsReleaseRecognizerRequest(AlibabaNls::SpeechRecognizerRequest* request) {
  return AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(
      request);
}

NLSAPI(AlibabaNls::SpeechSynthesizerRequest*)
NlsCreateSynthesizerRequest(int type) {
  const char* sdkName = "csharp";
  return AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest(
      (AlibabaNls::TtsVersion)type, sdkName);
}

NLSAPI(void)
NlsReleaseSynthesizerRequest(AlibabaNls::SpeechSynthesizerRequest* request) {
  return AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(
      request);
}

NLSAPI(AlibabaNlsCommon::FileTrans*) NlsCreateFileTransferRequest() {
  AlibabaNlsCommon::FileTrans* request = new AlibabaNlsCommon::FileTrans();
  return request;
}

NLSAPI(void)
NlsReleaseFileTransferRequest(AlibabaNlsCommon::FileTrans* request) {
  delete request;
  return;
}

NLSAPI(AlibabaNlsCommon::NlsToken*) NlsCreateNlsToken() {
  return new AlibabaNlsCommon::NlsToken();
}

NLSAPI(void) NlsReleaseNlsToken(AlibabaNlsCommon::NlsToken* token) {
  return delete token;
}

#endif  // _NLSCPPSDK_EXTERN_H_
