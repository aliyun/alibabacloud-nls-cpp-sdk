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

#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
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
using AlibabaNls::LogError;
using AlibabaNls::SpeechTranscriberRequest;

// 自定义线程参数
struct ParamStruct {
    string fileName;
    string token;
    string appkey;

    unsigned long long startedConsumed;
    unsigned long long completedConsumed;
    unsigned long long closeConsumed;

    unsigned long long failedConsumed;
    unsigned long long requestConsumed;

    unsigned long long startTotalValue;
    unsigned long long startAveValue;
    unsigned long long startMaxValue;
    unsigned long long startMinValue;

    unsigned long long endTotalValue;
    unsigned long long endAveValue;
    unsigned long long endMaxValue;
    unsigned long long endMinValue;

    unsigned long long s50Value;
    unsigned long long s100Value;
    unsigned long long s200Value;
    unsigned long long s500Value;
    unsigned long long s1000Value;
    unsigned long long s2000Value;

    pthread_mutex_t  mtx;
};

// 自定义事件回调参数
class ParamCallBack {
public:
    ParamCallBack(ParamStruct* param) {
        userId = 1234;
        userInfo = "User.";
        tParam = param;

        pthread_mutex_init(&mtxWord, NULL);
        pthread_cond_init(&cvWord, NULL);
    };
    ~ParamCallBack() {
        tParam = NULL;
        pthread_mutex_destroy(&mtxWord);
        pthread_cond_destroy(&cvWord);
    };

    int userId;
    string userInfo;

    pthread_mutex_t  mtxWord;
    pthread_cond_t  cvWord;

    struct timeval startTv;
    struct timeval startedTv;
    struct timeval completedTv;
    struct timeval closedTv;
    struct timeval failedTv;

    ParamStruct* tParam;
};

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

int g_threadCount = 0;
int g_eventCount = 0;

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
    * @param compressRate 数据压缩率，例如压缩比为10:1的16k opus编码，此时为10；非压缩数据则为1
    * @return 返回sendAudio之后需要sleep的时间
    * @note 对于8k pcm 编码数据, 16位采样，建议每发送1600字节 sleep 100 ms.
            对于16k pcm 编码数据, 16位采样，建议每发送3200字节 sleep 100 ms.
            对于其它编码格式的数据, 用户根据压缩比, 自行估算, 比如压缩比为10:1的 16k opus,
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
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTranscriptionStarted(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    gettimeofday(&(tmpParam->startedTv), NULL);

//    pthread_mutex_lock(&(tmpParam->tParam->mtx));
    tmpParam->tParam->startedConsumed ++;

    unsigned long long timeValue1 = tmpParam->startedTv.tv_sec - tmpParam->startTv.tv_sec;
    unsigned long long timeValue2 = tmpParam->startedTv.tv_usec - tmpParam->startTv.tv_usec;
    unsigned long long timeValue = 0;
    if (timeValue1 > 0) {
        timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    } else {
        timeValue = (timeValue2 / 1000);
    }

    //max
    if (timeValue > tmpParam->tParam->startMaxValue) {
        tmpParam->tParam->startMaxValue = timeValue;
    }

    unsigned long long tmp = timeValue;
    if (tmp <= 50) {
        tmpParam->tParam->s50Value ++;
    } else if (tmp <= 100) {
        tmpParam->tParam->s100Value ++;
    } else if (tmp <= 200) {
        tmpParam->tParam->s200Value ++;
    } else if (tmp <= 500) {
        tmpParam->tParam->s500Value ++;
    } else if (tmp <= 1000) {
        tmpParam->tParam->s1000Value ++;
    } else {
        tmpParam->tParam->s2000Value ++;
    }
    cout << "Started Max Time: " << tmpParam->tParam->startMaxValue << " task id: " << cbEvent->getTaskId()<< endl;

    //min
    if (tmpParam->tParam->startMinValue == 0) {
        tmpParam->tParam->startMinValue = timeValue;
    } else {
        if (timeValue < tmpParam->tParam->startMinValue) {
            tmpParam->tParam->startMinValue = timeValue;
        }
    }

    //ave
    tmpParam->tParam->startTotalValue += timeValue;
    tmpParam->tParam->startAveValue = tmpParam->tParam->startTotalValue / tmpParam->tParam->startedConsumed;

//    pthread_mutex_unlock(&(tmpParam->tParam->mtx));

//    cout << "CbParam: " << tmpParam->userId << ", " << tmpParam->userInfo << endl; // 仅表示自定义参数示例

//	cout << "onTranscriptionStarted: "
//		<< "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
//		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
//		<< endl;
//	 cout << "onTranscriptionStarted: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息

    //通知发送线程start()成功, 可以继续发送数据
//    pthread_mutex_lock(&(tmpParam->mtxWord));
//    pthread_cond_signal(&(tmpParam->cvWord));
//    pthread_mutex_unlock(&(tmpParam->mtxWord));
}

/**
    * @brief 服务端检测到了一句话的开始, sdk内部线程上报SentenceBegin事件
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onSentenceBegin(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
//    cout << "CbParam: " << tmpParam->userId << ", " << tmpParam->userInfo << endl; // 仅表示自定义参数示例
//
//	cout << "onSentenceBegin: "
//		<< "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
//		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
//		<< ", index: " << cbEvent->getSentenceIndex() //句子编号，从1开始递增
//		<< ", time: " << cbEvent->getSentenceTime() //当前已处理的音频时长，单位是毫秒
//		<< endl;
	// cout << "onSentenceBegin: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 服务端检测到了一句话结束, sdk内部线程上报SentenceEnd事件
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onSentenceEnd(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
//    cout << "CbParam: " << tmpParam->userId << ", " << tmpParam->userInfo << endl; // 仅表示自定义参数示例

//	cout << "onSentenceEnd: "
//         << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
//         << ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
//         << ", result: " << cbEvent->getResult()    // 当前句子的完成识别结果
//         << ", index: " << cbEvent->getSentenceIndex()  // 当前句子的索引编号
//         << ", time: " << cbEvent->getSentenceTime()    // 当前句子的音频时长
//         << ", begin_time: " << cbEvent->getSentenceBeginTime() // 对应的SentenceBegin事件的时间
//         << ", confidence: " << cbEvent->getSentenceConfidence()    // 结果置信度,取值范围[0.0,1.0]，值越大表示置信度越高
//         << ", stashResult begin_time: " << cbEvent->getStashResultBeginTime() //下一句话开始时间
//         << ", stashResult current_time: " << cbEvent->getStashResultCurrentTime() //下一句话当前时间
//         << ", stashResult Sentence_id: " << cbEvent->getStashResultSentenceId() //sentence Id
//         << ", stashResult Text: " << cbEvent->getStashResultText() //下一句话前缀
//         << endl;
	// cout << "onSentenceEnd: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 识别结果发生了变化, sdk在接收到云端返回到最新结果时, sdk内部线程上报ResultChanged事件
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTranscriptionResultChanged(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
//    cout << "CbParam: " << tmpParam->userId << ", " << tmpParam->userInfo << endl; // 仅表示自定义参数示例

//	cout << "onTranscriptionResultChanged: "
//		<< "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
//		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
//		<< ", result: " << cbEvent->getResult()    // 当前句子的中间识别结果
//		<< ", index: " << cbEvent->getSentenceIndex()  // 当前句子的索引编号
//		<< ", time: " << cbEvent->getSentenceTime()    // 当前句子的音频时长
//		<< endl;
	// cout << "onTranscriptionResultChanged: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 服务端停止实时音频流识别时, sdk内部线程上报Completed事件
    * @note 上报Completed事件之后，SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送.
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTranscriptionCompleted(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

    gettimeofday(&(tmpParam->completedTv), NULL);

//    pthread_mutex_lock(&(tmpParam->tParam->mtx));
    tmpParam->tParam->completedConsumed ++;

    unsigned long long timeValue1 = tmpParam->completedTv.tv_sec - tmpParam->startTv.tv_sec;
    unsigned long long timeValue2 = tmpParam->completedTv.tv_usec - tmpParam->startTv.tv_usec;
    unsigned long long timeValue = 0;
    if (timeValue1 > 0) {
        timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    } else {
        timeValue = (timeValue2 / 1000);
    }

    //max
    if (timeValue > tmpParam->tParam->endMaxValue) {
        tmpParam->tParam->endMaxValue = timeValue;
    }

    //min
    if (tmpParam->tParam->endMinValue == 0) {
        tmpParam->tParam->endMinValue = timeValue;
    } else {
        if (timeValue < tmpParam->tParam->endMinValue) {
            tmpParam->tParam->endMinValue = timeValue;
        }
    }

    //ave
    tmpParam->tParam->endTotalValue += timeValue;
    tmpParam->tParam->endAveValue = tmpParam->tParam->endTotalValue / tmpParam->tParam->completedConsumed;

    cout << "Completed Max Time: " << tmpParam->tParam->endMaxValue << " task id: " << cbEvent->getTaskId()<< endl;

//    pthread_mutex_unlock(&(tmpParam->tParam->mtx));

//    cout << "CbParam: " << tmpParam->userId << ", " << tmpParam->userInfo << endl; // 仅表示自定义参数示例

	cout << "onTranscriptionCompleted: "
		<< "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< endl;
	// cout << "onTranscriptionCompleted: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 识别过程(包含start(), send(), stop())发生异常时, sdk内部线程上报TaskFailed事件
    * @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onTaskFailed(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

//    gettimeofday(&(tmpParam->failedTv), NULL);

//    pthread_mutex_lock(&(tmpParam->tParam->mtx));
    tmpParam->tParam->failedConsumed ++;
//    pthread_mutex_unlock(&(tmpParam->tParam->mtx));

//    cout << "CbParam: " << tmpParam->userId << ", " << tmpParam->userInfo << endl; // 仅表示自定义参数示例

	cout << "onTaskFailed: "
		<< "status code: " << cbEvent->getStatusCode()
		<< ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
		<< ", error message: " << cbEvent->getErrorMessage()
		<< endl;
	// cout << "onTaskFailed: All response:" << cbEvent->getAllResponse() << endl; // 获取服务端返回的全部信息
}

/**
    * @brief 识别结束或发生异常时，会关闭连接通道, sdk内部线程上报ChannelCloseed事件
    * @param cbEvent 回调事件结构, 详见nlsEvent.h
    * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
    * @return
*/
void onChannelClosed(NlsEvent* cbEvent, void* cbParam) {
	ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

//    gettimeofday(&(tmpParam->closedTv), NULL);

//    pthread_mutex_lock(&(tmpParam->tParam->mtx));
    tmpParam->tParam->closeConsumed ++;
//    cout << "CloseConsumed: " << tmpParam->tParam->closeConsumed << endl;
//    pthread_mutex_unlock(&(tmpParam->tParam->mtx));

//    cout << "CbParam: " << tmpParam->userId << ", " << tmpParam->userInfo << endl; // 仅表示自定义参数示例

	cout << "OnChannelCloseed: All response: " << cbEvent->getAllResponse() << endl; // getResponse() 可以通道关闭信息

    //通知发送线程, 最终识别结果已经返回, 可以调用stop()
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
}

// 工作线程
void* pthreadFunction(void* arg) {
    int sleepMs = 0;
    int testCount = 50;

    ParamStruct* tst = (ParamStruct*)arg;
    if (tst == NULL) {
        cout << "arg is not valid." << endl;
        return NULL;
    }

    pthread_mutex_init(&(tst->mtx), NULL);

    do {
    //初始化自定义回调参数, 以下两变量仅作为示例表示参数传递, 在demo中不起任何作用
    //回调参数在堆中分配之后, SDK在销毁requesr对象时会一并销毁, 外界无需在释放
    ParamCallBack *cbParam = NULL;
    cbParam = new ParamCallBack(tst);

//    pthread_mutex_lock(&(cbParam->tParam->mtx));
    cbParam->tParam->requestConsumed ++;
//    pthread_mutex_unlock(&(cbParam->tParam->mtx));

    /* 打开音频文件, 获取数据 */
    ifstream fs;
    fs.open(tst->fileName.c_str(), ios::binary | ios::in);
    if (!fs) {
        cout << tst->fileName << " isn't exist.." << endl;
        return NULL;
    }

    /*
     * 2: 创建实时音频流识别SpeechTranscriberRequest对象
     */
    SpeechTranscriberRequest* request = NlsClient::getInstance()->createTranscriberRequest();
    if (request == NULL) {
        cout << "createTranscriberRequest failed." << endl;
        return NULL;
    }

    request->setOnTranscriptionStarted(onTranscriptionStarted, cbParam); // 设置识别启动回调函数
    request->setOnTranscriptionResultChanged(onTranscriptionResultChanged, cbParam); // 设置识别结果变化回调函数
    request->setOnTranscriptionCompleted(onTranscriptionCompleted, cbParam); // 设置语音转写结束回调函数
    request->setOnSentenceBegin(onSentenceBegin, cbParam); // 设置一句话开始回调函数
    request->setOnSentenceEnd(onSentenceEnd, cbParam); // 设置一句话结束回调函数
    request->setOnTaskFailed(onTaskFailed, cbParam); // 设置异常识别回调函数
    request->setOnChannelClosed(onChannelClosed, cbParam); // 设置识别通道关闭回调函数

//    request->setUrl("ws://100.82.42.38:8101/ws/v1");
//    request->setAppKey("default");
//    request->setToken("default");

    request->setAppKey("fSog6lRUUU0fPKFd");
    request->setToken("6f2f5f6b6ba543e5a6f0d48a596fd07f");

//    request->setAppKey("230ee5f5");
//    request->setToken("450372e4279243ab980c01bcc2b3c793");
//    request->setUrl("ws://pre-nls-gateway-inner.aliyuncs.com:80/ws/v1");
//    request->setPayloadParam("{\"model\":\"test-regression-model\"}");

//    request->setAppKey(tst->appkey.c_str()); // 设置AppKey, 必填参数, 请参照官网申请
	request->setFormat("pcm"); // 设置音频数据编码格式, 默认是pcm
	request->setSampleRate(SAMPLE_RATE); // 设置音频数据采样率, 可选参数，目前支持16000, 8000. 默认是16000
	request->setIntermediateResult(true); // 设置是否返回中间识别结果, 可选参数. 默认false
	request->setPunctuationPrediction(true); // 设置是否在后处理中添加标点, 可选参数. 默认false
	request->setInverseTextNormalization(true); // 设置是否在后处理中执行数字转写, 可选参数. 默认false

    //语音断句检测阈值，一句话之后静音长度超过该值，即本句结束，合法参数范围200～2000(ms)，默认值800ms
//    request->setMaxSentenceSilence(800);
//    request->setCustomizationId("TestId_123"); //定制模型id, 可选.
//    request->setVocabularyId("TestId_456"); //定制泛热词id, 可选.

//    request->setToken(tst->token.c_str());

    gettimeofday(&(cbParam->startTv), NULL);

    struct timeval now;
    struct timespec outtime;
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 5;
    outtime.tv_nsec = now.tv_usec * 1000;

    if (request->start() < 0) {
        cout << "start() failed." << endl;
        NlsClient::getInstance()->releaseTranscriberRequest(request); // start()失败，释放request对象
        return NULL;
    }

    //等待started事件返回, 在发送
//    cout << "wait started callback." << endl;
//    pthread_mutex_lock(&(cbParam->mtxWord));
//    pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
//    pthread_mutex_unlock(&(cbParam->mtxWord));

    int ret = 0;
    while (!fs.eof()) {
        uint8_t data[FRAME_SIZE] = {0};

        fs.read((char *)data, sizeof(uint8_t) * FRAME_SIZE);
        size_t nlen = fs.gcount();
        if (nlen <= 0) {
            continue;
        }

        /*
        * 4: 发送音频数据. sendAudio返回-1表示发送失败, 需要停止发送.
        */
        ret = request->sendAudio(data, nlen);
        if (ret < 0) {
            // 发送失败, 退出循环数据发送
            cout << "send data fail." << endl;
            break;
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
        usleep(sleepMs * 1000);
    }

    // 关闭音频文件
    fs.close();

    /*
    * 6: 数据发送结束，关闭识别连接通道.
    * stop()为异步操作.
    */
    request->stop();

	/*
    * 7: 识别结束, 释放request对象
	*/
    if (ret == 0) {
        cout << "wait closed callback." << endl;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 10;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&(cbParam->mtxWord));
        pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
        pthread_mutex_unlock(&(cbParam->mtxWord));
    }

    NlsClient::getInstance()->releaseTranscriberRequest(request);

//    cout << "Started Max time:" << tst->startMaxValue << endl;
//    cout << "Completed Max time:" << tst->endMaxValue << endl;

//    usleep(5* 1000 * 1000);

    testCount --;
} while(testCount >= 0);

    pthread_mutex_destroy(&(tst->mtx));

    return NULL;
}

/**
 * 识别多个音频数据;
 * sdk多线程指一个音频数据对应一个线程, 非一个音频数据对应多个线程.
 * 示例代码为同时开启4个线程识别4个文件;
 * 免费用户并发连接不能超过10个;
 */
#define AUDIO_FILE_NUMS 50//300
#define AUDIO_FILE_NAME_LENGTH 32
int speechTranscriberMultFile(const char* appkey) {
    /**
     * 获取当前系统时间戳，判断token是否过期
     */
    std::time_t curTime = std::time(0);
//    if (g_expireTime - curTime < 10) {
//        cout << "the token will be expired, please generate new token by AccessKey-ID and AccessKey-Secret." << endl;
//        if (-1 == generateToken(g_akId, g_akSecret, &g_token, &g_expireTime)) {
//            return -1;
//        }
//    }

    char audioFileNames[AUDIO_FILE_NAME_LENGTH] = {0};
    ParamStruct pa[g_threadCount];

    for (int i = 0; i < g_threadCount; i ++) {
        pa[i].token = g_token;
        pa[i].appkey = appkey;
        memset(audioFileNames, 0x0, AUDIO_FILE_NAME_LENGTH);

        pa[i].startedConsumed = 0;
        pa[i].completedConsumed = 0;
        pa[i].closeConsumed = 0;
        pa[i].failedConsumed = 0;
        pa[i].requestConsumed = 0;

        pa[i].startTotalValue = 0;
        pa[i].startAveValue = 0;
        pa[i].startMaxValue = 0;
        pa[i].startMinValue = 0;

        pa[i].endTotalValue = 0;
        pa[i].endAveValue = 0;
        pa[i].endMaxValue = 0;
        pa[i].endMinValue = 0;

        pa[i].s50Value = 0;
        pa[i].s100Value = 0;
        pa[i].s200Value = 0;
        pa[i].s500Value = 0;
        pa[i].s1000Value = 0;
        pa[i].s2000Value = 0;

        snprintf(audioFileNames, AUDIO_FILE_NAME_LENGTH, "test%d.wav", i);
//        snprintf(audioFileNames, AUDIO_FILE_NAME_LENGTH, "test%d.wav", i+1);
//        cout << "audioFileNames:" << audioFileNames << endl;
        pa[i].fileName = audioFileNames;
    }

    vector<pthread_t> pthreadId(g_threadCount);
    for (int j = 0; j < g_threadCount; j++) {
        pthread_create(&pthreadId[j], NULL, &pthreadFunction, (void *)&(pa[j]));
    }

    for (int j = 0; j < g_threadCount; j++) {
        pthread_join(pthreadId[j], NULL);
    }

    unsigned long long sTotalCount = 0;
    unsigned long long eTotalCount = 0;
    unsigned long long fTotalCount = 0;
    unsigned long long cTotalCount = 0;
    unsigned long long rTotalCount = 0;

    unsigned long long sMaxTime = 0;
    unsigned long long sMinTime = 0;
    unsigned long long sAveTime = 0;

    unsigned long long s50Count = 0;
    unsigned long long s100Count = 0;
    unsigned long long s200Count = 0;
    unsigned long long s500Count = 0;
    unsigned long long s1000Count = 0;
    unsigned long long s2000Count = 0;

    unsigned long long eMaxTime = 0;
    unsigned long long eMinTime = 0;
    unsigned long long eAveTime = 0;

    for (int i = 0; i < g_threadCount; i ++) {

        sTotalCount += pa[i].startedConsumed;
        eTotalCount += pa[i].completedConsumed;
        fTotalCount += pa[i].failedConsumed;
        cTotalCount += pa[i].closeConsumed;
        rTotalCount += pa[i].requestConsumed;

        cout << "Closed:" << pa[i].closeConsumed << endl;

        //start
        if (pa[i].startMaxValue > sMaxTime) {
            sMaxTime = pa[i].startMaxValue;
        }

        if (sMinTime == 0) {
            sMinTime = pa[i].startMinValue;
        } else {
            if (pa[i].startMinValue < sMinTime) {
                sMinTime = pa[i].startMinValue;
            }
        }

        sAveTime += pa[i].startAveValue;

        s50Count += pa[i].s50Value;
        s100Count += pa[i].s100Value;
        s200Count += pa[i].s200Value;
        s500Count += pa[i].s500Value;
        s1000Count += pa[i].s1000Value;
        s2000Count += pa[i].s2000Value;

        //end
        if (pa[i].endMaxValue > eMaxTime) {
            eMaxTime = pa[i].endMaxValue;
        }

        if (eMinTime == 0) {
            eMinTime = pa[i].endMinValue;
        } else {
            if (pa[i].endMinValue < eMinTime) {
                eMinTime = pa[i].endMinValue;
            }
        }

        eAveTime += pa[i].endAveValue;
    }

    sAveTime /= g_threadCount;
    eAveTime /= g_threadCount;

    for (int i = 0; i < g_threadCount; i ++) {
        cout << "No." << i << " Max completed time: " << pa[i].endMaxValue << " ms" << endl;
        cout << "No." << i << " Min completed time: " << pa[i].endMinValue << " ms" << endl;
        cout << "No." << i << " Ave completed time: " << pa[i].endAveValue << " ms" << endl;
    }

    cout << "\n ------------------- \n" << endl;

    cout << "Total. " << endl;
    cout << "Request: " << rTotalCount << endl;
    cout << "Started: " << sTotalCount << endl;
    cout << "Completed: " << eTotalCount << endl;
    cout << "Failed: " << fTotalCount << endl;
    cout << "Closed: " << cTotalCount << endl;

    cout << "\n ------------------- \n" << endl;

    cout << "Max started time: " << sMaxTime << " ms" << endl;
    cout << "Min started time: " << sMinTime << " ms" << endl;
    cout << "Ave started time: " << sAveTime << " ms" << endl;

    cout << "\n ------------------- \n" << endl;

    cout << "Started time <= 50: " << s50Count << endl;
    cout << "Started time <= 100: " << s100Count << endl;
    cout << "Started time <= 200: " << s200Count << endl;
    cout << "Started time <= 500: " << s500Count << endl;
    cout << "Started time <= 1000: " << s1000Count << endl;
    cout << "Started time > 1000: " << s2000Count << endl;

    cout << "\n ------------------- \n" << endl;

    cout << "Max completed time: " << eMaxTime << " ms" << endl;
    cout << "Min completed time: " << eMinTime << " ms" << endl;
    cout << "Ave completed time: " << eAveTime << " ms" << endl;

    cout << "\n ------------------- \n" << endl;

    return 0;
}

int main(int arc, char* argv[]) {
    if (arc < 3) {
        cout << "params is not valid. Usage: ./demo <your appkey> <your AccessKey ID> <your AccessKey Secret> " << endl;
        return -1;
    }

//    string appkey = argv[1];
//    g_akId = argv[2];
//    g_akSecret = argv[3];


    g_eventCount = atoi(argv[1]);
    g_threadCount = atoi(argv[2]);

    // 根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-Transcriber.txt， LogDebug表示输出所有级别日志
//    int ret = NlsClient::getInstance()->setLogConfig("log-transcriber", LogError, 200);
//    if (-1 == ret) {
//        cout << "set log failed." << endl;
//        return -1;
//    }

     cout << "Event Count: " << g_eventCount  << ", Thread Count: " << g_threadCount << endl;

     sleep(10);

    //启动工作线程
    NlsClient::getInstance()->startWorkThread(g_eventCount);

    // 识别多个音频数据
    speechTranscriberMultFile("123");//(appkey.c_str());

    // 所有工作完成，进程退出前，释放nlsClient. 请注意, releaseInstance()非线程安全.
    NlsClient::releaseInstance();

    return 0;
}
