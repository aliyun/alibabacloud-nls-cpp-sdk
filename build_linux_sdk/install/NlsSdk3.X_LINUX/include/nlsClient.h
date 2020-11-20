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
#ifndef NLS_SDK_CLIENT_H
#define NLS_SDK_CLIENT_H

#include "nlsGlobal.h"

#if defined(_WIN32)
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <string>

namespace AlibabaNls {
class INlsRequest;
class SpeechRecognizerCallback;
class SpeechRecognizerRequest;
class SpeechRecognizerSyncRequest;
class SpeechTranscriberCallback;
class SpeechTranscriberRequest;
class SpeechTranscriberSyncRequest;
class SpeechSynthesizerCallback;
class SpeechSynthesizerRequest;
class DialogAssistantCallback;
class DialogAssistantRequest;

class NlsEventClientNetWork;

enum LogLevel {
    LogError = 1,
    LogWarning,
    LogInfo,
    LogDebug
};

enum TtsVersion {
    ShortTts = 0,
    LongTts
};

enum DaVersion {
    DaV1 = 0,
    DaV2
};

class NLS_SDK_CLIENT_EXPORT NlsClient {

public:

/**
    * @brief 设置日志文件与存储路径
    * @param logOutputFile	日志文件
    * @param logLevel	日志级别，默认1（LogError : 1, LogWarning : 2, LogInfo : 3, LogDebug : 4）
    * @param logFileSize 日志文件的大小，以MB为单位，默认为10MB；
    *                    如果日志文件内容的大小超过这个值，SDK会自动备份当前的日志文件，最多可备份5个文件，超过后会循环覆盖已有文件
    * @return 成功则返回0，失败返回-1
    */
int setLogConfig(const char* logOutputFile, const LogLevel logLevel, unsigned int logFileSize = 10);

/**
    * @brief 创建一句话识别对象
    * @param onResultReceivedEvent	事件回调接口
    * @return 成功返回speechRecognizerRequest对象，否则返回NULL
    */
SpeechRecognizerRequest* createRecognizerRequest();

/**
    * @brief 销毁一句话识别对象
    * @param request  createRecognizerRequest所建立的request对象
    * @return
    */
void releaseRecognizerRequest(SpeechRecognizerRequest* request);

/**
    * @brief 创建一句话同步识别对象
    * @return 成功返回SpeechRecognizerSyncRequest对象，否则返回NULL
    */
//SpeechRecognizerSyncRequest* createRecognizerSyncRequest();

/**
    * @brief 销毁一句话同步识别对象
    * @param request  createRecognizerSyncRequest所建立的request对象
    * @return
    */
//void releaseRecognizerSyncRequest(SpeechRecognizerSyncRequest* request);

/**
    * @brief 创建实时音频流识别对象
    * @param onResultReceivedEvent	事件回调接口
    * @return 成功返回SpeechTranscriberRequest对象，否则返回NULL
    */
SpeechTranscriberRequest* createTranscriberRequest();

/**
    * @brief 销毁实时音频流识别对象
    * @param request  createTranscriberRequest所建立的request对象
    * @return
    */
void releaseTranscriberRequest(SpeechTranscriberRequest* request);

/**
    * @brief 创建实时音频流同步识别对象
    * @return 成功返回SpeechTranscriberSyncRequest对象，否则返回NULL
    */
//SpeechTranscriberSyncRequest* createTranscriberSyncRequest();

/**
    * @brief 销毁实时音频流同步识别对象
    * @param request  createTranscriberSyncRequest所建立的request对象
    * @return
    */
//void releaseTranscriberSyncRequest(SpeechTranscriberSyncRequest* request);

/**
    * @brief 创建语音合成对象
    * @param type tts类型
    * @return 成功则SpeechSynthesizerRequest对象，否则返回NULL
    */
SpeechSynthesizerRequest* createSynthesizerRequest(TtsVersion version = ShortTts);

/**
    * @brief 销毁语音合成对象
    * @param request  createSynthesizerRequest所建立的request对象
    * @return
    */
void releaseSynthesizerRequest(SpeechSynthesizerRequest* request);

/**
    * @brief 创建语音助手对象
    * @param onResultReceivedEvent	事件回调接口
    * @return 成功则DialogAssistantRequest对象，否则返回NULL
    */
DialogAssistantRequest* createDialogAssistantRequest(DaVersion version = DaV1);

/**
    * @brief 销毁语音助手对象
    * @param request  createDialogAssistantRequest所建立的request对象
    * @return
    */
void releaseDialogAssistantRequest(DialogAssistantRequest* request);

/**
    * @brief 当前版本信息
    * @return
    */
const char* getVersion();

/**
    * @brief 启动工作线程数量
    * @param threadsNumber 启动工作线程数量，默认设置值为1
    * @return
    */
void startWorkThread(int threadsNumber = 1);

/**
    * @brief NlsClient对象实例
    * @param sslInitial	是否初始化openssl 线程安全，默认为true
    * @return NlsClient对象
    */
static NlsClient* getInstance(bool sslInitial = true);

/**
    * @brief 销毁NlsClient对象实例
    * @note 进程退出时调用, 销毁NlsClient.
    * @return
    */
static void releaseInstance();

private:

NlsClient();
~NlsClient();

void releaseRequest(INlsRequest*);

#if defined(_WIN32)
static HANDLE _mtx;
#else
static pthread_mutex_t _mtx;
#endif
static bool _isInitializeSSL;
static bool _isInitializeThread;
static NlsClient* _instance;

};

}

#endif //NLS_SDK_CLIENT_H
