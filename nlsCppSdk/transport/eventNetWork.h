/*
 * Copyright 2015 Alibaba Group Holding Limited
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

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

//#include <stdint.h>
#include "event2/util.h"

namespace AlibabaNls {

//enum StopType {
//	StopNormal = 0,
//	StopWwv,
//	StopCancel
//};

class INlsRequest;
class WorkThread;

class NlsEventClientNetWork {
public:
    NlsEventClientNetWork();
    virtual ~NlsEventClientNetWork();

    static NlsEventClientNetWork * _eventClient;

	static void DnsLogCb(int w, const char *m);
    static void initEventNetWork(int count);
    static void destroyEventNetWork();

	int start(INlsRequest *request);
	int sendAudio(INlsRequest *request, const uint8_t * data, size_t dataSize, bool encoded);
	int stop(INlsRequest *request, int type);
	int stControl(INlsRequest* request, const char* message);

private:
    int selectThreadNumber();  //循环选择工作线程

    static WorkThread *_workThreadArray; //工作线程数组
    static size_t _workThreadsNumber; //工作线程数量
    static size_t _currentCpuNumber;
#if defined(_WIN32)
    static HANDLE _mtxThreadNumber;
#else
    static pthread_mutex_t  _mtxThreadNumber;
#endif

};

}

#endif //NLS_SDK_NETWORK_H
