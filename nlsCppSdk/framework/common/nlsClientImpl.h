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

#ifndef NLS_SDK_CLIENT_IMPL_H
#define NLS_SDK_CLIENT_IMPL_H

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include <string>

#include "nlsClient.h"
#include "nlsGlobal.h"
#include "nodeManager.h"

namespace AlibabaNls {

class INlsRequest;
class NlsClientImpl {
 public:
  NlsClientImpl(bool sslInitial);
  ~NlsClientImpl();

  int calculateUtf8CharsImpl(const char* value);
  void startWorkThreadImpl(int threadsNumber = 1);
  void releaseInstanceImpl();
  int setLogConfigImpl(const char* logOutputFile, const LogLevel logLevel,
                       unsigned int logFileSize = 10,
                       unsigned int logFileNum = 10,
                       LogCallbackMethod logCallback = NULL);
  void setAddrInFamilyImpl(const char* aiFamily = "AF_INET");
  void setDirectHostImpl(const char* ip);
  void setUseSysGetAddrInfoImpl(bool enable);
  void setSyncCallTimeoutImpl(unsigned int timeout_ms);

  SpeechRecognizerRequest* createRecognizerRequestImpl(
      const char* sdkName = "cpp", bool isLongConnection = false);
  void releaseRecognizerRequestImpl(SpeechRecognizerRequest* request);
  SpeechTranscriberRequest* createTranscriberRequestImpl(
      const char* sdkName = "cpp", bool isLongConnection = false);
  void releaseTranscriberRequestImpl(SpeechTranscriberRequest* request);
  SpeechSynthesizerRequest* createSynthesizerRequestImpl(
      TtsVersion version = ShortTts, const char* sdkName = "cpp",
      bool isLongConnection = false);
  void releaseSynthesizerRequestImpl(SpeechSynthesizerRequest* request);
  DialogAssistantRequest* createDialogAssistantRequestImpl(
      DaVersion version = DaV1, const char* sdkName = "cpp",
      bool isLongConnection = false);
  void releaseDialogAssistantRequestImpl(DialogAssistantRequest* request);
  FlowingSynthesizerRequest* createFlowingSynthesizerRequestImpl(
      const char* sdkName = "cpp", bool isLongConnection = false);
  void releaseFlowingSynthesizerRequestImpl(FlowingSynthesizerRequest* request);

#if defined(__linux__)
  int vipServerListGetUrlImpl(const std::string& vipServerDomainList,
                              const std::string& targetDomain,
                              std::string& url);
#endif

  NlsNodeManager* getNodeManger();

#if defined(_MSC_VER)
  HANDLE _mtxReleaseRequestGuard;
#else
  pthread_mutex_t _mtxReleaseRequestGuard;
#endif

 private:
  enum NlsClientConstValue {
    VipServerPort = 80,
  };

  void releaseRequest(INlsRequest*);

  static bool _isInitializeSSL;
  static bool _isInitializeThread;
  char _aiFamily[16];
  char _directHostIp[64];
  bool _enableSysGetAddr;
  unsigned int _syncCallTimeoutMs;

#if defined(__linux__)
#ifdef ENABLE_VIPSERVER
  static bool _isInitalizeVsClient;
#endif
  int vipServerGetIp(const std::string& vipServerDomain,
                     const int vipServerPort, const std::string& targetDomain,
                     std::string& nlsServerUrl);
#endif

  NlsNodeManager* _nodeManager;
};  // class NLS_SDK_CLIENT_EXPORT NlsClientImpl

}  // namespace AlibabaNls

#endif  // NLS_SDK_CLIENT_IMPL_H
