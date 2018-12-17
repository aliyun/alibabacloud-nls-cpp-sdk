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

#ifndef NLS_SDK_SPEECH_TRANSCRIBER_SYNC_REQUEST_H
#define NLS_SDK_SPEECH_TRANSCRIBER_SYNC_REQUEST_H

#include "nlsGlobal.h"
#include "speechTranscriberRequest.h"

#if defined(_WIN32)
	#pragma warning( push )
	#pragma warning ( disable : 4251 )
#endif

namespace AlibabaNls {

class NLS_SDK_CLIENT_EXPORT SpeechTranscriberSyncRequest {

public:
	SpeechTranscriberSyncRequest();
	~SpeechTranscriberSyncRequest();
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
    * @brief 发送语音数据
    * @note request对象format参数为为opu, encoded需设置为true. 其它格式默认为false.
    * @param data	语音数据
    * @param dataSize	语音数据长度(建议每次100ms左右数据)
	* @param status     语音数据的位置，第一块数据，中间数据块，最后一块数据
    * @param encoded	是否启用压缩, 默认为false不启用数据压缩.
    * @return 成功返发送数据的大小，失败返回-1
    */
	int sendSyncAudio(char* data, int dataSize, AudioDataStatus status, bool encoded = false);
	
	/**
	* @brief 同步获取实时音频流识别结果
	* @param eventQueue    识别响应的事件，包含识别结果、错误信息等，根据事件类型进行不同的处理，
	*                      事件类型请参考NlsEvent::EventType
	* @return 成功返回0，失败返回-1
	*/
	int getTranscriberResult(std::queue<NlsEvent>* eventQueue);

	/**
	* 内部接口
	*/
	bool isStarted();

private:
	SpeechTranscriberRequest* _request;
};

}

#if defined (_WIN32)
	#pragma warning( pop )
#endif

#endif //NLS_SDK_SPEECH_TRANSCRIBER_SYNC_REQUEST_H

