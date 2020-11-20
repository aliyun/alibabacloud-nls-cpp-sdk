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

#ifndef NLS_SDK_SPEECH_RECOGNIZER_REQUEST_H
#define NLS_SDK_SPEECH_RECOGNIZER_REQUEST_H

#include <queue>
#include "iNlsRequest.h"

#if defined(_WIN32)
	#pragma warning(push)
	#pragma warning(disable : 4251)
#endif

namespace AlibabaNls {
//class ConnectNode;
class SpeechRecognizerParam;
class INlsRequestListener;

class SpeechRecognizerCallback  {

public:
	SpeechRecognizerCallback();
	~SpeechRecognizerCallback();

	void setOnTaskFailed(NlsCallbackMethod event, void* param = NULL);
	void setOnRecognitionStarted(NlsCallbackMethod event, void* param = NULL);
	void setOnRecognitionCompleted(NlsCallbackMethod event, void* param = NULL);
	void setOnRecognitionResultChanged(NlsCallbackMethod event, void* param = NULL);
	void setOnChannelClosed(NlsCallbackMethod event, void* param = NULL);

	NlsCallbackMethod _onTaskFailed;
	NlsCallbackMethod _onRecognitionStarted;
	NlsCallbackMethod _onRecognitionCompleted;
	NlsCallbackMethod _onRecognitionResultChanged;
	NlsCallbackMethod _onChannelClosed;
	std::map < NlsEvent::EventType, void*> _paramap;
};

class NLS_SDK_CLIENT_EXPORT SpeechRecognizerRequest : public INlsRequest {

public:

SpeechRecognizerRequest();

virtual ~SpeechRecognizerRequest();

/**
    * @brief 设置一句话识别服务URL地址
    * @note 必填参数. 默认为公网服务URL地址.
    * @param value 服务url字符串
    * @return 成功则返回0，否则返回-1
    */
int setUrl(const char* value);

/**
    * @brief 设置appKey
    * @note 必填参数, 请参照官网申请
    * @param value appKey字符串
    * @return 成功则返回0，否则返回-1
    */
int setAppKey(const char* value);

/**
    * @brief 口令认证。所有的请求都必须通过SetToken方法认证通过，才可以使用
    * @note token需要申请获取, 必填参数
    * @param value	申请的token字符串
    * @return 成功则返回0，否则返回-1
    */
int setToken(const char* value);

/**
    * @brief 设置音频数据编码格式字段Format
    * @param value	可选参数, 目前支持pcm|opus|opu. 默认是pcm
    * @return 成功则返回0，否则返回-1
    */
int setFormat(const char* value);

/**
    * @brief 设置字段sample_rate
    * @param value 可选参数. 目前支持16000, 8000. 默认是16000
    * @return 成功则返回0，否则返回-1
    */
int setSampleRate(int value);

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
    * @brief 设置字段enable_intermediate_result设置
    * @param value	是否返回中间识别结果, 可选参数. 默认false
    * @return 成功则返回0，否则返回-1
    */
int setIntermediateResult(bool value);

/**
    * @brief 设置字段enable_punctuation_prediction
    * @param value	是否在后处理中添加标点, 可选参数. 默认false
    * @return 成功则返回0，否则返回-1
    */
int setPunctuationPrediction(bool value);

/**
    * @brief 设置字段enable_inverse_text_normalization
    * @param value	是否在后处理中执行ITN, 可选参数. 默认false
    * @return 成功则返回0，否则返回-1
    */
int setInverseTextNormalization(bool value);

/**
    * @brief 设置字段enable_voice_detection设置
    * @param value	是否启动自定义静音检测, 可选. 默认是False. 云端默认静音检测时间800ms.
    * @return 成功则返回0，否则返回-1
    */
int setEnableVoiceDetection(bool value);

/**
    * @brief 设置字段max_start_silence
    * @param value	允许的最大开始静音, 可选. 单位是毫秒. 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
    *       需要先设置enable_voice_detection为true. 建议时间2~5秒.
    * @return 成功则返回0，否则返回-1
    */
int setMaxStartSilence(int value);

/**
    * @brief 设置字段max_end_silence
    * @param value	允许的最大结束静音, 可选, 单位是毫秒. 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
    *       需要先设置enable_voice_detection为true. 建议时间0~5秒.
    * @return 成功则返回0，否则返回-1
    */
int setMaxEndSilence(int value);

/**
    * @brief 设置Socket接收超时时间
    * @param value 超时时间
    * @return 成功则返回0，否则返回-1
    */
int setTimeout(int value);

/**
    * @brief 设置输出文本的编码格式
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
    * @brief 启动SpeechRecognizerRequest
    * @note 异步操作。成功返回started事件。失败返回TaskFailed事件。
    * @return 成功则返回0，否则返回-1
    */
int start();

/**
    * @brief 会与服务端确认关闭，正常停止SpeechRecognizerRequest链接操作
    * @note 异步操作。失败返回TaskFailed。
    * @return 成功则返回0，否则返回-1
    */
int stop();

/**
    * @brief 不会与服务端确认关闭，直接关闭SpeechRecognizerRequest链接.
    * @note 调用cancel之后不会返回任何回调事件。
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
int sendAudio(const uint8_t * data, size_t dataSize, bool encoded = false);

/**
    * @brief 设置错误回调函数
    * @note 在请求过程中出现错误时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnTaskFailed(NlsCallbackMethod event, void* param = NULL);

/**
    * @brief 设置一句话识别开始回调函数
    * @note 在语音识别可以开始时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnRecognitionStarted(NlsCallbackMethod event, void* param = NULL);

/**
    * @brief 设置一句话识别结束回调函数
    * @note 在语音识别完成时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnRecognitionCompleted(NlsCallbackMethod event, void* param = NULL);

/**
    * @brief 设置一句话识别中间结果回调函数
    * @note 设置enable_intermediate_result字段为true, 才会有中间结果.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnRecognitionResultChanged(NlsCallbackMethod event, void* param = NULL);

/**
    * @brief 设置通道关闭回调函数
    * @note 在请求过程中通道关闭时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnChannelClosed(NlsCallbackMethod event, void* param = NULL);

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
SpeechRecognizerCallback* _callback;
SpeechRecognizerParam* _recognizerParam;
INlsRequestListener* _listener;

};

}

#if defined (_WIN32)
	#pragma warning(pop)
#endif

#endif //NLS_SDK_SPEECH_RECOGNIZER_REQUEST_H
