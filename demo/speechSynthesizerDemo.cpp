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

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include "pthread.h"
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <functional>
#include <exception>
#include <algorithm>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "speechSynthesizerRequest.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <algorithm>
#include <unistd.h>
using std::min;
#endif

#define FRAME_SIZE 6400

using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::ios;
using std::exception;

/*自定义线程参数*/
struct ParamStruct {
	string text;
	string config;
	string token;
    string audioFile;
};

/*自定义事件回调参数*/
struct ParamCallBack {
    string binAudioFile;
	ofstream audioFile;
};

/**
    * @brief 调用start(), 发送text至云端, sdk内部线程上报started事件
    * @note 不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnSynthesisStarted(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

	cout << "CbParam: " << tmpParam->binAudioFile << endl; /*仅表示自定义参数示例*/

	cout << "OnRecognitionStarted: " << cbEvent->getResponse() << endl; /*getResponse() 可以获取云端响应信息*/
}

/**
    * @brief sdk在接收到云端返回合成结束消息时, sdk内部线程上报Completed事件
    * @note 上报Completed事件之后，SDK内部会关闭识别连接通道.
    * 		不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnSynthesisCompleted(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->binAudioFile << endl; /*仅表示自定义参数示例*/

	cout << "OnRecognitionCompleted: " << cbEvent->getResponse() << endl; /*getResponse() 可以获取云端响应信息*/
}

/**
    * @brief 合成过程发生异常时, sdk内部线程上报TaskFailed事件
    * @note 上报TaskFailed事件之后，SDK内部会关闭识别连接通道.
    * 		不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnSynthesisTaskFailed(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->binAudioFile << endl; /*仅表示自定义参数示例*/

	cout << "OnRecognitionTaskFailed: " << cbEvent->getErrorMessage() << endl; /*getErrorMessage() 可以获取异常失败信息*/
}

/**
    * @brief 识别结束或发生异常时，会关闭连接通道, sdk内部线程上报ChannelCloseed事件
    * @note 不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnSynthesisChannelClosed(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->binAudioFile << endl; /*仅表示自定义参数示例*/

	cout << "OnRecognitionChannelCloseed: " << cbEvent->getResponse() << endl; /*getResponse() 可以获取关闭信息*/
}

/**
    * @brief 文本上报服务端之后, 收到服务端返回的二进制音频数据, SDK内部线程通过BinaryDataRecved事件上报给用户
    * @note 不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnBinaryDataRecved(NlsEvent* cbEvent, void* para) {
    ParamCallBack* tmpParam = (ParamCallBack*)para;

    cout << "CbParam: " << tmpParam->binAudioFile << endl; /*仅表示自定义参数示例*/

    vector<unsigned char> data = cbEvent->getBinaryData(); /*getBinaryData() 获取文本合成的二进制音频数据*/

    std::cout << "OnBinaryDataRecved: " << data.size() << endl;

    /*以追加形式将二进制音频数据写入文件*/
    if (data.size() > 0) {
        tmpParam->audioFile.write((char*)&data[0], data.size());
    }

}

/*工作线程*/
void* pthreadFunc(void* arg) {
	int sleepMs = 0;
	ParamCallBack cbParam;
	SpeechSynthesizerCallback* callback = NULL;

	/*0: 从自定义线程参数中获取token, 配置文件等参数.*/
	ParamStruct* tst = (ParamStruct*)arg;
	if (tst == NULL) {
		cout << "arg is not valid." << endl;
		return NULL;
	}

	/*初始化自定义回调参数*/
	cbParam.binAudioFile = tst->audioFile;
	cbParam.audioFile.open(cbParam.binAudioFile.c_str(), ios::binary | ios::out);

	/*
     * 1: 创建并设置回调函数
     */
	callback = new SpeechSynthesizerCallback();
	callback->setOnSynthesisStarted(OnSynthesisStarted, &cbParam); /*设置音频合成启动成功回调函数*/
	callback->setOnSynthesisCompleted(OnSynthesisCompleted, &cbParam); /*设置音频合成结束回调函数*/
	callback->setOnChannelClosed(OnSynthesisChannelClosed, &cbParam); /*设置音频合成通道关闭回调函数*/
	callback->setOnTaskFailed(OnSynthesisTaskFailed, &cbParam); /*设置异常失败回调函数*/
    callback->setOnBinaryDataReceived(OnBinaryDataRecved, &cbParam); /*设置文本音频数据接收回调函数*/

	/***************以读取config.txt方式创建request****************/
	/*
    * 创建语音识别SpeechSynthesizerRequest对象, 第一个参数为callback对象, 第二个参数为config.txt文件.
    * 目前SynthesizerRequest对象在调用一次start()，发送完一个text文本之后，必须调用stop()并releaseSynthesizerRequest()释放对象
    */
	/*
     * 2: 创建语音识别SpeechSynthesizerRequest对象
     */
	SpeechSynthesizerRequest* request = NlsClient::getInstance()->createSynthesizerRequest(callback, tst->config.c_str());
	if (request == NULL) {
		cout << "createSynthesizerRequest failed." << endl;
        delete callback;
        callback = NULL;
		return NULL;
	}

	/*****************以参数设置方式创建request******************/
//	SpeechSynthesizerRequest* request = NlsClient::getInstance()->createSynthesizerRequest(callback, NULL);
//	if (request == NULL) {
//		std::cout << "createNlsRequest fail" << endl;
//
//        delete callback;
//        callback = NULL;
//
//		return NULL;
//	}
//
//	request->setUrl("wss://nls-gateway.cn-shanghai.aliyuncs.com/ws/v1"); /*设置服务端url, 必填参数*/
//	request->setAppKey("your-appkey"); /*设置AppKey, 必填参数, 请参照官网申请*/
//	request->setText(tst->text.c_str()); /*设置待合成文本, 必填参数. */
//    request->setVoice("xiaoyun"); /*发音人, 包含"xiaoyun", "xiaogang". 可选参数, 默认是xiaoyun */
//    request->setVolume(50); /*音量, 范围是0~100, 可选参数, 默认50 */
//    request->setFormat("pcm"); /*音频编码格式, 可选参数, 默认是pcm. 支持的格式pcm, wav, mp3 */
//    request->setSampleRate(16000); /*音频采样率, 包含8000, 16000. 可选参数, 默认是16000 */
//    request->setSpeechRate(0); /*语速, 范围是-500~500, 可选参数, 默认是0 */
//    request->setPitchRate(0); /*语调, 范围是-500~500, 可选参数, 默认是0 */
//    request->setMethod(0); /*合成方法, 可选参数, 默认是0. 参数含义0:不带录音的参数合成; 1:带录音的拼接合成; 2:不带录音的拼接合成; 3:带录音的参数合成 */

	request->setToken(tst->token.c_str()); /*设置账号校验token, 必填参数*/

	/*
    * 3: start()为阻塞操作, 发送start指令之后, 会等待服务端响应, 或超时之后才返回.
    * 调用start()之后, 文本被发送至云端. SDk接到云端返回的合成音频数据，会通过OnBinaryDataRecved回调函数上报至用户进程.
    */
	if (request->start() < 0) {
		cout << "start() failed." << endl;
		NlsClient::getInstance()->releaseSynthesizerRequest(request); /*start()失败，释放request对象*/

        delete callback;
        callback = NULL;

		return NULL;
	}

	/*
    *6: start()返回之后，关闭识别连接通道.
    *stop()为阻塞操作, 在接受到服务端响应, 或者超时之后, 才会返回.
    *
    */
	request->stop();

	cbParam.audioFile.close();

	/*7: 识别结束, 释放request对象*/
	NlsClient::getInstance()->releaseSynthesizerRequest(request);

	/*8: 释放callback对象*/
    delete callback;
	callback = NULL;

	return NULL;
}

/**
 * 合成单个文本数据
 */
int speechSynthesizerFile(const char* configFile, const char* token) {

	ParamStruct pa;
	pa.config = configFile;
	pa.token = token;

	/*注意: VS2015编译时, 可能会将汉字编译为乱码. 导致合成失败. 请使用英文或配置文件形式测试.*/
	pa.text = "中华人民共和国万岁, 万岁, 万万岁";
    pa.audioFile = "syAudio.wav";

	pthread_t pthreadId;

	/*启动一个工作线程, 用于识别*/
	pthread_create(&pthreadId, NULL, &pthreadFunc, (void *)&pa);

	pthread_join(pthreadId, NULL);
	
	return 0;

}

/**
 * 合成多个文本数据;
 * sdk多线程指一个文本数据对应一个线程, 非一个文本数据对应多个线程.
 * 示例代码为同时开启4个线程合成4个文件;
 * 免费用户并发连接不能超过10个;
 */
#define AUDIO_TEXT_NUMS 4
#define AUDIO_TEXT_LENGTH 64
#define AUDIO_FILE_NAME_LENGTH 32
int speechSynthesizerMultFile(const char* configFile, const char* token) {

    const char syAudioFiles[AUDIO_TEXT_NUMS][AUDIO_FILE_NAME_LENGTH] = {"syAudio0.wav", "syAudio1.wav", "syAudio2.wav", "syAudio3.wav"};
	const char texts[AUDIO_TEXT_NUMS][AUDIO_TEXT_LENGTH] = {"今日天气真不错，我想去操作踢足球",
								  "明天有大暴雨，还是宅在家里看电影吧",
								  "前天天气非常好，可惜没有去运动",
                                  "每天都吃这么多肉，你会胖成猪的"};
	ParamStruct pa[AUDIO_TEXT_NUMS];

	for (int i = 0; i < AUDIO_TEXT_NUMS; i ++) {
		pa[i].config = configFile;
		pa[i].token = token;
		pa[i].text = texts[i];
        pa[i].audioFile = syAudioFiles[i];
	}

	vector<pthread_t> pthreadId(AUDIO_TEXT_NUMS);
	/*启动四个工作线程, 同时识别四个音频文件*/
	for (int j = 0; j < AUDIO_TEXT_NUMS; j++) {
		pthread_create(&pthreadId[j], NULL, &pthreadFunc, (void *)&(pa[j]));
	}

	for (int j = 0; j < AUDIO_TEXT_NUMS; j++) {
		pthread_join(pthreadId[j], NULL);
	}

	return 0;

}

int main(int arc, char* argv[]) {
	if (arc < 3) {
		cout << "params is not valid. Usage: ./demo config.txt your_token" << endl;
		return -1;
	}

	/*根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-Synthesizer.txt， LogDebug表示输出所有级别日志*/
	int ret = NlsClient::getInstance()->setLogConfig("log-synthesizer.txt", LogInfo);
	if (-1 == ret) {
		cout << "set log failed." << endl;
		return -1;
	}

	/*合成单个文本*/
	speechSynthesizerFile(argv[1], argv[2]);

	/*合成多个文本*/
//    speechSynthesizerMultFile(argv[1], argv[2]);

	/*所有工作完成，进程退出前，释放nlsClient. 请注意, releaseInstance()非线程安全.*/
	NlsClient::releaseInstance();

	return 0;
}
