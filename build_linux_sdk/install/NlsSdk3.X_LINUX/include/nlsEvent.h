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

#ifndef NLS_SDK_EVENT_H
#define NLS_SDK_EVENT_H

#include <string>
#include <vector>
#include <list>
#include "nlsGlobal.h"

#if defined(_WIN32)
	#pragma warning( push )
	#pragma warning ( disable : 4251 )
#endif

namespace AlibabaNls {

enum AudioDataStatus {
	AUDIO_FIRST = 0, /**第一块音频数据**/ 
	AUDIO_MIDDLE, /**中间音频数据**/
	AUDIO_LAST /**最后一块音频数据**/
};

typedef struct {
	std::string text;
	int startTime;
	int endTime;
}WordInfomation;

class NLS_SDK_CLIENT_EXPORT NlsEvent {

public:

enum EventType {
    TaskFailed = 0,
	RecognitionStarted,
	RecognitionCompleted,
	RecognitionResultChanged,
	WakeWordVerificationCompleted,
	TranscriptionStarted,
	SentenceBegin,
	TranscriptionResultChanged,
	SentenceEnd,
	SentenceSemantics,
	TranscriptionCompleted,
	SynthesisStarted,
	SynthesisCompleted,
	Binary,
	MetaInfo,
	DialogResultGenerated,
    Close  /*语音功能通道连接关闭*/
};

/**
    * @brief NlsEvent构造函数
    * @param event    NlsEvent对象
    */
NlsEvent(const NlsEvent& event);

/**
    * @brief NlsEvent构造函数
    * @param msg    Event消息字符串
    * @param code   Event状态编码
    * @param type    Event类型
    * @param taskId 任务的task id
    */
NlsEvent(const char * msg, int code, EventType type, std::string & taskId);

/**
    * @brief NlsEvent构造函数
    * @param msg    Event消息字符串
    */
NlsEvent(std::string & msg);

/**
    * @brief 解析消息字符串
    * @note SDK内部函数
    * @return 成功返回0，失败返回-1, 抛出异常
*/
int parseJsonMsg();

/**
    * @brief NlsEvent构造函数
    * @param data    二进制数据
    * @param code   Event状态编码
    * @param type    Event类型
    * @param taskId 任务的task id
    * @return
    */
NlsEvent(std::vector<unsigned char> data, int code, EventType type, std::string taskId);

/**
    * @brief NlsEvent析构函数
    */
~NlsEvent();

/**
    * @brief 获取状态码
    * @note 正常情况为0或者20000000，失败时对应失败的错误码。错误码参考SDK文档说明。
    * @return int
    */
int getStatusCode();

/**
    * @brief 获取云端返回的识别结果
    * @note json格式
    * @return const char*
    */
const char* getAllResponse();

/**
    * @brief 在TaskFailed回调中，获取NlsRequest操作过程中出现失败时的错误信息
    * @note 在Failed回调函数中使用
    * @return const char*
    */
const char* getErrorMessage();

/**
    * @brief 获取任务的task id
    * @return const char*
    */
const char* getTaskId();

/**
    * @brief 获取一句话识别或者实时语音识别的识别结果
    * @return const char*
    */
const char* getResult();

/**
    * @brief 获取实时语音检测的句子编号
    * @note 只有在实时语音检测功能才能获得识别句子的编号
    * @result int
    */
int getSentenceIndex();

/**
    * @brief 获取实时语音检测的句子的音频时长，单位是毫秒
    * @note 只有在实时语音检测功能才能获得识别句子的音频时长
    * @result int
    */
int getSentenceTime();

/**
    * @brief 获取sentence超时状态
    * @note 在实时语音识别SentenceEnd事件回调中使用. 正常返回2000000, 超时返回51040104
    * @result int
    */
int getSentenceTimeOutStatus();

/**
    * @brief 对应的SentenceBegin事件的时间，单位是毫秒
    * @note 在实时语音识别SentenceEnd事件回调中使用
    * @result int
    */
int getSentenceBeginTime();

/**
    * @brief 结果置信度,取值范围[0.0,1.0]，值越大表示置信度越高
    * @note 在实时语音识别SentenceEnd事件回调中使用
    * @result int
    */
double getSentenceConfidence();

/**
    * @brief 本句话中的词信息
    * @note 在实时语音识别SentenceEnd事件回调中使用
    * @result int
    */
std::list<WordInfomation> getSentenceWordsList();

/**
    * @brief 获取云端返回的二进制数据
    * @note 仅用于语音合成功能
    * @return vector<unsigned char>
    */
std::vector<unsigned char> getBinaryData();

/**
    * @brief 获取当前所发生Event的类型
    * @return EventType
    */
EventType getMsgType();

/**
    * @brief 获取用于显示的文本
    * @return const char*
    */
const char* getDisplayText();

/**
    * @brief 获取用于朗读的文本
    * @return const char*
    */
const char* getSpokenText();

/**
    * @brief 服务端确认结果
    * @return const bool
    */
const bool getWakeWordAccepted();

/**
    * @brief 获取stashResult的sentence Id
    * @return id
    */
const int getStashResultSentenceId();

/**
    * @brief 获取stashResult的beginTime
    * @return 下一句的开始时间
    */
const int getStashResultBeginTime();

/**
    * @brief 获取stashResult的CurrentTime
    * @return 当前时间
    */
const int getStashResultCurrentTime();

/**
    * @brief 获取stashResult的text
    * @return 下一句已识别文本
    */
const char* getStashResultText();

private:
int parseMsgType(std::string name);

private:

int _statusCode;
std::string _msg;
EventType _msgType;
std::string _taskId;
std::string _result;
std::string _displayText;
std::string _spokenText;
int _sentenceTimeOutStatus;
int _sentenceIndex;
int _sentenceTime;
int _sentenceBeginTime;
double _sentenceConfidence;
std::list<WordInfomation> _sentenceWordsList;
bool _wakeWordAccepted;
bool _wakeWordKnown;
std::string _wakeWordUserId;
int _wakeWordGender;

std::vector<unsigned char> _binaryData;

int _stashResultSentenceId;
int _stashResultBeginTime;
std::string _stashResultText;
int _stashResultCurrentTime;
};

typedef void (*NlsCallbackMethod)(NlsEvent*, void*);

}

#if defined (_WIN32)
	#pragma warning( pop )
#endif

#endif //NLS_SDK_EVENT_H
