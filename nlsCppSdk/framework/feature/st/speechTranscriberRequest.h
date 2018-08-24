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

#ifndef NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_H
#define NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_H

#if defined(_WIN32)
#ifndef  ASR_API
#define ASR_API _declspec(dllexport)
#endif
#else
#define ASR_API
#endif

#include <map>
#include "nlsEvent.h"

class SpeechTranscriberParam;
class SpeechTranscriberSession;
class SpeechTranscriberListener;

class ASR_API SpeechTranscriberCallback {
public:

/**
    * @brief 构造函数
    */
SpeechTranscriberCallback();

/**
    * @brief 析构函数
    */
~SpeechTranscriberCallback();

/**
    * @brief 设置错误回调函数
    * @note 在请求过程中出现异常错误时，sdk内部线程上报该回调。用户可以在事件的消息头中检查状态码和状态消息，以确认失败的具体原因.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置实时音频流识别开始回调函数
    * @note 服务端就绪、可以开始识别时，sdk内部线程上报该回调.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTranscriptionStarted(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置一句话开始回调
    * @note 检测到一句话的开始时, sdk内部线程上报该回调.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnSentenceBegin(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置实时音频流识别中间结果回调函数
    * @note 设置enable_intermediate_result字段为true，才会有中间结果.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTranscriptionResultChanged(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置一句话结束回调函数
    * @note 检测到了一句话的结束时, sdk内部线程上报该回调.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnSentenceEnd(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置服务端结束服务回调函数
    * @note 云端结束实时音频流识别服务时, sdk内部线程上报该回调.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTranscriptionCompleted(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置通道关闭回调函数
    * @note 在请求过程中通道关闭时，sdk内部线程上报该回调.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);

NlsCallbackMethod _onTaskFailed;
NlsCallbackMethod _onTranscriptionStarted;
NlsCallbackMethod _onSentenceBegin;
NlsCallbackMethod _onTranscriptionResultChanged;
NlsCallbackMethod _onSentenceEnd;
NlsCallbackMethod _onTranscriptionCompleted;
NlsCallbackMethod _onChannelClosed;
std::map < NlsEvent::EventType, void*> _paramap;

};

class ASR_API SpeechTranscriberRequest {
public:

/**
    * @brief 构造函数
    * @param cb	事件回调接口
    * @param configPath 配置文件
    */
SpeechTranscriberRequest(SpeechTranscriberCallback* cb, const char* configPath);

/**
    * @brief 析构函数
    */
~SpeechTranscriberRequest();

/**
    * @brief 设置实时音频流识别服务URL地址
    * @param value 服务url字符串
    * @return 成功则返回0，否则返回-1
    */
int setUrl(const char* value);

/**
    * @brief 设置appKey
    * @note 请参照官网
    * @param value appKey字符串
    * @return 成功则返回0，否则返回-1
    */
int setAppKey(const char* value);

/**
    * @brief 口令认证。所有的请求都必须通过SetToken方法认证通过，才可以使用
    * @note token需要申请获取
    * @param value	申请的token字符串
    * @return 成功则返回0，否则返回-1
    */
int setToken(const char* value);

/**
    * @brief 设置音频数据编码格式
    * @note  可选参数，目前支持pcm, opu. 默认是pcm
    * @param value	音频数据编码字符串
    * @return 成功则返回0，否则返回-1
    */
int setFormat(const char* value);

/**
    * @brief 设置音频数据采样率sample_rate
    * @note 可选参数，目前支持16000, 8000. 默认是1600
    * @param value
    * @return 成功则返回0，否则返回-1
    */
int setSampleRate(int value);

/**
    * @brief 设置是否返回中间识别结果
    * @note 可选参数. 默认false
    * @param value "true" "false" bool字符串
    * @return 成功则返回0，否则返回-1
    */
int setIntermediateResult(const char* value);

/**
    * @brief 设置是否在后处理中添加标点
    * @note 可选参数. 默认false
    * @param value "true" "false" bool字符串
    * @return 成功则返回0，否则返回-1
    */
int setPunctuationPrediction(const char* value);

/**
    * @brief 设置是否在后处理中执行ITN
    * @note 可选参数. 默认false
    * @param value "true" "false" bool字符串
    * @return 成功则返回0，否则返回-1
    */
int setInverseTextNormalization(const char* value);

/**
* @brief 设置输出文本的编码格式
* @note
* @param value 编码格式 UTF-8 or GBK
* @return 成功则返回0，否则返回-1
*/
int setOutputFormat(const char* value);

/**
    * @brief 设置用户自定义参数
    * @param key 字段
    * @param value 参数
    * @return 成功则返回0，否则返回-1
    */
int setContextParam(const char* key, const char* value);

/**
    * @brief 启动实时音频流识别
    * @return 成功则返回0，否则返回-1
    */
int start();

/**
    * @brief 会与服务端确认关闭，正常停止实时音频流识别操作
    * @note 阻塞操作，等待服务端响应、或10秒超时才会返回
    * @return 成功则返回0，否则返回-1
    */
int stop();

/**
    * @brief 不会与服务端确认关闭，直接关闭实时音频流识别过程
    * @note 建议使用stop()结束请求访问.cancel()会中断访问请求流程，返回结果不可预知.
    * @return 成功则返回0，否则返回-1
    */
int cancel();

/**
    * @brief 发送语音数据
    * @note request对象format参数为pcm时, 使用false即可. format为opu, 使用压缩数据时, 需设置为true.
    * @param data	语音数据
    * @param dataSize	语音数据长度
    * @param encoded	是否启用压缩, 默认为false不启用数据压缩.
    * @return 成功则返回实际发送长度，失败返回-1
    */
int sendAudio(char* data, size_t dataSize, bool encoded = false);

private:
SpeechTranscriberListener* _listener;
SpeechTranscriberParam* _requestParam;
SpeechTranscriberSession* _session;
};

#endif //NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_H
