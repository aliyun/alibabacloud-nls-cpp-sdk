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

#include "nlsGlobal.h"
#include "iNlsRequest.h"

#if defined(_WIN32)
	#pragma warning( push )
	#pragma warning ( disable : 4251 )
#endif

namespace AlibabaNls {

class SpeechTranscriberParam;

class NLS_SDK_CLIENT_EXPORT SpeechTranscriberCallback {

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

class NLS_SDK_CLIENT_EXPORT SpeechTranscriberRequest : public INlsRequest {

public:

/**
    * @brief 构造函数
    * @param cb	事件回调接口
    */
SpeechTranscriberRequest(SpeechTranscriberCallback* cb);

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
    * @param value true 或 false
    * @return 成功则返回0，否则返回-1
    */
int setIntermediateResult(bool value);

/**
    * @brief 设置是否在后处理中添加标点
    * @note 可选参数. 默认false
    * @param value true 或 false
    * @return 成功则返回0，否则返回-1
    */
int setPunctuationPrediction(bool value);

/**
    * @brief 设置是否在后处理中执行数字转换
    * @note 可选参数. 默认false
    * @param value true 或 false
    * @return 成功则返回0，否则返回-1
    */
int setInverseTextNormalization(bool value);

/**
    * @brief 设置是否使用语义断句
    * @note 可选参数. 默认false
    * @param value true 或 false
    * @return 成功则返回0，否则返回-1
    */
int setSemanticSentenceDetection(bool value);

/**
    * @brief 设置vad阀值
    * @note 可选参数. 静音时长超过该阈值会被认为断句, 合法参数范围200～2000(ms), 默认值800ms.
    * @param value vad阀值
    * @return 成功则返回0，否则返回-1
    */
int setMaxSentenceSilence(int value);

/**
    * @brief 设置Socket接收超时时间
    * @note
    * @param value 超时时间
    * @return 成功则返回0，否则返回-1
    */
int setTimeout(int value);

/**
* @brief 设置输出文本的编码格式
* @note
* @param value 编码格式 UTF-8 or GBK
* @return 成功则返回0，否则返回-1
*/
int setOutputFormat(const char* value);

/**
    * @brief 参数设置
    * @note  暂不对外开放
    * @param value 参数
    * @return 成功则返回0，否则返回-1
    */
int setPayloadParam(const char* value);

/**
    * @brief 设置用户自定义参数
    * @param value 参数
    * @return 成功则返回0，否则返回-1
    */
int setContextParam(const char* value);

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
    * @note request对象format参数为为opu, encoded需设置为true. 其它格式默认为false.
    * @param data	语音数据
    * @param dataSize	语音数据长度(建议每次100ms左右数据)
    * @param encoded	是否启用压缩, 默认为false不启用数据压缩.
    * @return 成功则返回实际发送长度，失败返回-1
    */
int sendAudio(char* data, int dataSize, bool encoded = false);

/**
    * @brief SDK内部使用
    */
int getTranscriberResult(std::queue<NlsEvent>* eventQueue);

private:
SpeechTranscriberParam* _transcriberParam;
};

}

#if defined (_WIN32)
	#pragma warning( pop )
#endif

#endif //NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_H
