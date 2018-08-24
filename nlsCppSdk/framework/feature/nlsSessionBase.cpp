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

#include "iNlsRequestParam.h"
#include "nlsSessionBase.h"
#include "util/exception.h"
#include <string.h>
#include <sstream>

#if defined(__ANDROID__) || defined(__linux__)
#include <stdlib.h>
#include <errno.h>
#endif

using std::string;
using std::vector;
using namespace util;
using namespace transport;

#define STOP_RECV_TIMEOUT 15

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp){
	time_t clock;
	struct tm tm;
	SYSTEMTIME wtm;

	GetLocalTime(&wtm);
	tm.tm_year = wtm.wYear - 1900;
	tm.tm_mon = wtm.wMonth - 1;
	tm.tm_mday = wtm.wDay;
	tm.tm_hour = wtm.wHour;
	tm.tm_min = wtm.wMinute;
	tm.tm_sec = wtm.wSecond;
	tm.tm_isdst = -1;

	clock = mktime(&tm);

	tp->tv_sec = clock;
	tp->tv_usec = wtm.wMilliseconds * 1000;

	return 0;
}
#endif

NlsSessionBase::NlsSessionBase(INlsRequestParam* param) : _wsa(WebSocketTcp::connectTo(WebSocketAddress::urlConvert2WebSocketAddress(param->_url), param->_timeout, param->_token)),
                                                          _nlsRequestParam(param) {
	_status = RequestInitial;
    _handler = NULL;

    pthread_mutex_init(&_mtxClose, NULL);
	pthread_cond_init(&_cv, NULL);
	pthread_mutex_init(&_mtxNls, NULL);
	pthread_cond_init(&_cvNls, NULL);

    pthread_mutex_init(&_mtxStatus, NULL);

	switch (param->_mode) {
	case SR:
    case ST:
	case WWV:
	case SY:
    case TGA:
    case VPR:
    case VPM:
		_converter = new IWebSocketFrameResultConverter(param->_outputFormat);
		break;
	default:
		throw ExceptionWithString("not support mode", 10000020);
		_converter = NULL;
		break;
	}
}

NlsSessionBase::~NlsSessionBase() {

	delete _converter;
	_converter = NULL;

	pthread_mutex_destroy(&_mtxNls);
	pthread_mutex_destroy(&_mtxClose);
	pthread_cond_destroy(&_cv);
	pthread_cond_destroy(&_cvNls);

    pthread_mutex_destroy(&_mtxStatus);

}

int NlsSessionBase::start() {

    bool started = false;

    pthread_mutex_lock(&_mtxStatus);

    if (_status != RequestInitial) {
        pthread_mutex_unlock(&_mtxStatus);
        LOG_WARN("Invoke failed. Please check the order of execution.");
        return -1;
    }

    _status = RequestStarting;

    pthread_mutex_unlock(&_mtxStatus);

    string req = _nlsRequestParam->getStartCommand();
    int len = _wsa.sendText(req);
    if (len <= 0) {
        LOG_ERROR("StartCommand Send failed.");
        return -1;
    }
    LOG_INFO("StartCommand sent to server.");

    _wsa.setDataHandler(this);
    _wsa.start();

    struct timeval now;
    struct timespec outtime;
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 15;
    outtime.tv_nsec = now.tv_usec * 1000;

    pthread_mutex_lock(&_mtxNls);
    LOG_DEBUG("Begin Wait Signal.");
    if (ETIMEDOUT == pthread_cond_timedwait(&_cvNls, &_mtxNls, &outtime)) {
		LOG_ERROR("timeout of 15 seconds waiting for request started.");
        setStatus(RequestStopped);
        _wsa.cancle();
    }
	LOG_DEBUG("End Wait Signal.");
    pthread_mutex_unlock(&_mtxNls);

    if (!compareStatus(RequestStarted)) {
        LOG_ERROR("start is failed.");
        return -1;
    }

    return 0;
}

int NlsSessionBase::stop() {

    bool stopping = false;
    RequestStatus currentStatus;

    pthread_mutex_lock(&_mtxStatus);
    if (_status == RequestStarted) {

        _status = RequestStopping;
        stopping = true;
    } else {
        currentStatus = _status;
    }
    pthread_mutex_unlock(&_mtxStatus);

    if (stopping) {
        string req = _nlsRequestParam->getStopCommand();

        if (!req.empty()) {
            _wsa.setSocketTimeOut(STOP_RECV_TIMEOUT);
			int len = _wsa.sendText(req);
			if (len <= 0) {
				LOG_ERROR("StopCommand Send failed.");
			} else {
                LOG_DEBUG("StopCommand sent to server.");
            }
        }

        waitExit();

        return 0;
    } else {
        if (currentStatus == RequestStartFailed) {
            LOG_DEBUG("Invoke failed. start() is failed.");
        } else if (currentStatus == RequestStopped) {
            LOG_DEBUG("Invoke failed. The request is stopped.");
        } else {
            LOG_DEBUG("Invoke failed:%d. Please check the order of execution.", _status);
        }
        return -1;
    }
}

void NlsSessionBase::waitExit() {

    if (compareStatus(RequestStopping)) {
        pthread_mutex_lock(&_mtxClose);

        LOG_DEBUG("begin pthread_mutex_lock.");

        pthread_cond_wait(&_cv, &_mtxClose);

        LOG_DEBUG("end pthread_mutex_lock.");

        pthread_mutex_unlock(&_mtxClose);
    } else {
        LOG_DEBUG("The request's status is %d.", getStatus());
    }

}

int NlsSessionBase::shutdown() {

    bool canceling = false;
    RequestStatus currentStatus;

    pthread_mutex_lock(&_mtxStatus);
    if (_status == RequestStarted) {

        LOG_DEBUG("It's shutdown:%d.", _status);

        _status = RequestStopped;
        canceling = true;
    } else {
        currentStatus = _status;
    }
    pthread_mutex_unlock(&_mtxStatus);

    if (canceling) {
        _wsa.cancle();
    } else {
        if (currentStatus == RequestStartFailed) {
            LOG_DEBUG("Invoke failed. start() is failed.");
        } else if (currentStatus == RequestStopped) {
            LOG_DEBUG("Invoke failed. The request is stopped.");
        } else {
            LOG_DEBUG("Invoke failed:%d. Please check the order of execution.", _status);
        }

        return -1;
    }

	return 0;
}

void NlsSessionBase::byteArray2Short(uint8_t *data, int len, int16_t *result, bool isBigEndian) {
	if (!data || len <= 0 || !result) {
        return;
    }

	for (int i = 0; i < len; i += 2) {
		if (isBigEndian) {
            result[i / 2] = (int16_t) ((data[i] << 8 & 0xff00) | (data[i + 1] & 0xff));
        } else {
            result[i / 2] = (int16_t) ((data[i + 1] << 8 & 0xff00) | (data[i] & 0xff));
        }
	}
}

void NlsSessionBase::handlerFrame(WebsocketFrame frame) {
    NlsEvent* nlsevent = NULL;

    if (compareStatus(RequestStopped)) {
        return;
    }

    if (frame.data.size() < 1) {
		return;
	}

    LOG_INFO("Begin HandlerFrame status: %d", _status);

	if (frame.type == WsheaderType::CLOSE) {
		string msg(frame.data.begin(), frame.data.end());
		if (frame.closecode == -1) {
			nlsevent = new NlsEvent(msg, frame.closecode, NlsEvent::TaskFailed);
		} else {
            nlsevent = new NlsEvent(msg, frame.closecode, NlsEvent::Close);
        }
	} else {
		try {
			nlsevent = _converter->convertResult(frame);
		} catch (ExceptionWithString& e) {
			nlsevent = new NlsEvent(e.what(), e.getErrorcode(), NlsEvent::TaskFailed);
		}
	}

	if (nlsevent == NULL) {
        return;
	}

    LOG_DEBUG("HandlerFrame MsgType:%d.", nlsevent->getMsgType());

	if (this->_handler != NULL) {
		this->_handler->handlerFrame(*nlsevent);
	}

    bool sendStartSignal = false;
	pthread_mutex_lock(&_mtxStatus);
    if (_status == RequestStarting) {
        sendStartSignal = true;

		if (nlsevent->getMsgType() == NlsEvent::TaskFailed) {
			_status = RequestStartFailed;
		}
		else {
			_status = RequestStarted;
		}
    }
	pthread_mutex_unlock(&_mtxStatus);

	if (nlsevent->getMsgType() == NlsEvent::Close ||
        nlsevent->getMsgType() == NlsEvent::TaskFailed ||
        nlsevent->getMsgType() == NlsEvent::RecognitionCompleted ||
        nlsevent->getMsgType() == NlsEvent::TranscriptionCompleted ||
        nlsevent->getMsgType() == NlsEvent::SynthesisCompleted) {

        this->close();

        if (nlsevent->getMsgType() != NlsEvent::Close) {
            string closeMsg = "{\"channeclClosed\": \"nls request finished.\"}";
            NlsEvent* closeEvent = NULL;
            closeEvent = new NlsEvent(closeMsg, frame.closecode, NlsEvent::Close);
            if (this->_handler != NULL) {
                this->_handler->handlerFrame(*closeEvent);
            }

            delete closeEvent;
            closeEvent = NULL;
        }
	}

    if (sendStartSignal) {
        pthread_mutex_lock(&_mtxNls);
        pthread_cond_signal(&_cvNls);
        pthread_mutex_unlock(&_mtxNls);
    }

    delete nlsevent;
    nlsevent = NULL;

    LOG_INFO("End HandlerFrame statis: %d.", _status);
}

int NlsSessionBase::close() {

    bool canSignal = false;

    _wsa.close();

    pthread_mutex_lock(&_mtxStatus);
    if (RequestStopping == _status) {
        canSignal = true;
        LOG_DEBUG("need send signal.");
    }
    _status = RequestStopped;
    pthread_mutex_unlock(&_mtxStatus);

    if (canSignal) {
        pthread_mutex_lock(&_mtxClose);
        pthread_cond_signal(&_cv);
        LOG_DEBUG("signal pthread_cond_signal.");
        pthread_mutex_unlock(&_mtxClose);
    }

    return 0;
}

int NlsSessionBase::sendOpusVoice(const unsigned char* buffer, size_t bufferSize) {
	int16_t wave[::WAVE_FRAM_SIZE];
	uint8_t spx[640] = { 0 };

    if (compareStatus(RequestStarted)) {
        byteArray2Short((uint8_t *) buffer, WAVE_FRAM_SIZE * 2, wave, false);

        int nSize = codec.bufferFrame(wave, 0, WAVE_FRAM_SIZE, spx);

        return sendPcmVoice(spx, nSize);
    } else {
        LOG_WARN("Invoke failed:%d. Please check the order of execution.", _status);

        return -1;
    }
}

int NlsSessionBase::sendPcmVoice(const unsigned char* buffer, size_t num) {

    if (compareStatus(RequestStarted)) {
        return _wsa.sendBinary(buffer, num);
    } else {
        LOG_WARN("Invoke failed:%d. Please check the order of execution.", _status);

        return -1;
    }

}

void NlsSessionBase::setHandler(HandleBaseOneParamWithReturnVoid<NlsEvent>* ptr) {
	this->_handler = ptr;
}

int NlsSessionBase::stopWakeWordVerification() {

	return 0;
}

bool NlsSessionBase::compareStatus(RequestStatus status) {

    bool retValue = false;
    pthread_mutex_lock(&_mtxStatus);
    if (status == _status) {
        retValue = true;
    }
    pthread_mutex_unlock(&_mtxStatus);

    return retValue;
}

RequestStatus NlsSessionBase::getStatus() {
    RequestStatus retValue;
    pthread_mutex_lock(&_mtxStatus);
    retValue = _status;
    pthread_mutex_unlock(&_mtxStatus);

    return retValue;
}

void NlsSessionBase::setStatus(RequestStatus status) {
    pthread_mutex_lock(&_mtxStatus);
    _status = status;
    pthread_mutex_unlock(&_mtxStatus);
}
