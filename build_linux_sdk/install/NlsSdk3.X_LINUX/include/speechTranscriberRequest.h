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
class INlsRequestListener;

class NLS_SDK_CLIENT_EXPORT SpeechTranscriberCallback {

public:

SpeechTranscriberCallback();
~SpeechTranscriberCallback();

void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);
void setOnTranscriptionStarted(NlsCallbackMethod _event, void* para = NULL);
void setOnSentenceBegin(NlsCallbackMethod _event, void* para = NULL);
void setOnTranscriptionResultChanged(NlsCallbackMethod _event, void* para = NULL);
void setOnSentenceEnd(NlsCallbackMethod _event, void* para = NULL);
void setOnTranscriptionCompleted(NlsCallbackMethod _event, void* para = NULL);
void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);
void setOnSentenceSemantics(NlsCallbackMethod _event, void* para);

NlsCallbackMethod _onSentenceSemantics;
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

SpeechTranscriberRequest();
~SpeechTranscriberRequest();

/**
    * @brief 设置实时音频流识别服务URL地址
    * @note 必填参数. 默认为公网服务URL地址.
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
    * @note 可选参数. 默认false. 如果使用语义断句, 则vad断句设置不会生效. 两者为互斥关系.
    * @param value true 或 false
    * @return 成功则返回0，否则返回-1
    */
int setSemanticSentenceDetection(bool value);

/**
    * @brief 设置vad阀值
    * @note 可选参数. 静音时长超过该阈值会被认为断句, 合法参数范围200～2000(ms), 默认值800ms.
    * 		vad断句与语义断句为互斥关系, 不能同时使用. 调用此设置前, 请将语义断句setSemanticSentenceDetection设置为false.
    * @param value vad阀值
    * @return 成功则返回0，否则返回-1
    */
int setMaxSentenceSilence(int value);

/**
    * @brief 设置定制模型
    * @param value	定制模型id字符串
    * @return 成功则返回0，否则返回-1
    */
int setCustomizationId(const char * value);

/**
    * @brief 设置泛热词
    * @param value	定制泛热词id字符串
    * @return 成功则返回0，否则返回-1
    */
int setVocabularyId(const char * value);

/**
    * @brief 设置Socket接收超时时间
    * @note
    * @param value 超时时间
    * @return 成功则返回0，否则返回-1
    */
int setTimeout(int value);

/**
* @brief 设置是否开启nlp服务
* @param value 编码格式 UTF-8 or GBK
* @return 成功则返回0，否则返回-1
*/
int setEnableNlp(bool enable);

/**
* @brief 设置nlp模型名称，开启NLP服务后必填
* @param value 编码格式 UTF-8 or GBK
* @return 成功则返回0，否则返回-1
*/
int setNlpModel(const char* value);

/**
* @brief 设置session id
* @note  用于请求异常断开重连时，服务端识别是同一个会话。
* @param value session id 字符串
* @return 成功则返回0，否则返回-1
*/
int setSessionId(const char* value);

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
    * @brief 设置用户自定义ws阶段http header参数
    * @param key 参数名称
    * @param value 参数内容
    * @return 成功则返回0，否则返回-1
    */
int AppendHttpHeaderParam(const char* key, const char* value);

/**
    * @brief 启动实时音频流识别
    * @note 异步操作。成功返回started事件。失败返回TaskFailed事件。
    * @return 成功则返回0，否则返回-1
    */
int start();

/**
    * @brief 要求服务端更新识别参数
    * @note 异步操作。失败返回TaskFailed。
    * @return 成功则返回0，否则返回-1
    */
int control(const char* message);

/**
    * @brief 会与服务端确认关闭，正常停止实时音频流识别操作
    * @note 异步操作。失败返回TaskFailed。
    * @return 成功则返回0，否则返回-1
    */
int stop();

/**
    * @brief 直接关闭实时音频流识别过程
    * @note 调用cancel之后不会在上报任何回调事件
    * @return 成功则返回0，否则返回-1
    */
int cancel();

/**
    * @brief 发送语音数据
    * @note 异步操作。request对象format参数为为opu, encoded需设置为true. 其它格式默认为false. 异步操作
    * @param data	语音数据
    * @param dataSize	语音数据长度(建议每次100ms左右数据)
    * @param encoded	是否启用压缩, 默认为false不启用数据压缩.
    * @return 成功则返回实际发送长度，失败返回-1
    */
int sendAudio(const uint8_t * data, size_t dataSize, bool encoded = false);

/**
    * @brief 设置错误回调函数
    * @note 在请求过程中出现异常错误时，sdk内部线程上报该回调。用户可以在事件的消息头中检查状态码和状态消息，以确认失败的具体原因.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置实时音频流识别开始回调函数
    * @note 服务端就绪、可以开始识别时，sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTranscriptionStarted(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置一句话开始回调
    * @note 检测到一句话的开始时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnSentenceBegin(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置实时音频流识别中间结果回调函数
    * @note 设置enable_intermediate_result字段为true，才会有中间结果.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTranscriptionResultChanged(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置一句话结束回调函数
    * @note 检测到了一句话的结束时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnSentenceEnd(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置服务端结束服务回调函数
    * @note 云端结束实时音频流识别服务时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTranscriptionCompleted(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置通道关闭回调函数
    * @note 在请求过程中通道关闭时，sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置二次处理结果回调函数
    * @note 表示对实时转写的原始结果进行处理后的结果, 开启enable_nlp后返回
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnSentenceSemantics(NlsCallbackMethod _event, void* para = NULL);

///**
//    * @brief 获取request错误信息
//    * @return 错误信息字符串
//    */
//const char * getRequestErrorMsg();
//
///**
//    * @brief 获取request错误信息
//    * @return 错误信息代码
//    */
//int getRequestErrorStatus();

private:
SpeechTranscriberParam* _transcriberParam;
SpeechTranscriberCallback* _callback;
INlsRequestListener* _listener;

};

}

#if defined (_WIN32)
	#pragma warning( pop )
#endif

#endif //NLS_SDK_SPEECH_TRANSCRIBER_REQUEST_H
