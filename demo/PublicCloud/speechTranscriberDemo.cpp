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

#ifdef _WIN32
#include <windows.h>
#include "pthread.h"
#else

#include <pthread.h>
#include <unistd.h>
#endif

#include <ctime>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "speechTranscriberRequest.h"

#include "nlsCommonSdk/Token.h"

#define FRAME_SIZE 3200
#define SAMPLE_RATE 16000

using std::map;
using std::string;
using std::vector;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;

using namespace AlibabaNlsCommon;

using AlibabaNls::NlsClient;
using AlibabaNls::NlsEvent;
using AlibabaNls::LogDebug;
using AlibabaNls::LogInfo;
using AlibabaNls::SpeechTranscriberCallback;
using AlibabaNls::SpeechTranscriberRequest;

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey Secret重新生成一个token，并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
 */
string g_akId = "";
string g_akSecret = "";
string g_token = "";
long g_expireTime = -1;

// 自定义线程参数
struct ParamStruct {
    string fileName;
    string token;
    string appkey;
};

// 自定义事件回调参数
struct ParamCallBack {
    int iExg;
    string sExg;

    pthread_mutex_t mtx;
    bool bSend;
};

/**
 * 根据AccessKey ID和AccessKey Secret重新生成一个token，并获取其有效期时间戳
 */
int generateToken(string akId, string akSecret, string* token, long* expireTime) {
    NlsToken nlsTokenRequest;
    nlsTokenRequest.setAccessKeyId(akId);
    nlsTokenRequest.setKeySecret(akSecret);

    if (-1 == nlsTokenRequest.applyNlsToken()) {
        cout << "Failed: " << nlsTokenRequest.getErrorMsg() << endl; /*获取失败原因*/

        return -1;
    }

    *token = nlsTokenRequest.getToken();
    *expireTime = nlsTokenRequest.getExpireTime();

    return 0;
}

/**
    * @brief 获取sendAudio发送延时时间
    * @param dataSize 待发送数据大小
    * @param sampleRate 采样率 16k/8K
    * @param compressRate 数据压缩率，例如压缩比为10:1的编码，此时为10；非压缩数据则为1
    * @return 返回sendAudio之后需要sleep的时间
    * @note 对于8k pcm 编码数据, 16位采样，建议每发送1600字节 sleep 100 ms.
            对于16k pcm 编码数据, 16位采样，建议每发送3200字节 sleep 100 ms.
            对于其它编码格式的数据, 用户根据压缩比, 自行估算, 比如压缩比为10:1的编码,
            需要每发送3200/10=320 sleep 100ms.
*/
unsigned int getSendAudioSleepTime(const int dataSize,
                                   const int sampleRate,
                                   const int compressRate) {
    // 仅支持16位采样
    const int sampleBytes = 16;
    // 仅支持单通道
    const int soundChannel = 1;

    // 当前采样率，采样位数下每秒采样数据的大小
    int bytes = (sampleRate * sampleBytes * soundChannel) / 8;

    // 当前采样率，采样位数下每毫秒采样数据的大小
    int bytesMs = bytes / 1000;

    // 待发送数据大小除以每毫秒采样数据大小，以获取sleep时间
    int sleepMs = (dataSize * compressRate) / bytesMs;

    return sleepMs;
}

/**
    * @brief 调用start(), 成功与云端建立连接, sdk内部线程上报started事件
    * @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTranscriptionStarted(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
	cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; // 仅表示自定义参数示例

	cout << "onTranscriptionStarted: "
		<< "status code: " << cbEvent->getStausCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< endl;
	// cout << "onTranscriptionStarted: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 服务端检测到了一句话的开始, sdk内部线程上报SentenceBegin事件
    * @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onSentenceBegin(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
	cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; // 仅表示自定义参数示例

	cout << "onSentenceBegin: "
		<< "status code: " << cbEvent->getStausCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< ", index: " << cbEvent->getSentenceIndex() //句子编号，从1开始递增
		<< ", time: " << cbEvent->getSentenceTime() //当前已处理的音频时长，单位是毫秒
		<< endl;
	// cout << "onSentenceBegin: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 服务端检测到了一句话结束, sdk内部线程上报SentenceEnd事件
    * @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onSentenceEnd(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
	cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; // 仅表示自定义参数示例

	cout << "onSentenceEnd: "
		<< "status code: " << cbEvent->getStausCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< ", result: " << cbEvent->getResult()    // 当前句子的完成识别结果
		<< ", index: " << cbEvent->getSentenceIndex()  // 当前句子的索引编号
		<< ", time: " << cbEvent->getSentenceTime()    // 当前句子的音频时长
        << ", begin_time: " << cbEvent->getSentenceBeginTime() // 对应的SentenceBegin事件的时间
        << ", confidence: " << cbEvent->getSentenceConfidence()    // 结果置信度,取值范围[0.0,1.0]，值越大表示置信度越高
		<< endl;
	// cout << "onSentenceEnd: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 识别结果发生了变化, sdk在接收到云端返回到最新结果时, sdk内部线程上报ResultChanged事件
    * @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTranscriptionResultChanged(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
	cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; // 仅表示自定义参数示例

	cout << "onTranscriptionResultChanged: "
		<< "status code: " << cbEvent->getStausCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< ", result: " << cbEvent->getResult()    // 当前句子的中间识别结果
		<< ", index: " << cbEvent->getSentenceIndex()  // 当前句子的索引编号
		<< ", time: " << cbEvent->getSentenceTime()    // 当前句子的音频时长
		<< endl;
	// cout << "onTranscriptionResultChanged: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 服务端停止实时音频流识别时, sdk内部线程上报Completed事件
    * @note 上报Completed事件之后，SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送.
    *       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTranscriptionCompleted(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
	cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; // 仅表示自定义参数示例

	cout << "onTranscriptionCompleted: "
		<< "status code: " << cbEvent->getStausCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< endl;
	// cout << "onTranscriptionCompleted: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 识别过程(包含start(), send(), stop())发生异常时, sdk内部线程上报TaskFailed事件
    * @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTaskFailed(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
	cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; // 仅表示自定义参数示例

	cout << "onTaskFailed: "
		<< "status code: " << cbEvent->getStausCode()
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< ", error message: " << cbEvent->getErrorMessage()
		<< endl;
	// cout << "onTaskFailed: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息

    /*设置发送状态位, 停止数据发送. */
    pthread_mutex_lock(&(tmpParam->mtx));
    tmpParam->bSend = false;
    pthread_mutex_unlock(&(tmpParam->mtx));
}

/**
    * @brief 识别结束或发生异常时，会关闭连接通道, sdk内部线程上报ChannelCloseed事件
    * @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onChannelClosed(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
	cout << "CbParam: " << tmpParam->iExg << " " << tmpParam->sExg << endl; //仅表示自定义参数示例

	cout << "OnChannelCloseed: All response: " << cbEvent->getAllResponse() << endl; // getResponse() 可以通道关闭信息
}

// 工作线程
void* pthreadFunc(void* arg) {
    bool tmpStatus = true;
    int sleepMs = 0;
    ParamCallBack cbParam;
    SpeechTranscriberCallback* callback = NULL;

    // 初始化自定义回调参数, 以下两变量仅作为示例表示参数传递, 在demo中不起任何作用
    cbParam.iExg = 1;
    cbParam.sExg = "exg.";

    //控制发送流程
    pthread_mutex_init(&(cbParam.mtx), NULL);
    cbParam.bSend = true;

    // 0: 从自定义线程参数中获取token, 配置文件等参数.
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

    /*
     * 1: 创建并设置回调函数
     */
    callback = new SpeechTranscriberCallback();
    callback->setOnTranscriptionStarted(onTranscriptionStarted, &cbParam); // 设置识别启动回调函数
    callback->setOnTranscriptionResultChanged(onTranscriptionResultChanged, &cbParam); // 设置识别结果变化回调函数
    callback->setOnTranscriptionCompleted(onTranscriptionCompleted, &cbParam); // 设置语音转写结束回调函数
    callback->setOnSentenceBegin(onSentenceBegin, &cbParam); // 设置一句话开始回调函数
    callback->setOnSentenceEnd(onSentenceEnd, &cbParam); // 设置一句话结束回调函数
    callback->setOnTaskFailed(onTaskFailed, &cbParam); // 设置异常识别回调函数
    callback->setOnChannelClosed(onChannelClosed, &cbParam); // 设置识别通道关闭回调函数

    /*
    * 创建实时音频流识别SpeechTranscriberRequest对象, 参数为callback对象.
    * request对象在一个会话周期内可以重复使用.
    * 会话周期是一个逻辑概念. 比如Demo中, 指读取, 发送完整个音频文件数据的时间.
    * 音频文件数据发送结束时, 可以releaseTranscriberRequest()释放对象.
    * createTranscriberRequest(), start(), sendAudio(), stop(), releaseTranscriberRequest()请在
    * 同一线程内完成, 跨线程使用可能会引起异常错误.
    */
    /*
     * 2: 创建实时音频流识别SpeechTranscriberRequest对象
     */
    SpeechTranscriberRequest* request = NlsClient::getInstance()->createTranscriberRequest(callback);
    if (request == NULL) {
        cout << "createTranscriberRequest failed." << endl;

        delete callback;
        callback = NULL;

        return NULL;
    }

	request->setAppKey(tst->appkey.c_str()); // 设置AppKey, 必填参数, 请参照官网申请
	request->setFormat("pcm"); // 设置音频数据编码格式, 可选参数，目前支持pcm, opus, speex. 默认是pcm
	request->setSampleRate(SAMPLE_RATE); // 设置音频数据采样率, 可选参数，目前支持16000, 8000. 默认是16000
	request->setIntermediateResult(false); // 设置是否返回中间识别结果, 可选参数. 默认false
	request->setPunctuationPrediction(false); // 设置是否在后处理中添加标点, 可选参数. 默认false
	request->setInverseTextNormalization(false); // 设置是否在后处理中执行数字转写, 可选参数. 默认false
    request->setSemanticSentenceDetection(false); // 设置是否语义断句, 可选参数. 默认false
    request->setMaxSentenceSilence(300);
    request->setToken(tst->token.c_str()); // 设置账号校验token, 必填参数

    /*
    * 3: start()为阻塞操作, 发送start指令之后, 会等待服务端响应, 或超时之后才返回
    */
    if (request->start() < 0) {
        cout << "start() failed." << endl;
        NlsClient::getInstance()->releaseTranscriberRequest(request); // start()失败，释放request对象

        delete callback;
        callback = NULL;

        return NULL;
    }

    // 文件是否读取完毕, 或者接收到TaskFailed, closed, completed回调, 终止send
    while ((!fs.eof()) && (tmpStatus)) {
        char data[FRAME_SIZE] = {0};

        fs.read(data, sizeof(char) * FRAME_SIZE);
        int nlen = (int)fs.gcount();

        /*
        * 4: 发送音频数据. sendAudio返回-1表示发送失败, 需要停止发送. 对于第三个参数, 默认使用false.
        */
        nlen = request->sendAudio(data, nlen, false);
        if (nlen < 0) {
            // 发送失败, 退出循环数据发送
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
        sleepMs = getSendAudioSleepTime(nlen, SAMPLE_RATE, 1); // 根据 发送数据大小，采样率，数据压缩比 来获取sleep时间

        /*
        * 5: 语音数据发送延时控制
        */
#ifdef _WIN32
        Sleep(sleepMs);
#else
        usleep(sleepMs * 1000);
#endif

        pthread_mutex_lock(&(cbParam.mtx));
        tmpStatus = cbParam.bSend;
        pthread_mutex_unlock(&(cbParam.mtx));
    }

    // 关闭音频文件
    fs.close();

    /*
    * 6: 数据发送结束，关闭识别连接通道.
    * stop()为阻塞操作, 在接受到服务端响应, 或者超时之后, 才会返回.
    */
    request->stop();
	/*
    * 7: 识别结束, 释放request对象
	*/
    NlsClient::getInstance()->releaseTranscriberRequest(request);

    /*
	* 8: 释放callback对象
	*/
    delete callback;
    callback = NULL;

    return NULL;
}

/**
 * 识别单个音频数据
 */
int speechTranscriberFile(const char* appkey) {
    /**
     * 获取当前系统时间戳，判断token是否过期
     */
    std::time_t curTime = std::time(0);
    if (g_expireTime - curTime < 10) {
        cout << "the token will be expired, please generate new token by AccessKey-ID and AccessKey-Secret." << endl;
        if (-1 == generateToken(g_akId, g_akSecret, &g_token, &g_expireTime)) {
            return -1;
        }
    }

    ParamStruct pa;
    pa.token = g_token;
    pa.appkey = appkey;
    pa.fileName = "test0.wav";

    pthread_t pthreadId;

    // 启动一个工作线程, 用于识别
    pthread_create(&pthreadId, NULL, &pthreadFunc, (void *)&pa);

    pthread_join(pthreadId, NULL);
	
	return 0;

}

/**
 * 识别多个音频数据;
 * sdk多线程指一个音频数据对应一个线程, 非一个音频数据对应多个线程.
 * 示例代码为同时开启4个线程识别4个文件;
 * 免费用户并发连接不能超过10个;
 */
#define AUDIO_FILE_NUMS 4
#define AUDIO_FILE_NAME_LENGTH 32
int speechTranscriberMultFile(const char* appkey) {
    /**
     * 获取当前系统时间戳，判断token是否过期
     */
    std::time_t curTime = std::time(0);
    if (g_expireTime - curTime < 10) {
        cout << "the token will be expired, please generate new token by AccessKey-ID and AccessKey-Secret." << endl;
        if (-1 == generateToken(g_akId, g_akSecret, &g_token, &g_expireTime)) {
            return -1;
        }
    }

    char audioFileNames[AUDIO_FILE_NUMS][AUDIO_FILE_NAME_LENGTH] = {"test0.wav", "test1.wav", "test2.wav", "test3.wav"};
    ParamStruct pa[AUDIO_FILE_NUMS];

    for (int i = 0; i < AUDIO_FILE_NUMS; i ++) {
        pa[i].token = g_token;
        pa[i].appkey = appkey;
        pa[i].fileName = audioFileNames[i];
    }

    vector<pthread_t> pthreadId(AUDIO_FILE_NUMS);
    // 启动四个工作线程, 同时识别四个音频文件
    for (int j = 0; j < AUDIO_FILE_NUMS; j++) {
        pthread_create(&pthreadId[j], NULL, &pthreadFunc, (void *)&(pa[j]));
    }

    for (int j = 0; j < AUDIO_FILE_NUMS; j++) {
        pthread_join(pthreadId[j], NULL);
    }

	return 0;
}

int main(int arc, char* argv[]) {
    if (arc < 4) {
        cout << "params is not valid. Usage: ./demo <your appkey> <your AccessKey ID> <your AccessKey Secret>" << endl;
        return -1;
    }

    string appkey = argv[1];
    g_akId = argv[2];
    g_akSecret = argv[3];

    // 根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-Transcriber.txt， LogDebug表示输出所有级别日志
    int ret = NlsClient::getInstance()->setLogConfig("log-transcriber.txt", LogInfo);
    if (-1 == ret) {
        cout << "set log failed." << endl;
        return -1;
    }

    // 识别单个音频数据
    speechTranscriberFile(appkey.c_str());

    // 识别多个音频数据
    // speechTranscriberMultFile(appkey.c_str());

    // 所有工作完成，进程退出前，释放nlsClient. 请注意, releaseInstance()非线程安全.
    NlsClient::releaseInstance();

    return 0;
}
