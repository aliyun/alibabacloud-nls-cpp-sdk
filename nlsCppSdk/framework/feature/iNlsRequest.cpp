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

#include <string>
#include "log.h"
#include "nlsSessionBase.h"
#include "iNlsRequestParam.h"
#include "iNlsRequestListener.h"
#include "nlsRequestParamInfo.h"
#include "iNlsRequest.h"

using std::string;

namespace AlibabaNls {

using namespace util;

INlsRequest::INlsRequest() {
    _isStarted = false;

    pthread_mutex_init(&_mtxStarted, NULL);
}

INlsRequest::~INlsRequest() {
    pthread_mutex_destroy(&_mtxStarted);
}

int INlsRequest::start() {
    int ret = -1;
    string errorInfo;
    int errorCode = 0;
    string taskId = _requestParam->_task_id;

    try {
        if (!_session) {
            _session = new NlsSessionBase(_requestParam);
            if (_session == NULL) {
                errorInfo = "new the SpeechRecognizerSession failed, please check if the memory is enough";
                _listener->handlerFrame(errorInfo, 10000010, NlsEvent::TaskFailed, taskId);
                return -1;
            }
            _session->setHandler(_listener);
        }
        ret = _session->start();
        if (0 == ret) {
            setStarted(true);
        }
        return ret;
    } catch (ExceptionWithString &e) {
        errorInfo = e.what();
        errorCode = e.getErrorcode();
        LOG_ERROR("start failed: %s.", e.what());
        ret = -1;
    }

    if (-1 == ret) {
        errorInfo += ", start finised.";
        _listener->handlerFrame(errorInfo, errorCode, NlsEvent::TaskFailed, taskId);
    }

    return ret;
}

int INlsRequest::stop() {

    if (!isStarted()) {
        string errorInfo = "Stop invoke error. Please check the order of execution or whether the data sent is valid.";
        _listener->handlerFrame(errorInfo, 10000011, NlsEvent::TaskFailed, _requestParam->_task_id);
        return -1;
    } else {
        if (_session) {
            if (_session->compareStatus(RequestStopped)) {
                setStarted(false);
                LOG_DEBUG("The Speech connect is stopped.");

                return -1;
            }
        }
    }

    int ret = _session->stop();
    if (0 == ret) {
        setStarted(false);
    }
    return ret;
}

int INlsRequest::cancel() {

    if (!isStarted()) {
        string errorInfo = "Stop invoke error. Please check the order of execution or whether the data sent is valid.";
        _listener->handlerFrame(errorInfo, 10000011, NlsEvent::TaskFailed, _requestParam->_task_id);
        return -1;
    } else {
        if (_session) {
            if (_session->compareStatus(RequestStopped)) {
                setStarted(false);
                LOG_DEBUG("The Speech connect is stopped.");
                return -1;
            }
        }
    }

    int ret = _session->shutdown();
    if (0 == ret) {
        setStarted(false);
    }
    return ret;
}

bool INlsRequest::isStarted() {
    bool value;
    pthread_mutex_lock(&_mtxStarted);
    value = _isStarted;
    pthread_mutex_unlock(&_mtxStarted);

    return value;
}

void INlsRequest::setStarted(bool value) {
    pthread_mutex_lock(&_mtxStarted);
    _isStarted = value;
    pthread_mutex_unlock(&_mtxStarted);
}

int INlsRequest::sendAudio(char* data, int dataSzie, bool encoded ) {
    int ret = -1;
    string errorInfo;
    string format = _requestParam->_payload[D_FORMAT].asCString();
    string taskId = _requestParam->_task_id;

    if (!isStarted()) {
        errorInfo = "SendAudio invoke error. Please check the order of execution or whether the sent data is valid.";
        _listener->handlerFrame(errorInfo, 10000011, NlsEvent::TaskFailed, taskId);

        return -1;
    } else {
        if (_session) {
            if (_session->compareStatus(RequestStopped)) {
                LOG_DEBUG("The Speech connect is stopped.");
                return -1;
            }
        }
    }

    if (!data || dataSzie <= 0) {
        errorInfo = "The sent data is null or dataSize <= 0.";
        _listener->handlerFrame(errorInfo, 10000013, NlsEvent::TaskFailed, taskId);

        return -1;
    }

    ret = _session->sendPcmVoice((unsigned char *)data, dataSzie);

    return ret ;
}

int INlsRequest::getRecognizerResult(std::queue<NlsEvent>* eventQueue) {
    return _listener->getRecognizerResult(eventQueue);
}

int INlsRequest::setContextParam(const char* value) {
    Json::Value root;
    Json::Reader reader;
    Json::Value::iterator iter;
    Json::Value::Members members;
    string tmpValue = value;

    if (!reader.parse(tmpValue, root)) {
        LOG_ERROR("parse json fail: %s", value);
        return -1;
    }

    if (!root.isObject()) {
        LOG_ERROR("value is n't a json object.");
        return -1;
    }

    string jsonKey;
    string jsonValue;
    members = root.getMemberNames();
    Json::Value::Members::iterator it = members.begin();
    for (; it != members.end(); ++it) {
        jsonKey = *it;
        LOG_DEBUG("json key:%s.", jsonKey.c_str());

        this->_requestParam->setContextParam(jsonKey.c_str(), root);
    }

    return 0;
}

int INlsRequest::setToken(const char*token) {
    if (!token) {
        LOG_ERROR("It's null token.");
        return -1;
    }

    return this->_requestParam->setToken(token);
}

int INlsRequest::setUrl(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Url.");
        return -1;
    }

    return this->_requestParam->setUrl(value);
}

int INlsRequest::setAppKey(const char* value) {
    if (!value) {
        LOG_ERROR("It's null AppKey.");
        return -1;
    }

    return this->_requestParam->setAppKey(value);
}

int INlsRequest::setFormat(const char* value) {
    if (!value) {
        LOG_ERROR("It's null Format.");
        return -1;
    }

    return this->_requestParam->setFormat(value);
}

int INlsRequest::setSampleRate(int value) {
    return this->_requestParam->setSampleRate(value);
}

int INlsRequest::setTimeout(int value) {
    return this->_requestParam->setTimeout(value);
}

int INlsRequest::setOutputFormat(const char* value) {
    if (!value) {
        LOG_ERROR("It's null OutputFormat.");
        return -1;
    }

    return this->_requestParam->setOutputFormat(value);
}

int INlsRequest::setPayloadParam(const char* value) {
    Json::Value root;
    Json::Reader reader;
    Json::Value::iterator iter;
    Json::Value::Members members;
    string tmpValue = value;

    if (!reader.parse(tmpValue, root)) {
        LOG_ERROR("parse json fail: %s", value);
        return -1;
    }

    if (!root.isObject()) {
        LOG_ERROR("value is n't a json object.");
        return -1;
    }

    string jsonKey;
    string jsonValue;
    members = root.getMemberNames();
    Json::Value::Members::iterator it = members.begin();
    for (; it != members.end(); ++it) {
        jsonKey = *it;
        LOG_DEBUG("json key:%s.", jsonKey.c_str());

        this->_requestParam->setPayloadParam(jsonKey.c_str(), root);
    }

    return 0;
}

}
