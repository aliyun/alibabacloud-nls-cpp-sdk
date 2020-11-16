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

#ifndef NLS_SDK_DIALOG_ASSISTANT_REQUEST_H
#define NLS_SDK_DIALOG_ASSISTANT_REQUEST_H

#include "nlsGlobal.h"
#include "iNlsRequest.h"

#if defined(_WIN32)
	#pragma warning(push)
	#pragma warning(disable : 4251)
#endif

namespace AlibabaNls {

class DialogAssistantParam;

class NLS_SDK_CLIENT_EXPORT DialogAssistantCallback {

public:
DialogAssistantCallback();
~DialogAssistantCallback();

void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);
void setOnRecognitionStarted(NlsCallbackMethod _event, void* para = NULL);
void setOnRecognitionResultChanged(NlsCallbackMethod _event, void* para = NULL);
void setOnRecognitionCompleted(NlsCallbackMethod _event, void* para = NULL);
void setOnDialogResultGenerated(NlsCallbackMethod _event, void* para = NULL);
void setOnWakeWordVerificationCompleted(NlsCallbackMethod _event, void* para = NULL);
void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);

NlsCallbackMethod _onTaskFailed;
NlsCallbackMethod _onRecognitionStarted;
NlsCallbackMethod _onRecognitionCompleted;
NlsCallbackMethod _onRecognitionResultChanged;
NlsCallbackMethod _onDialogResultGenerated;
NlsCallbackMethod _onWakeWordVerificationCompleted;
NlsCallbackMethod _onChannelClosed;
std::map < NlsEvent::EventType, void*> _paramap;
};

class NLS_SDK_CLIENT_EXPORT DialogAssistantRequest : public INlsRequest {

public:
DialogAssistantRequest(int version = 0);
~DialogAssistantRequest();

/**
    * @brief 设置一句话识别服务URL地址
    * @note 必填参数.默认为公网服务URL地址.
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
    * @brief 对话服务的会话ID.
    * @note 必填参数
    * @param sessionId 会话ID
    * @return 成功则返回0，否则返回-1
    */
int setSessionId(const char* sessionId);

/**
    * @brief 对话参数.
    * @note 可选参数
    * @param value 待定
    * @return 成功则返回0，否则返回-1
    */
int setQueryParams(const char* value);

/**
    * @brief 对话附加信息.
    * @note 可选参数
    * @param value 附加信息
    * @return 成功则返回0，否则返回-1
    */
int setQueryContext(const char* value);

/**
    * @brief 对话的输入文本.
    * @note 调用sendExecuteDialog()时必填.调用start()时, 不用填写.
    * @param value 输入文本
    * @return 成功则返回0，否则返回-1
    */
int setQuery(const char* value);

/**
    * @brief 参数设置
    * @note  暂不对外开放
    * @param value 参数.
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
    * @brief 是否对语音唤醒进行云端确认. 可选, 默认是false.
    * @param value 参数
    * @return 成功则返回0，否则返回-1
    */
int setEnableWakeWordVerification(bool value);

/**
    * @brief 客户端检测到的唤醒词.
    * @note  setEnableWakeWordVerification如果设置为true, 此参数必须设置
    * @param value 参数
    * @return 成功则返回0, 否则返回-1.
    */
int setWakeWord(const char* value);

/**
    * @brief 唤醒词服务的模型名称. 可选.
	* @note  setEnableWakeWordVerification如果设置为true, 此参数必须设置
    * @param value 参数
    * @return 成功则返回0, 否则返回-1.
    */
int setWakeWordModel(const char* value);

/**
    * @brief 设置用户自定义ws阶段http header参数
    * @param key 参数名称
    * @param value 参数内容
    * @return 成功则返回0，否则返回-1
    */
int AppendHttpHeaderParam(const char* key, const char* value);

/**
    * @brief 启动DialogAssistantRequest
    * @note 异步操作。成功返回started事件。失败返回TaskFailed事件。
    * @return 成功则返回0，否则返回-1
    */
int start();

/**
    * @brief 会与服务端确认关闭，正常停止DialogAssistantRequest链接操作
    * @note 异步操作。失败返回TaskFailed。
    * @return 成功则返回0，否则返回-1
    */
int stop();

/**
    * @brief 直接关闭DialogAssistantRequest链接.
    * @note 调用cancel之后不会返回任何回调事件。
    * @return 成功则返回0，否则返回-1
    */
int cancel();

/**
    * @brief 会与服务端确认关闭，正常停止DialogAssistantRequest链接操作
    * @note 阻塞操作，等待服务端响应才会返回
    * @return 成功则返回0，否则返回-1
    */
int	StopWakeWordVerification();

/**
    * @brief 启用文本进, 文本出
    * @return 成功则返回0，失败返回-1
    */
int queryText();

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
void setOnTaskFailed(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置语音助手开始回调函数
    * @note 用于通知客户端服务端已准备好接受音频数据.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnRecognitionStarted(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 用于通知客户端, 随着语音数据增多, 语音识别结果发生了变化. 客户端可以根据这个事件更新UI状态
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnRecognitionResultChanged(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 用于通知客户端识, 最终的识别结果已经确定, 不会再发生变化. 客户端可以根据这个事件更新UI状态
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnRecognitionCompleted(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 用于通知客户端用户的意图已经被确认
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnDialogResultGenerated(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 用于语音唤醒的二次确认
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnWakeWordVerificationCompleted(NlsCallbackMethod _event, void* para = NULL);

/**
    * @brief 设置通道关闭回调函数
    * @note 在请求过程中通道关闭时, sdk内部线程上报该回调.
    * @param _event	回调方法
    * @param para	用户传入参数, 默认为NULL
    * @return void
    */
void setOnChannelClosed(NlsCallbackMethod _event, void* para = NULL);

void setEnableMultiGroup(bool value);

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
DialogAssistantParam* _dialogAssistantParam;
DialogAssistantCallback* _callback;
INlsRequestListener* _listener;

};

}

#if defined (_WIN32)
	#pragma warning(pop)
#endif

#endif //NLS_SDK_DIALOG_ASSISTANT_REQUEST_H
