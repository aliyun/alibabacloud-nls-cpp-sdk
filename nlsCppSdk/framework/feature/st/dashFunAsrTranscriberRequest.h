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

#ifndef NLS_SDK_DASH_FUNASR_TRANSCRIBER_REQUEST_H
#define NLS_SDK_DASH_FUNASR_TRANSCRIBER_REQUEST_H

#include "iNlsRequest.h"
#include "nlsEvent.h"
#include "nlsGlobal.h"

namespace AlibabaNls {

class DashFunAsrTranscriberParam;
class DashFunAsrTranscriberCallback {
 public:
  DashFunAsrTranscriberCallback();
  ~DashFunAsrTranscriberCallback();

  void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);
  void setOnTranscriptionStarted(NlsCallbackMethod _event, void* para = NULL);
  void setOnSentenceBegin(NlsCallbackMethod _event, void* para = NULL);
  void setOnTranscriptionResultChanged(NlsCallbackMethod _event,
                                       void* para = NULL);
  void setOnSentenceEnd(NlsCallbackMethod _event, void* para = NULL);
  void setOnTranscriptionCompleted(NlsCallbackMethod _event, void* para = NULL);
  void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);
  void setOnSentenceSemantics(NlsCallbackMethod _event, void* para);
  void setOnMessage(NlsCallbackMethod _event, void* para = NULL);

  NlsCallbackMethod _onSentenceSemantics;
  NlsCallbackMethod _onTaskFailed;
  NlsCallbackMethod _onTranscriptionStarted;
  NlsCallbackMethod _onSentenceBegin;
  NlsCallbackMethod _onTranscriptionResultChanged;
  NlsCallbackMethod _onSentenceEnd;
  NlsCallbackMethod _onTranscriptionCompleted;
  NlsCallbackMethod _onChannelClosed;
  NlsCallbackMethod _onMessage;
  std::map<NlsEvent::EventType, void*> _paramap;
};

class NLS_SDK_CLIENT_EXPORT DashFunAsrTranscriberRequest : public INlsRequest {
 public:
  DashFunAsrTranscriberRequest(const char* sdkName = "cpp",
                               bool isLongConnection = false);
  ~DashFunAsrTranscriberRequest();

  /**
   * @brief 设置实时音频流识别服务URL地址
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
   * @brief 指定要使用的模型。详情参见模型列表。
   * @return 成功则返回0，否则返回负值错误码
   */
  int setModel(const char* value);

  /**
   * @brief 设置音频数据编码格式
   * @note  可选参数，目前支持pcm, opus. 默认是pcm
   * @param value	音频数据编码字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setFormat(const char* value);

  /**
   * @brief 设置音频数据采样率sample_rate
   * @note 可选参数，目前支持16000, 8000. 默认是1600
   * @param value
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSampleRate(int value);

  /**
   * @brief 设置热词ID
   * @param value 定制泛热词id字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setVocabularyId(const char* value);

  /**
   * @brief 是否开启语义断句。
   * @note  默认值：false。
   *        true：
   *            使用语义断句。关闭 VAD（Voice Activity
   * Detection，语音活动检测）断句。适合会议转写，准确度高。 false： 使用 VAD
   * 断句。关闭语义断句。适合交互场景，延迟低。
   *        语义断句能更精准地识别句子边界。VAD 断句响应更快。
   *        通过调整 semantic_punctuation_enabled
   * 参数，可在不同场景间灵活切换断句方式。
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSemanticPunctuationEnabled(bool enable);

  /**
   * @brief VAD静音时长阈值。在 VAD（Voice Activity
   * Detection，语音活动检测）断句中，静音时长超过该阈值即判定句子结束。
   * @note  单位：毫秒（ms）。默认值：1300。取值范围：[200, 6000]。
   *        生效条件：仅在 semantic_punctuation_enabled 参数为 false（使用 VAD
   * 断句）时有效。
   * @param value vad阀值
   * @return 成功则返回0，否则返回负值错误码
   */
  int setMaxSentenceSilence(int value);

  /**
   * @brief 是否开启防止 VAD 断句过长的功能。
   * @note  默认值：false。
   *        true：开启，限制 VAD（Voice Activity
   * Detection，语音活动检测）断句长度，避免过长切割。
   *        false：关闭。生效条件：仅在 semantic_punctuation_enabled 为
   * false（使用 VAD 断句）时有效。
   * @return 成功则返回0，否则返回负值错误码
   */
  int setMultiThresholdModeEnabled(bool enable);

  /**
   * @brief 是否开启长连接保持开关。
   * @note  默认值：false。
   *        true：开启。在持续发送静音音频时，连接与服务端保持不中断。
   *        false：关闭。即使持续发送静音音频，60 秒后连接因超时断开。
   *        说明：
   *        静音音频：音频文件或数据流中无声音信号的内容。
   *        生成方式：可用音频编辑软件（如 Audacity、Adobe
   * Audition），或命令行工具（如 FFmpeg）创建。
   * @return 成功则返回0，否则返回负值错误码
   */
  int setHeartbeat(bool enable);

  /**
   * @brief 设置链接超时时间
   * @param value 超时时间(ms), 默认500ms. 内部会以value(ms)尝试4次链接.
   * @return 成功则返回0，否则返回负值错误码
   */
  int setTimeout(int value);

  /**
   * @brief 设置开启接收超时时间
   * @param value 默认false, 即默认关闭接收超时时间,
   *              开启后长时间未收服务端则报错
   * @return 成功则返回0，否则返回负值错误码
   */
  int setEnableRecvTimeout(bool value);

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
   * @note 暂不支持, 输出均为UTF-8
   * @param value 编码格式 UTF-8 or GBK
   * @return 成功则返回0，否则返回负值错误码
   */
  int setOutputFormat(const char* value);

  /**
   * @brief 获得设置的输出文本的编码格式
   * @return 返回 UTF-8 or GBK
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
   * @brief 设置开启服务器返回消息回调。听悟实时推流功能请开启此功能。
   * @param value 默认false, 即默认不开启服务器返回消息回调
   * @return 成功则返回0，否则返回负值错误码
   */
  int setEnableOnMessage(bool enable);

  /**
   * @brief
   * 设置是否开启重连续传，需要明确服务是否支持重连续传，否则会导致无法正常抛出异常。
   * @param enable 默认false, 是否开启重连续传
   * @return 成功则返回0，否则返回负值错误码
   */
  int setEnableContinued(bool enable);

  /**
   * @brief 启动实时音频流识别
   * @note 异步操作。成功返回started事件。失败返回TaskFailed事件。
   * @return 成功则返回0，否则返回负值，查看nlsGlobal.h中错误码详细定位
   */
  int start();

  /**
   * @brief 会与服务端确认关闭，正常停止实时音频流识别操作
   * @note 异步操作。失败返回TaskFailed。
   * @return 成功则返回0，否则返回负值，查看nlsGlobal.h中错误码详细定位
   */
  int stop();

  /**
   * @brief 直接关闭实时音频流识别过程
   * @note 调用cancel之后不会在上报任何回调事件
   * @return 成功则返回0，否则返回负值，查看nlsGlobal.h中错误码详细定位
   */
  int cancel();

  /**
   * @brief 发送语音数据
   * @note 异步操作。request
   * @param data 语音数据
   * @param dataSize 语音数据长度(建议每次100ms左右数据, 且推荐少于16K字节)
   * @param type ENCODER_NONE 表示原始音频进行传递,
                              建议每次100ms音频数据,支持16K和8K;
                 ENCODER_OPU 表示以定制OPUS压缩后进行传递,
                             建议每次大于20ms音频数据(即大于640bytes),
   格式要求16K16b1c ENCODER_OPUS 表示以OPUS压缩后进行传递, 强烈建议使用此类型,
                              建议每次大于20ms音频数据(即大于640/320bytes),
   支持16K16b1c和8K16b1c
   * @return
   成功则返回字节数(可能为0，即留下包音频数据再发送)，失败返回负值，查看nlsGlobal.h中错误码详细定位。
   */
  int sendAudio(const uint8_t* data, size_t dataSize,
                ENCODER_TYPE type = ENCODER_NONE);

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
   * @note 在请求过程中出现异常错误时，sdk内部线程上报该回调。
   *       用户可以在事件的消息头中检查状态码和状态消息，以确认失败的具体原因.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置实时音频流识别开始回调函数
   * @note 服务端就绪、可以开始识别时，sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnTranscriptionStarted(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置一句话开始回调
   * @note 检测到一句话的开始时, sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSentenceBegin(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置实时音频流识别中间结果回调函数
   * @note 设置enable_intermediate_result字段为true，才会有中间结果.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnTranscriptionResultChanged(NlsCallbackMethod _event,
                                       void* para = NULL);

  /**
   * @brief 设置一句话结束回调函数
   * @note 检测到了一句话的结束时, sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSentenceEnd(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置服务端结束服务回调函数
   * @note 云端结束实时音频流识别服务时, sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnTranscriptionCompleted(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置通道关闭回调函数
   * @note 在请求过程中通道关闭时，sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置二次处理结果回调函数
   * @note 表示对实时转写的原始结果进行处理后的结果, 开启enable_nlp后返回
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnSentenceSemantics(NlsCallbackMethod _event, void* para = NULL);

  /**
   * @brief 设置服务端response message回调函数
   * @note 表示返回所有服务端返回的结果
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnMessage(NlsCallbackMethod _event, void* para = NULL);

 private:
  DashFunAsrTranscriberParam* _transcriberParam;
  DashFunAsrTranscriberCallback* _callback;
  INlsRequestListener* _listener;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_DASH_FUNASR_TRANSCRIBER_REQUEST_H
