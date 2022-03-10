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

#ifndef NLS_SDK_NETWORK_H
#define NLS_SDK_NETWORK_H

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <pthread.h>
#endif
//#include <stdint.h>
#include "nlsEncoder.h"

namespace AlibabaNls {

class INlsRequest;
class WorkThread;

class NlsEventNetWork {
 public:
  NlsEventNetWork();
  virtual ~NlsEventNetWork();

  static NlsEventNetWork * _eventClient;

  static void DnsLogCb(int w, const char *m);
  static void initEventNetWork(int count, char *aiFamily, char *directIp);
  static void destroyEventNetWork();

  int start(INlsRequest *request);
  int sendAudio(INlsRequest *request, const uint8_t * data,
                size_t dataSize, ENCODER_TYPE type);
  int stop(INlsRequest *request, int type);
  int stControl(INlsRequest* request, const char* message);

 private:
  int selectThreadNumber();            //循环选择工作线程

  static WorkThread *_workThreadArray; //工作线程数组
  static size_t _workThreadsNumber;    //工作线程数量
  static size_t _currentCpuNumber;
  static int _addrInFamily;
  static char _directIp[64];

#if defined(_MSC_VER)
  static HANDLE _mtxThread;
#else
  static pthread_mutex_t _mtxThread;
#endif

};

}  // namespace AlibabaNls

#endif //NLS_SDK_NETWORK_H
