/*
 * Copyright 2021 Alibaba Group Holding Limited
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

#include "iNlsRequest.h"
#include "nlsEvent.h"

namespace AlibabaNls {

class SpeechRecognizerParam;

class SpeechRecognizerCallback {
 public:
  SpeechRecognizerCallback();
  ~SpeechRecognizerCallback();

  void setOnTaskFailed(NlsCallbackMethod event, void* param = NULL);
  void setOnRecognitionStarted(NlsCallbackMethod event, void* param = NULL);
  void setOnRecognitionCompleted(NlsCallbackMethod event, void* param = NULL);
  void setOnRecognitionResultChanged(NlsCallbackMethod event,
                                     void* param = NULL);
  void setOnChannelClosed(NlsCallbackMethod event, void* param = NULL);
  void setOnMessage(NlsCallbackMethod event, void* param = NULL);

  NlsCallbackMethod _onTaskFailed;
  NlsCallbackMethod _onRecognitionStarted;
  NlsCallbackMethod _onRecognitionCompleted;
  NlsCallbackMethod _onRecognitionResultChanged;
  NlsCallbackMethod _onChannelClosed;
  NlsCallbackMethod _onMessage;
  std::map<NlsEvent::EventType, void*> _paramap;
};

class NLS_SDK_CLIENT_EXPORT SpeechRecognizerRequest : public INlsRequest {
 public:
  SpeechRecognizerRequest(const char* sdkName = "cpp",
                          bool isLongConnection = false);
  virtual ~SpeechRecognizerRequest();

  /**
   * @brief 设置一句话识别服务URL地址
   * @note 必填参数. 默认为公网服务URL地址.
   * @param value 服务url字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setUrl(const char* value);

  /**
   * @brief 设置appKey
   * @note 必填参数, 请参照官网申请
   * @param value appKey字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setAppKey(const char* value);

  /**
   * @brief 口令认证。所有的请求都必须通过SetToken方法认证通过，才可以使用
   * @note token需要申请获取, 必填参数
   * @param value 申请的token字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setToken(const char* value);

  /**
   * @brief 设置Token的超期时间
   * @note 启用预连接池功能才有效, 用于刷新预连接池内部的节点. 如果不设置,
   * 则按照加入预连接池时间戳+12h为超期时间.
   * @param value Unix时间戳, 单位ms. 为Token创建时间获得.
   * @return 成功则返回0，否则返回负值错误码
   */
  int setTokenExpirationTime(uint64_t value);

  /**
   * @brief 设置音频数据编码格式字段Format
   * @param value 可选参数, 目前支持pcm|opus|opu. 默认是pcm
   * @return 成功则返回0，否则返回负值错误码
   */
  int setFormat(const char* value);

  /**
   * @brief 设置字段sample_rate
   * @param value 可选参数. 目前支持16000, 8000. 默认是16000
   * @return 成功则返回0，否则返回负值错误码
   */
  int setSampleRate(int value);

  /**
   * @brief 设置定制模型
   * @param value 定制模型id字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setCustomizationId(const char* value);

  /**
   * @brief 设置泛热词
   * @param value 定制泛热词id字符串
   * @return 成功则返回0，否则返回负值错误码
   */
  int setVocabularyId(const char* value);

  /**
   * @brief 设置字段enable_intermediate_result设置
   * @param value 是否返回中间识别结果, 可选参数. 默认false
   * @return 成功则返回0，否则返回负值错误码
   */
  int setIntermediateResult(bool value);

  /**
   * @brief 设置字段enable_punctuation_prediction
   * @param value 是否在后处理中添加标点, 可选参数. 默认false
   * @return 成功则返回0，否则返回负值错误码
   */
  int setPunctuationPrediction(bool value);

  /**
   * @brief 设置字段enable_inverse_text_normalization
   * @param value 是否在后处理中执行ITN, 可选参数. 默认false
   * @return 成功则返回0，否则返回负值错误码
   */
  int setInverseTextNormalization(bool value);

  /**
   * @brief 设置字段enable_voice_detection设置
   * @param value 是否启动自定义静音检测, 可选.
   *              默认是False. 云端默认静音检测时间800ms.
   * @return 成功则返回0，否则返回负值错误码
   */
  int setEnableVoiceDetection(bool value);

  /**
   * @brief 设置字段max_start_silence
   * @param value 允许的最大开始静音, 可选. 单位是毫秒.
   *              超出设置时间后(即开始识别后多长时间没有检测到声音)服务端将会发送TaskFailed事件,
   * 结束本次识别. 需要先设置enable_voice_detection为true. 建议时间2~5秒.
   * @return 成功则返回0，否则返回负值错误码
   */
  int setMaxStartSilence(int value);

  /**
   * @brief 设置字段max_end_silence
   * @param value 允许的最大结束静音, 可选, 单位是毫秒.
   *              超出时长服务端会发送RecognitionCompleted事件,
   *              结束本次识别(需要注意后续的语音将不会进行识别).
   *              需要先设置enable_voice_detection为true. 建议时间0~5秒.
   * @return 成功则返回0，否则返回负值错误码
   */
  int setMaxEndSilence(int value);

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
   * @param value 编码格式 UTF-8 or GBK
   * @return 成功则返回0，否则返回负值错误码
   */
  int setOutputFormat(const char* value);

  /**
   * @brief 获得设置的输出文本的编码格式
   * @return 返回  UTF-8 or GBK
   */
  const char* getOutputFormat();

  /*
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
   * @brief 设置用户自定义ws阶段http header参数
   * @param key 参数名称
   * @param value 参数内容
   * @return 成功则返回0，否则返回负值错误码
   */
  int AppendHttpHeaderParam(const char* key, const char* value);

  /**
   * @brief 可通过公网访问的音频文件下载链接
   * @param value 音频文件下载链接, 推荐使用阿里云OSS
   * @return 成功则返回0，否则返回负值错误码
   */
  int setAudioAddress(const char* value);

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
   * @brief 启动SpeechRecognizerRequest
   * @note 异步操作。成功返回started事件。失败返回TaskFailed事件。
   * @return 成功则返回0，否则返回负值错误码
   */
  int start();

  /**
   * @brief 会与服务端确认关闭，正常停止SpeechRecognizerRequest链接操作
   * @note 异步操作。失败返回TaskFailed。
   * @return 成功则返回0，否则返回负值错误码
   */
  int stop();

  /**
   * @brief 不会与服务端确认关闭，直接关闭SpeechRecognizerRequest链接.
   * @note 调用cancel之后不会返回任何回调事件。
   * @return 成功则返回0，否则返回负值错误码
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
   * @brief 设置错误回调函数
   * @note 在请求过程中出现错误时, sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnTaskFailed(NlsCallbackMethod event, void* param = NULL);

  /**
   * @brief 设置一句话识别开始回调函数
   * @note 在语音识别可以开始时, sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnRecognitionStarted(NlsCallbackMethod event, void* param = NULL);

  /**
   * @brief 设置一句话识别结束回调函数
   * @note 在语音识别完成时, sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnRecognitionCompleted(NlsCallbackMethod event, void* param = NULL);

  /**
   * @brief 设置一句话识别中间结果回调函数
   * @note 设置enable_intermediate_result字段为true, 才会有中间结果.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnRecognitionResultChanged(NlsCallbackMethod event,
                                     void* param = NULL);

  /**
   * @brief 设置通道关闭回调函数
   * @note 在请求过程中通道关闭时, sdk内部线程上报该回调.
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnChannelClosed(NlsCallbackMethod event, void* param = NULL);

  /**
   * @brief 设置服务端response message回调函数
   * @note 表示返回所有服务端返回的结果
   * @param _event 回调方法
   * @param para 用户传入参数, 默认为NULL
   * @return void
   */
  void setOnMessage(NlsCallbackMethod _event, void* para = NULL);

 private:
  SpeechRecognizerCallback* _callback;
  SpeechRecognizerParam* _recognizerParam;
  INlsRequestListener* _listener;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_SPEECH_RECOGNIZER_REQUEST_H
