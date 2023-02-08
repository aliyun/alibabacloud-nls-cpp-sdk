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

#ifndef NLS_SDK_SPEECH_SYNTHESIZER_REQUEST_H
#define NLS_SDK_SPEECH_SYNTHESIZER_REQUEST_H

#include "nlsGlobal.h"
#include "iNlsRequest.h"

#if defined(_MSC_VER)
	#pragma warning( push )
	#pragma warning ( disable : 4251 )
#endif

namespace AlibabaNls {

class SpeechSynthesizerParam;
class INlsRequestListener;

class NLS_SDK_CLIENT_EXPORT SpeechSynthesizerCallback {
 public:
  SpeechSynthesizerCallback();
  ~SpeechSynthesizerCallback();

  void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);
  void setOnSynthesisStarted(NlsCallbackMethod _event, void* para = NULL);
  void setOnSynthesisCompleted(NlsCallbackMethod _event, void* para = NULL);
  void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);
  void setOnBinaryDataReceived(NlsCallbackMethod _event, void* para = NULL);
  void setOnMetaInfo(NlsCallbackMethod _event, void* para = NULL);
  void setOnMessage(NlsCallbackMethod _event, void* para = NULL);

  NlsCallbackMethod _onTaskFailed;
  NlsCallbackMethod _onSynthesisStarted;
  NlsCallbackMethod _onSynthesisCompleted;
  NlsCallbackMethod _onChannelClosed;
  NlsCallbackMethod _onBinaryDataReceived;
  NlsCallbackMethod _onMetaInfo;
  NlsCallbackMethod _onMessage;
  std::map<NlsEvent::EventType, void*> _paramap;
};

class NLS_SDK_CLIENT_EXPORT SpeechSynthesizerRequest : public INlsRequest {
 public:
  SpeechSynthesizerRequest(int version = 0,
                           const char* sdkName = "cpp",
                           bool isLongConnection = false);
  ~SpeechSynthesizerRequest();

  /**
   * @brief 设置SpeechSynthesizer服务URL地址
   * @note 必填参数. 默认为公网服务URL地址.
   * @param value 服务url字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setUrl(const char* value);

  /**
   * @brief 设置appKey
   * @note 请参照官网申请, 必选参数
   * @param value appKey字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setAppKey(const char* value);

  /**
   * @brief 口令认证。所有的请求都必须通过SetToken方法认证通过，才可以使用
   * @note token需要申请获取, 必选参数.
   * @param value	申请的token字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setToken(const char* value);

  /**
   * @brief 音频编码格式Format设置
   * @note 可选参数, 默认是pcm. 支持的格式pcm, wav, mp3
   * @param value	音频编码格式字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setFormat(const char* value);

  /**
   * @brief 音频采样率sample_rate设置
   * @note 包含8000, 16000.可选参数, 默认是16000
   * @param value	音频采样率
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSampleRate(int value);

  /**
   * @brief 待合成音频文本内容text设置
   * @note 必选参数，需要传入UTF-8编码的文本内容
   *       短文本语音合成模式下(默认), 支持一次性合成300字符以内的文字,
   *       其中1个汉字、1个英文字母或1个标点均算作1个字符,
   *       超过300个字符的内容将会报错(或者截断).
   *       超过300个字符可考虑长文本语音合成, 详见官方文档.
   * @param value	待合成文本字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setText(const char* value);

  /**
   * @brief 发音人voice设置
   * @note 包含"xiaoyun", "xiaogang". 可选参数, 默认是xiaoyun.
   * @param value 发音人字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setVoice(const char* value);

  /**
   * @brief 音量volume设置
   * @note 范围是0~100, 可选参数, 默认50
   * @param value 音量
   * @return 成功则返回0，否则返回负值错误码
   */
  int setVolume(int value);

  /**
   * @brief 语速speech_rate设置
   * @note 范围是-500~500, 可选参数, 默认是0
   * @param value 语速
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSpeechRate(int value);

  /**
   * @brief 语调pitch_rate设置
   * @note 范围是-500~500, 可选参数, 默认是0
   * @param value 语调
   * @return 成功则返回0，否则返回负值错误码
   */
  int setPitchRate(int value);

  /**
   * @brief 合成方法method设置
   * @note 可选参数, 默认是0.
   *       0 统计参数合成: 基于统计参数的语音合成，优点是能适应的韵律特征的范围较宽，合成器比特率低，资源占用小，性能高，音质适中
   *       1 波形拼接合成: 基于高质量音库提取学习合成，资源占用相对较高，音质较好，更加贴近真实发音，但没有参数合成稳定
   * @param value 语调
   * @return 成功则返回0，否则返回负值错误码
   */
  int setMethod(int value);

  /**
   * @brief 是否开启字幕功能
   * @param value
   * @return 成功则返回0，否则返回负值错误码
   */
  int setEnableSubtitle(bool value);

  /**
   * @brief 参数设置
   * @note  暂不对外开放
   * @param value 参数
   * @return 成功则返回0，否则返回负值错误码
   */
  int setPayloadParam(const char* value);

  /**
   * @brief 设置链接超时时间
   * @param value 超时时间(ms), 默认5000ms
   * @return 成功则返回0，否则返回负值错误码
   */
  int setTimeout(int value);

  /**
   * @brief 设置接收超时时间
   * @param value 超时时间(ms), 默认15000ms
   * @return 成功则返回0，否则返回负值错误码
   */
  int setRecvTimeout(int value);

  /**
   * @brief 设置发送超时时间
   * @param value 超时时间(ms), 默认5000ms
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSendTimeout(int value);

  /**
   * @brief 设置输出文本的编码格式
   * @note
   * @param value 编码格式 UTF-8 or GBK
   * @return 成功则返回0，否则返回负值错误码
   */
  int setOutputFormat(const char* value);

  /*
   * @brief 获得设置的输出文本的编码格式
   * @return 返回  UTF-8 or GBK
   */
  const char* getOutputFormat();

  /**
   * @brief 设置用户自定义参数
   * @param value 参数
   * @return 成功则返回0，否则返回负值错误码
   */
  int setContextParam(const char* value);

  /**
   * @brief 设置开启服务器返回消息回调
   * @param value 默认false, 即默认不开启服务器返回消息回调
   * @return 成功则返回0，否则返回负值错误码
   */
  int setEnableOnMessage(bool value);

  /**
   * @brief 设置用户自定义ws阶段http header参数
   * @param key 参数名称
   * @param value 参数内容
   * @return 成功则返回0，否则返回负值错误码
   */
  int AppendHttpHeaderParam(const char* key, const char* value);

  /**
   * @brief 启动SpeechSynthesizerRequest
   * @note 异步操作。成功返回BinaryRecv事件。失败返回TaskFailed事件。
   * @return 成功则返回0，否则返回负值错误码
   */
  int start();

  /**
   * @brief 此接口废弃，调用与否都会正常停止SpeechSynthesizerRequest链接操作
   * @note 异步操作。失败返回TaskFailed。
   * @return 成功则返回0，否则返回负值错误码
   */
  int stop();

  /**
   * @brief 不会与服务端确认关闭，直接关闭语音合成过程
   * @note 调用cancel之后不会在上报任何回调事件
   * @return 成功则返回0，否则返回负值错误码
   */
  int cancel();

  /**
   * @brief 设置错误回调函数
   * @note 在语音合成过程中出现错误时，sdk内部线程该回调上报.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置语音合成结束回调函数
   * @note 在语音合成完成时，sdk内部线程该回调上报.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSynthesisCompleted(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置通道关闭回调函数
   * @note 语音合成连接通道关闭时，sdk内部线程该回调上报.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置语音合成二进制音频数据接收回调函数
   * @note 接收到服务端发送的二进制音频数据时，sdk内部线程上报该回调函数.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   * @notice 切不可在回调中进行阻塞操作，只可做音频数据转存，否则整个流程将会有较大延迟
   */
  void setOnBinaryDataReceived(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置文本对应的日志信息接收回调函数
   * @note 接收到服务端送回文本对应的日志信息，增量返回对应的字幕信息时，sdk内部线程上报该回调函数.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnMetaInfo(NlsCallbackMethod _event, void* para = NULL);

  /*
   * @brief 设置服务端response message回调函数
   * @note 表示返回所有服务端返回的结果
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnMessage(NlsCallbackMethod _event, void* para = NULL);


 private:
  SpeechSynthesizerParam* _synthesizerParam;
  SpeechSynthesizerCallback* _callback;
  INlsRequestListener* _listener;

};

}

#if defined (_MSC_VER)
	#pragma warning( pop )
#endif

#endif //NLS_SDK_SPEECH_SYNTHESIZER_REQUEST_H
