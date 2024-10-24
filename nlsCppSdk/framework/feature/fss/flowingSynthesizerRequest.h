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

#ifndef NLS_SDK_FLOWING_SYNTHESIZER_REQUEST_H
#define NLS_SDK_FLOWING_SYNTHESIZER_REQUEST_H

#include "iNlsRequest.h"
#include "nlsEvent.h"
#include "nlsGlobal.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace AlibabaNls {

class FlowingSynthesizerParam;

class FlowingSynthesizerCallback {
 public:
  FlowingSynthesizerCallback();
  ~FlowingSynthesizerCallback();

  void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);
  void setOnSynthesisStarted(NlsCallbackMethod _event, void* para = NULL);
  void setOnSynthesisCompleted(NlsCallbackMethod _event, void* para = NULL);
  void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);
  void setOnBinaryDataReceived(NlsCallbackMethod _event, void* para = NULL);
  void setOnSentenceBegin(NlsCallbackMethod _event, void* para = NULL);
  void setOnSentenceEnd(NlsCallbackMethod _event, void* para = NULL);
  void setOnSentenceSynthesis(NlsCallbackMethod _event, void* para = NULL);
  void setOnMessage(NlsCallbackMethod _event, void* para = NULL);

  NlsCallbackMethod _onTaskFailed;
  NlsCallbackMethod _onSynthesisStarted;
  NlsCallbackMethod _onSynthesisCompleted;
  NlsCallbackMethod _onChannelClosed;
  NlsCallbackMethod _onBinaryDataReceived;
  NlsCallbackMethod _onSentenceBegin;
  NlsCallbackMethod _onSentenceEnd;
  NlsCallbackMethod _onSentenceSynthesis;
  NlsCallbackMethod _onMessage;
  std::map<NlsEvent::EventType, void*> _paramap;
};

class NLS_SDK_CLIENT_EXPORT FlowingSynthesizerRequest : public INlsRequest {
 public:
  FlowingSynthesizerRequest(const char* sdkName = "cpp",
                            bool isLongConnection = false);
  ~FlowingSynthesizerRequest();

  /**
   * @brief 设置FlowingSynthesizerRequest服务URL地址
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
   * @param value 超时时间(ms), 默认500ms. 内部会以value(ms)尝试4次链接.
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

  /**
   * @brief 获得设置的输出文本的编码格式
   * @return 返回  UTF-8 or GBK
   */
  const char* getOutputFormat();

  /**
   * @brief 获得当前请求的task_id
   * @return 返回当前请求的task_id
   */
  const char* getTaskId();

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
   * @brief
   * 设置是否开启重连续传，需要明确服务是否支持重连续传，否则会导致无法正常抛出异常。
   * @param enable 默认false, 是否开启重连续传
   * @return 成功则返回0，否则返回负值错误码
   */
  int setEnableContinued(bool enable);

  /**
   * @brief 设置用户自定义ws阶段http header参数
   * @param key 参数名称
   * @param value 参数内容
   * @return 成功则返回0，否则返回负值错误码
   */
  int AppendHttpHeaderParam(const char* key, const char* value);

  /**
   * @brief 启动FlowingSynthesizerRequest
   * @note 异步操作。成功返回BinaryRecv事件。失败返回TaskFailed事件。
   * @return 成功则返回0，否则返回负值错误码
   */
  int start();

  /**
   * @brief 正常停止FlowingSynthesizerRequest, 表示送完待合成文本,
   * 开始等待收完所有合成音频。
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
   * @brief 需要合成的文本。
   * @note 异步操作。失败返回TaskFailed。
   *       在持续发sendPing()的情况下，两次sendText()不超过23秒，否则会收到超时报错。
   *       在不发sendPing()的情况下，两次sendText()不超过10秒，否则会收到超时报错。
   * @return 成功则返回0，否则返回负值，查看nlsGlobal.h中错误码详细定位
   */
  int sendText(const char* text);

  /**
   * @brief 表示送完待合成文本, 立即开始合成音频。
   * @note 异步操作。失败返回TaskFailed。
   * @return 成功则返回0，否则返回负值错误码
   */
  int sendFlush();

  /**
   * @brief 获得当前请求的全部运行信息
   * @note
   *{
   *	"block": {
   *    // 表示调用了stop, 已经运行了10014ms, 阻塞
   *		"stop": "running",
   *		"stop_duration_ms": 10014,
   *		"stop_timestamp": "2024-04-26_15:19:44.783"
   *	},
   *	"callback": {
   *    // 表示回调SentenceBegin阻塞
   *		"name": "SentenceBegin",
   *		"start": "2024-04-26_15:15:25.607",
   *		"status": "running"
   *	},
   *	"data": {
   *		"recording_bytes": 183764,
   *		"send_count": 288
   *	},
   *	"last": {
   *		"action": "2024-04-26_15:15:30.897",
   *		"send": "2024-04-26_15:15:30.894",
   *		"status": "NodeStop"
   *	},
   *	"timestamp": {
   *		"create": "2024-04-26_15:15:24.862",
   *		"start": "2024-04-26_15:15:24.862",
   *		"started": "2024-04-26_15:15:25.124",
   *		"stop": "2024-04-26_15:15:30.897"
   *	}
   *}
   * @return
   */
  const char* dumpAllInfo();

  /**
   * @brief 设置错误回调函数
   * @note 在语音合成过程中出现错误时，sdk内部线程该回调上报.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置语音合成开始回调函数
   * @note 在语音合成可以开始时，sdk内部线程该回调上报.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSynthesisStarted(NlsCallbackMethod _event, void* para = NULL);

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
   * @notice
   * 切不可在回调中进行阻塞操作，只可做音频数据转存，否则整个流程将会有较大延迟
   */
  void setOnBinaryDataReceived(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 服务端检测到了一句话的开始。
   * @note sdk内部线程该回调上报.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSentenceBegin(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 服务端检测到了一句话的结束，返回该句的全量时间戳。
   * @note sdk内部线程该回调上报.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSentenceEnd(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置对应结果的信息接收回调函数
   * @note
   * 表示有新的合成结果返回，包含最新的音频和时间戳，句内全量，句间增量，sdk内部线程上报该回调函数.
   * @param _event	回调方法
   * @param para	用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSentenceSynthesis(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置服务端response message回调函数
   * @note 表示返回所有服务端返回的结果
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnMessage(NlsCallbackMethod _event, void* para = NULL);

 private:
  FlowingSynthesizerParam* _flowingSynthesizerParam;
  FlowingSynthesizerCallback* _callback;
  INlsRequestListener* _listener;
};

}  // namespace AlibabaNls

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif  // NLS_SDK_FLOWING_SYNTHESIZER_REQUEST_H
