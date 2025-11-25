/*
 * Copyright 2025 Alibaba Group Holding Limited
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

#ifndef NLS_SDK_DASH_COSYVOICE_SYNTHESIZER_REQUEST_H
#define NLS_SDK_DASH_COSYVOICE_SYNTHESIZER_REQUEST_H

#include "iNlsRequest.h"
#include "nlsEvent.h"
#include "nlsGlobal.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace AlibabaNls {

class DashCosyVoiceSynthesizerParam;

class DashCosyVoiceSynthesizerCallback {
 public:
  DashCosyVoiceSynthesizerCallback();
  ~DashCosyVoiceSynthesizerCallback();

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

class NLS_SDK_CLIENT_EXPORT DashCosyVoiceSynthesizerRequest
    : public INlsRequest {
 public:
  DashCosyVoiceSynthesizerRequest(const char* sdkName = "cpp",
                                  bool isLongConnection = false);
  ~DashCosyVoiceSynthesizerRequest();

  /**
   * @brief 设置DashCosyVoiceSynthesizerRequest服务URL地址
   * @note 必填参数. 默认为公网服务URL地址.
   * @param value 服务url字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setUrl(const char* value);

  /**
   * @brief 口令认证。所有的请求都必须通过SetAPIKey方法认证通过，才可以使用
   * @note apikey需要申请获取
   * @param value	申请的apikey字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setAPIKey(const char* value);

  /**
   * @brief 设置Token的超期时间
   * @note 启用预连接池功能才有效, 用于刷新预连接池内部的节点. 如果不设置,
   * 则按照加入预连接池时间戳+30s为超期时间.
   * @param value Unix时间戳, 单位ms. 为Token创建时间获得.
   * @return 成功则返回0，否则返回负值错误码
   */
  int setTokenExpirationTime(uint64_t value);

  /**
   * @brief 指定要使用的模型。详情参见模型列表
   * @return 成功则返回0，否则返回负值错误码
   */
  int setModel(const char* value);

  /**
   * @brief 指定语音合成所使用的音色,支持默认音色和专属音色
   * @param value 发音人字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setVoice(const char* value);

  /**
   * @brief 音频编码格式Format设置
   * @note 可选参数.
   *       所有模型均支持的编码格式: pcm, wav, mp3(默认)
   *       除cosyvoice-v1外，其他模型支持的编码格式：opus
   *       音频格式为opus时，支持通过bit_rate参数调整码率。
   * @param value	音频编码格式字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setFormat(const char* value);

  /**
   * @brief 音频采样率sample_rate设置
   * @note 音频采样率，支持下述采样率(单位：Hz):
   *       8000,16000,22050(默认),24000,44100,48000。
   * @param value	音频采样率
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSampleRate(int value);

  /**
   * @brief 音量volume设置
   * @note 范围是0~100, 可选参数, 默认50
   * @param value 音量
   * @return 成功则返回0，否则返回负值错误码
   */
  int setVolume(int value);

  /**
   * @brief 合成音频的语速speech_rate设置
   * @note 范取值范围：0.5~2,默认值:1.0
   *       0.5:表示默认语速的0.5倍速。
   *       1:
   * 表示默认语速。默认语速是指模型默认输出的合成语速，语速会依据每一个音色略有不同，约每秒钟4个字。
   *       2:  表示默认语速的2倍速。
   * @param rate 语速
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSpeechRate(float rate);

  /**
   * @brief 合成音频的语调pitch_rate设置
   * @note 取值范围：0.5~2, 默认是1.0
   * @param pitch 语调
   * @return 成功则返回0，否则返回负值错误码
   */
  int setPitchRate(float pitch);

  /**
   * @brief 是否开启SSML功能
   * @note 该参数设为 true 后，仅允许发送一次文本，支持纯文本或包含SSML的文本。
   * @return
   */
  void setSsmlEnabled(bool enable);

  /**
   * @brief 指定音频的码率，取值范围：6~510kbps
   * @note 码率越大，音质越好，音频文件体积越大.
   *       仅在音频格式（format）为opus时可用。
   *       cosyvoice-v1模型不支持该参数
   * @param rate 码率
   * @return 成功则返回0，否则返回负值错误码
   */
  int setBitRate(int rate);

  /**
   * @brief 是否开启字级别时间戳，默认为false关闭。
   * @note 仅cosyvoice-v2支持该功能。
   * @param enable
   * @return 成功则返回0，否则返回负值错误码
   */
  int setWordTimestampEnabled(bool enable);

  /**
   * @brief 生成时使用的随机数种子，使合成的效果产生变化。
   * @note 默认值0。取值范围：0~65535。
   * @param seed
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSeed(int seed);

  /**
   * @brief 合成文本语言提示,可选值为zh(中文)或en(英文),列表中仅第一个语言生效.
   * @note 仅cosyvoice-v3、cosyvoice-v3-plus支持该功能。
   *       此设置会影响阿拉伯数字等内容的读法。
   *       例如，当合成“123”时，若设置为zh，则读作“一百二十三”；
   *       而en则会读作“one hundred and twenty-three”。
   *       如果不设置，系统会根据文本内容自动判断并应用相应的合成规则。
   * @return 成功则返回0，否则返回负值错误码
   */
  int setLanguageHints(const char* jsonArrayStr);

  /**
   * @brief 设置提示词。
   * @note 仅cosyvoice-v3默认音色支持该功能，声音复刻暂不支持。
   *       目前仅支持设置情感。
   *       格式：“你说话的情感是<情感值>。”（注意，结尾一定不要遗漏句号，使用时将“<情感值>”替换为具体的情感值，例如替换为neutral）。
   *       示例：“你说话的情感是neutral。”
   *       支持的情感值：neutral、fearful、angry、sad、surprised、happy、disgusted。
   * @return 成功则返回0，否则返回负值错误码
   */
  int setInstruction(const char* value);

  /**
   * @brief 单次合成的文本。
   * @note 调用则启用单轮语音合成, 否则是流式多轮的语音合成.
   * @param value 待合成的文本, 不超过1万字, 总计不超过10万字,
   * 其中1个汉字、1个英文字母、1个标点或1个句子中间空格均算作1个字符.
   * @return 成功则返回0，否则返回负值，查看nlsGlobal.h中错误码详细定位
   */
  int setSingleRoundText(const char* value);

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
   * @brief 对话服务的32位任务ID, 不填的话由内部生成.
   * @note 必填参数
   * @param taskId 任务ID
   * @return 成功则返回0，否则返回负值错误码
   */
  int setTaskId(const char* taskId);

  /**
   * @brief 获得当前请求的task_id
   * @return 返回当前请求的task_id
   */
  const char* getTaskId();

  /**
   * @brief 参数设置
   * @note  暂不对外开放
   * @param value 参数
   * @return 成功则返回0，否则返回负值错误码
   */
  int setPayloadParam(const char* value);

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
   * @brief 需要合成的文本. 单次不超过2000字, 总计不超过20万字,
   * 其中1个汉字、1个英文字母、1个标点或1个句子中间空格均算作1个字符.
   * @note 异步操作。失败返回TaskFailed。
   *       在持续发sendPing()的情况下，两次sendText()不超过23秒，否则会收到超时报错。
   *       在不发sendPing()的情况下，两次sendText()不超过10秒，否则会收到超时报错。
   * @return 成功则返回0，否则返回负值，查看nlsGlobal.h中错误码详细定位
   */
  int sendText(const char* text);

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
   * @brief 获得当前请求的运行状态。
   * @note 异步操作。失败返回TaskFailed。
   * @return 成功则返回0，否则返回负值错误码
   */
  NlsRequestStatus getRequestStatus();

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
  DashCosyVoiceSynthesizerParam* _flowingSynthesizerParam;
  DashCosyVoiceSynthesizerCallback* _callback;
  INlsRequestListener* _listener;
};

}  // namespace AlibabaNls

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif  // NLS_SDK_DASH_COSYVOICE_SYNTHESIZER_REQUEST_H
