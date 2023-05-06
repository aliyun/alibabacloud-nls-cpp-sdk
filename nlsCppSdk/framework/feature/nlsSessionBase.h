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

#ifndef NLS_SDK_SESSION_BASE_H
#define NLS_SDK_SESSION_BASE_H

#include <string>

#if defined(_WIN32)
#include "pthread.h"
#else
#include <pthread.h>
#endif

#include "engine/webSocketAgent.h"
#include "webSocketFrameHandleBase.h"
#include "iWebSocketFrameResultConverter.h"

namespace AlibabaNls {

class NlsEvent;
class INlsRequestParam;

#ifdef _WIN32 
int gettimeofday(struct timeval *tp, void *tzp);
#endif

enum NlsStatus {
    Begin = 0,
    Started = 1,
};

enum RequestStatus {
    RequestInitial = 0,
    RequestStarting,
    RequestStartFailed,
    RequestStarted,
	RequestStopping,
    RequestStopped
};

class NlsSessionBase : public HandleBaseOneParamWithReturnVoid<util::WebsocketFrame> {
public:

	NlsSessionBase(INlsRequestParam* param);
	virtual ~NlsSessionBase();
	virtual int sendPcmVoice(const unsigned char* buffer, int num);
	virtual void setHandler(HandleBaseOneParamWithReturnVoid<NlsEvent>*);
	virtual int start();
	virtual int stop();
	virtual int close();
    virtual int shutdown();

    transport::engine::webSocketAgent _wsa;
    RequestStatus _status;
	INlsRequestParam* _nlsRequestParam;
	virtual void waitExit();

	pthread_mutex_t  _mtxClose;
	pthread_cond_t  _cv;
	pthread_mutex_t  _mtxNls;
	pthread_cond_t  _cvNls;

    static void byteArray2Short(uint8_t *data, int len, int16_t *result, bool isBigEndian);
	virtual void handlerFrame(util::WebsocketFrame frame);

	HandleBaseOneParamWithReturnVoid<NlsEvent>* _handler;
	IWebSocketFrameResultConverter* _converter;

	bool compareStatus(RequestStatus status);

private:

    void setStatus(RequestStatus status);
	RequestStatus getStatus();

    pthread_mutex_t  _mtxStatus;
};

}

#endif
