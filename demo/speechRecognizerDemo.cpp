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
#include "speechRecognizerRequest.h"

#if defined(_WIN32)
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
using std::ios;
//using std::min;
using std::exception;

/*自定义线程参数*/
struct ParamStruct {
	string fileName;
	string config;
	string token;
};

/*自定义事件回调参数*/
struct ParamCallBack {
    int iExg;
    string sExg;
};

/**
    * @brief 获取sendAudio发送延时时间
    * @param dataSize 待发送数据大小
    * @param sampleRate 采样率 16k/8K
    * @param compressRate 数据压缩率，例如压缩比为10:1的16k opus编码，此时为10；非压缩数据则为1
    * @return 返回sendAudio之后需要sleep的时间
    * @note 对于8k pcm 编码数据, 16位采样，建议每发送3200字节 sleep 200 ms.
            对于16k pcm 编码数据, 16位采样，建议每发送6400字节 sleep 200 ms.
            对于其它编码格式的数据, 用户根据压缩比, 自行估算, 比如压缩比为10:1的 16k opus,
            需要每发送6400/10=640 sleep 200ms.
*/
unsigned int getSendAudioSleepTime(const int dataSize,
                                   const int sampleRate,
                                   const int compressRate) {
    /*仅支持16位采样*/
    const int sampleBytes = 16;
    /*仅支持单通道*/
    const int soundChannel = 1;

    /*当前采样率，采样位数下每秒采样数据的大小*/
    int bytes = (sampleRate * sampleBytes * soundChannel) / 8;

    /*当前采样率，采样位数下每毫秒采样数据的大小*/
    int bytesMs = bytes / 1000;

    /*待发送数据大小除以每毫秒采样数据大小，以获取sleep时间*/
    int sleepMs = (dataSize * compressRate) / bytesMs;

    return sleepMs;
}

/**
    * @brief 调用start(), 成功与云端建立连接, sdk内部线程上报started事件
    * @note 不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnRecognitionStarted(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; /*仅表示自定义参数示例*/

    cout << "OnRecognitionStarted: " << cbEvent->getResponse() << endl; /*getResponse() 可以获取云端响应信息*/
}

/**
    * @brief 设置允许返回中间结果参数, sdk在接收到云端返回到中间结果时, sdk内部线程上报ResultChanged事件
    * @note 不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnRecognitionResultChanged(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; /*仅表示自定义参数示例*/

    cout << "OnRecognitionResultChanged: " << cbEvent->getResponse() << endl; /*getResponse() 可以获取中间结果*/
}

/**
    * @brief sdk在接收到云端返回识别结束消息时, sdk内部线程上报Completed事件
    * @note 上报Completed事件之后, SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送.
    *       不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常.
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnRecognitionCompleted(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; /*仅表示自定义参数示例*/

    cout << "OnRecognitionCompleted: " << cbEvent->getResponse() << endl; /*getResponse() 可以获取最终识别结果*/
}

/**
    * @brief 识别过程(包含start(), send(), stop())发生异常时, sdk内部线程上报TaskFailed事件
    * @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送.
    *       不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnRecognitionTaskFailed(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; /*仅表示自定义参数示例*/

    cout << "OnRecognitionTaskFailed: " << cbEvent->getErrorMessage() << endl; /*getErrorMessage() 可以获取异常失败信息*/
}

/**
    * @brief 识别结束或发生异常时，会关闭连接通道, sdk内部线程上报ChannelCloseed事件
    * @note 不允许在回调函数内部调用stop(), releaseRecognizerRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void OnRecognitionChannelCloseed(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; /*仅表示自定义参数示例*/

    cout << "OnRecognitionChannelCloseed: " << cbEvent->getResponse() << endl; /*getResponse() 可以通道关闭信息*/
}

/*工作线程*/
void* pthreadFunc(void* arg) {
    int sleepMs = 0;
    ParamCallBack cbParam;
    SpeechRecognizerCallback* callback = NULL;

    /*0: 从自定义线程参数中获取token, 配置文件等参数.*/
    ParamStruct* tst = (ParamStruct*)arg;
    if (tst == NULL) {
        cout << "arg is not valid." << endl;
        return NULL;
    }

    /* 打开音频文件, 获取数据 */
    ifstream fs;
    fs.open(tst->fileName.c_str(), ios::binary | ios::in);
    if (!fs) {
        cout << tst->fileName << " isn't exist.." << endl;

        return NULL;
    }

    /*初始化自定义回调参数, 仅作为示例表示参数传递, 在demo中不起任何作用*/
    cbParam.iExg = 1;
    cbParam.sExg = "exg.";

    /*
     * 1: 创建并设置回调函数
     */
    callback = new SpeechRecognizerCallback();
    callback->setOnRecognitionStarted(OnRecognitionStarted, &cbParam); /*设置start()成功回调函数*/
    callback->setOnTaskFailed(OnRecognitionTaskFailed, &cbParam); /*设置异常识别回调函数*/
    callback->setOnChannelClosed(OnRecognitionChannelCloseed, &cbParam); /*设置识别通道关闭回调函数*/
    callback->setOnRecognitionResultChanged(OnRecognitionResultChanged, &cbParam); /*设置中间结果回调函数*/
    callback->setOnRecognitionCompleted(OnRecognitionCompleted, &cbParam); /*设置识别结束回调函数*/

    /***************以读取config.txt方式创建request****************/
    /*
    * 创建一句话识别SpeechRecognizerRequest对象, 第一个参数为callback对象, 第二个参数为config.txt文件.
    * request对象在一个会话周期内可以重复使用.
    * 会话周期是一个逻辑概念. 比如Demo中, 指读取, 发送完整个音频文件数据的时间.
    * 音频文件数据发送结束时, 可以releaseRecognizerRequest()释放对象.
    * createRecognizerRequest(), start(), sendAudio(), stop(), releaseRecognizerRequest()请在
    * 同一线程内完成, 跨线程使用可能会引起异常错误。
	* 如果需要识别多次，请每次创建一个SpeechRecognizerRequest请求，执行start-sendAudio-stop,然后释放SpeechRecognizerRequest请求。
    */
    /*
     * 2: 创建一句话识别SpeechRecognizerRequest对象
     */
    SpeechRecognizerRequest* request = NlsClient::getInstance()->createRecognizerRequest(callback, tst->config.c_str());
    if (request == NULL) {
	    cout << "createRecognizerRequest failed." << endl;

        delete callback;
        callback = NULL;

		return NULL;
    }

    /*****************以参数设置方式创建request******************/
//	SpeechRecognizerRequest* request = NlsClient::getInstance()->createRecognizerRequest(callback, NULL);
//	if (request == NULL) {
//		std::cout << "createNlsRequest fail" << endl;
//		return NULL;
//	}
//
//	request->setUrl("wss://nls-gateway.cn-shanghai.aliyuncs.com/ws/v1"); /*设置服务端url, 必填参数*/
//	request->setAppKey("your-appkey"); /*设置AppKey, 必填参数, 请参照官网申请*/
//	request->setFormat("pcm"); /*设置音频数据编码格式, 可选参数, 目前支持pcm, opus, opu. 默认是pcm*/
//	request->setSampleRate(16000); /*设置音频数据采样率, 可选参数, 目前支持16000, 8000. 默认是16000*/
//	request->setIntermediateResult("false"); /*设置是否返回中间识别结果, 可选参数. 默认false*/
//	request->setPunctuationPrediction("false"); /*设置是否在后处理中添加标点, 可选参数. 默认false*/
//	request->setInverseTextNormalization("false"); /*设置是否在后处理中执行ITN, 可选参数. 默认false*/

    request->setToken(tst->token.c_str()); /*设置账号校验token, 必填参数*/

    /*
    * 3: start()为阻塞操作, 发送start指令之后, 会等待服务端响应, 或超时之后才返回
    */
    if (request->start() < 0) {
        cout << "start() failed." << endl;
        NlsClient::getInstance()->releaseRecognizerRequest(request); /*start()失败，释放request对象*/

        delete callback;
        callback = NULL;

        return NULL;
    }

    /*文件是否读取完毕, 或者接收到TaskFailed, closed, completed回调, 终止send*/
    while (!fs.eof()) {
        char data[FRAME_SIZE] = {0};

        fs.read(data, sizeof(char) * FRAME_SIZE);
        int nlen = fs.gcount();

        /*
        * 4: 发送音频数据. sendAudio返回-1表示发送失败, 需要停止发送. 对于第三个参数:
        * request对象format参数为pcm时, 使用false即可. format为opu, 使用压缩数据时, 需设置为true.
        */
        nlen = request->sendAudio(data, nlen, false);
        if (nlen < 0) {
            /*发送失败, 退出循环数据发送*/
            cout << "send data fail." << endl;
            break;
        } else {
            cout << "send len:" << nlen << " ." << endl;
        }

        /*
        *语音数据发送控制：
        *语音数据是实时的, 不用sleep控制速率, 直接发送即可.
        *语音数据来自文件, 发送时需要控制速率, 使单位时间内发送的数据大小接近单位时间原始语音数据存储的大小.
        */
        sleepMs = getSendAudioSleepTime(6400, 16000, 1); /*根据 发送数据大小，采样率，数据压缩比 来获取sleep时间*/

        /*
        * 5: 语音数据发送延时控制
        */
#if defined(_WIN32)
        Sleep(sleepMs);
#else
        usleep(sleepMs * 1000);
#endif
    }

    /* 关闭音频文件 */
    fs.close();

    /*
    *6: 数据发送结束，关闭识别连接通道.
    *stop()为阻塞操作, 在接受到服务端响应, 或者超时之后, 才会返回.
    */
    request->stop();

    /*7: 识别结束, 释放request对象*/
    NlsClient::getInstance()->releaseRecognizerRequest(request);

    /*8: 释放callback对象*/
    delete callback;
    callback = NULL;

	return NULL;
}

/** 
  * 线程循环识别
  * 需要调整count值和每次要识别的文件，Demo中默认每次识别一个文件
  */
void* multiRecognize(void* arg) {
	int count = 2;
	while (count > 0) {
		pthreadFunc(arg);
		count--;
	}

	return NULL;
}

/**
 * 识别单个音频数据
 */
int speechRecognizerFile(const char* configFile, const char* token) {

    ParamStruct pa;
    pa.config = configFile;
    pa.token = token;
    pa.fileName = "test0.wav";

    pthread_t pthreadId;

    /*启动一个工作线程, 用于单次识别*/
    pthread_create(&pthreadId, NULL, &pthreadFunc, (void *)&pa);

	/*启动一个工作线程, 用于循环识别*/
	// pthread_create(&pthreadId, NULL, &multiRecognize, (void *)&pa);

    pthread_join(pthreadId, NULL);
	
	return 0;

}

/**
 * 识别多个音频数据;
 * sdk多线程指一个音频数据源对应一个线程, 非一个音频数据对应多个线程.
 * 示例代码为同时开启4个线程识别4个文件;
 * 免费用户并发连接不能超过10个;
 */
#define AUDIO_FILE_NUMS 4
#define AUDIO_FILE_NAME_LENGTH 32
int speechRecognizerMultFile(const char* configFile, const char* token) {

    char audioFileNames[AUDIO_FILE_NUMS][AUDIO_FILE_NAME_LENGTH] = {"test0.wav", "test1.wav", "test2.wav", "test3.wav"};
    ParamStruct pa[AUDIO_FILE_NUMS];

    for (int i = 0; i < AUDIO_FILE_NUMS; i ++) {
        pa[i].config = configFile;
        pa[i].token = token;
        pa[i].fileName = audioFileNames[i];
    }

    vector<pthread_t> pthreadId(AUDIO_FILE_NUMS);
    /*启动四个工作线程, 同时识别四个音频文件*/
    for (int j = 0; j < AUDIO_FILE_NUMS; j++) {
        pthread_create(&pthreadId[j], NULL, &pthreadFunc, (void *)&(pa[j]));
    }

    for (int j = 0; j < AUDIO_FILE_NUMS; j++) {
        pthread_join(pthreadId[j], NULL);
    }

	return 0;
}

int main(int arc, char* argv[]) {
	if (arc < 3) {
        cout << "params is not valid. Usage: ./demo config.txt your_token" << endl;
        return -1;
    }

    /*根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-recognizer.txt， LogDebug表示输出所有级别日志*/
    int ret = NlsClient::getInstance()->setLogConfig("log-recognizer.txt", LogInfo);
    if (-1 == ret) {
        cout << "set log failed." << endl;
        return -1;
    }

    /*识别单个音频数据*/
    speechRecognizerFile(argv[1], argv[2]);

    /*识别多个音频数据*/
    //speechRecognizerMultFile(argv[1], argv[2]);

    /*所有工作完成，进程退出前，释放nlsClient. 请注意, releaseInstance()非线程安全.*/
    NlsClient::releaseInstance();

	return 0;
}
