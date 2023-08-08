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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctime>
#include <map>
#include <list>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sys/time.h>
#include <signal.h>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "nlsToken.h"
#include "dialogAssistantRequest.h"

#define SELF_TESTING_TRIGGER
#define FRAME_SIZE 640
//#define FRAME_SIZE 3200
#define SAMPLE_RATE 16000
#define LOOP_TIMEOUT 60

// 自定义线程参数
struct ParamStruct {
  std::string fileName;
  std::string wakeWordFileName;
  std::string appkey;
  std::string token;
  std::string url;
  std::string text;
};

// 自定义事件回调参数
struct ParamCallBack {
 public:
  ParamCallBack() {
    pthread_mutex_init(&mtxWord, NULL);
    pthread_cond_init(&cvWord, NULL);
  };
  ~ParamCallBack() {
    pthread_mutex_destroy(&mtxWord);
    pthread_cond_destroy(&cvWord);
  };

  unsigned long userId;
  char userInfo[8];

  pthread_mutex_t mtxWord;
  pthread_cond_t cvWord;
};

// 统计参数
struct ParamStatistics {
  bool running;
  bool success_flag;
  bool failed_flag;

  uint64_t audio_ms;
  uint64_t start_ms;
  uint64_t end_ms;
  uint64_t ave_ms;

  uint32_t s_cnt;
};

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey Secret重新生成一个token，
 * 并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，
 * 只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
 */
std::string g_appkey = "";
std::string g_akId = "";
std::string g_akSecret = "";
std::string g_token = "";
std::string g_url = "";
int g_threads = 1;
static int loop_timeout = LOOP_TIMEOUT;

long g_expireTime = -1;
volatile static bool global_run = false;
volatile static bool auto_close_run = false;
static std::map<unsigned long, struct ParamStatistics *> g_statistics;
static pthread_mutex_t params_mtx;
static int encoder_type = ENCODER_OPU;

void signal_handler_int(int signo) {
  std::cout << "\nget interrupt mesg\n" << std::endl;
  global_run = false;
}
void signal_handler_quit(int signo) {
  std::cout << "\nget quit mesg\n" << std::endl;
  global_run = false;
}

static void vectorStartStore(unsigned long pid) {
  pthread_mutex_lock(&params_mtx);

  std::map<unsigned long, struct ParamStatistics*>::iterator iter;
  iter = g_statistics.find(pid);
  if (iter != g_statistics.end()) {
    // 已经存在
    struct timeval start_tv;
    gettimeofday(&start_tv, NULL);
    iter->second->start_ms = start_tv.tv_sec * 1000 + start_tv.tv_usec / 1000;
    std::cout << "vectorStartStore start:" << iter->second->start_ms << std::endl;
  }

  pthread_mutex_unlock(&params_mtx);
  return;
}

static void vectorSetParams(unsigned long pid, bool add,
                            struct ParamStatistics params) {
  pthread_mutex_lock(&params_mtx);

  std::map<unsigned long, struct ParamStatistics*>::iterator iter;
  iter = g_statistics.find(pid);
  if (iter != g_statistics.end()) {
    // 已经存在
    iter->second->running = params.running;
    iter->second->success_flag = params.success_flag;
    iter->second->failed_flag = false;
    if (params.audio_ms > 0) {
      iter->second->audio_ms = params.audio_ms;
    }
  } else {
    // 不存在, 新的pid
    if (add) {
//      std::cout << "vectorSetParams create pid:" << pid << std::endl;
      struct ParamStatistics *p_tmp = new(struct ParamStatistics);
      if (!p_tmp) return;
      memset(p_tmp, 0, sizeof(struct ParamStatistics));
      p_tmp->running = params.running;
      p_tmp->success_flag = params.success_flag;
      p_tmp->failed_flag = false;
      if (params.audio_ms > 0) {
        p_tmp->audio_ms = params.audio_ms;
      }
      g_statistics.insert(std::make_pair(pid, p_tmp));
    } else {
    }
  }

  pthread_mutex_unlock(&params_mtx);
  return;
}

static void vectorSetRunning(unsigned long pid, bool run) {
  pthread_mutex_lock(&params_mtx);

  std::map<unsigned long, struct ParamStatistics*>::iterator iter;
  iter = g_statistics.find(pid);
//  std::cout << "vectorSetRunning pid:"<< pid
//    << "; run:" << run << std::endl;
  if (iter != g_statistics.end()) {
    // 已经存在
    iter->second->running = run;
  } else {
#if 0
    // 不存在, 新的pid
#endif
  }

  pthread_mutex_unlock(&params_mtx);
  return;
}

static void vectorSetResult(unsigned long pid, bool ret) {
  pthread_mutex_lock(&params_mtx);

  std::map<unsigned long, struct ParamStatistics*>::iterator iter;
  iter = g_statistics.find(pid);
  if (iter != g_statistics.end()) {
    // 已经存在
    iter->second->success_flag = ret;

    if (ret) {
      struct timeval end_tv;
      gettimeofday(&end_tv, NULL);
      iter->second->end_ms = end_tv.tv_sec * 1000 + end_tv.tv_usec / 1000;
      uint64_t d_ms = iter->second->end_ms - iter->second->start_ms;
#if 0
      std::cout << "vectorSetResult start:" << iter->second->start_ms
        << " end:" << iter->second->end_ms
        << " d:" << iter->second->end_ms - iter->second->start_ms
        << std::endl;
#endif
      if (iter->second->ave_ms == 0) {
        iter->second->ave_ms = d_ms;
      } else {
        iter->second->ave_ms = (d_ms + iter->second->ave_ms) / 2;
      }
      iter->second->s_cnt++;
    }
  } else {
  }

  pthread_mutex_unlock(&params_mtx);
  return;
}

static void vectorSetFailed(unsigned long pid, bool ret) {
  pthread_mutex_lock(&params_mtx);

  std::map<unsigned long, struct ParamStatistics*>::iterator iter;
  iter = g_statistics.find(pid);
  if (iter != g_statistics.end()) {
    // 已经存在
    iter->second->failed_flag = ret;
  } else {
  }

  pthread_mutex_unlock(&params_mtx);
  return;
}

static bool vectorGetRunning(unsigned long pid) {
  pthread_mutex_lock(&params_mtx);

  bool result = false;
  std::map<unsigned long, struct ParamStatistics*>::iterator iter;
  iter = g_statistics.find(pid);
  if (iter != g_statistics.end()) {
    // 存在
    result = iter->second->running;
  } else {
    // 不存在, 新的pid
  }

  pthread_mutex_unlock(&params_mtx);
  return result;
}

static bool vectorGetFailed(unsigned long pid) {
  pthread_mutex_lock(&params_mtx);

  bool result = false;
  std::map<unsigned long, struct ParamStatistics*>::iterator iter;
  iter = g_statistics.find(pid);
  if (iter != g_statistics.end()) {
    // 存在
    result = iter->second->failed_flag;
  } else {
    // 不存在, 新的pid
  }

  pthread_mutex_unlock(&params_mtx);
  return result;
}


/**
 * 根据AccessKey ID和AccessKey Secret重新生成一个token，并获取其有效期时间戳
 */
int generateToken(std::string akId, std::string akSecret,
                  std::string* token, long* expireTime) {
  AlibabaNlsCommon::NlsToken nlsTokenRequest;
  nlsTokenRequest.setAccessKeyId(akId);
  nlsTokenRequest.setKeySecret(akSecret);

  int retCode = nlsTokenRequest.applyNlsToken();
  /*获取失败原因*/
  if (retCode < 0) {
    std::cout << "Failed error code: "
              << retCode
              << "  error msg: "
              << nlsTokenRequest.getErrorMsg()
              << std::endl;
    return retCode;
  }

  *token = nlsTokenRequest.getToken();
  *expireTime = nlsTokenRequest.getExpireTime();

  return 0;
}

/**
 * @brief 获取sendAudio发送延时时间.
 * @param dataSize 待发送数据大小.
 * @param sampleRate 采样率 16k/8K.
 * @param compressRate 数据压缩率, 例如压缩比为10:1的16k opus编码, 此时为10;
 *                     非压缩数据则为1.
 * @return 返回sendAudio之后需要sleep的时间.
 * @note 对于8k pcm 编码数据, 16位采样, 建议每发送1600字节 sleep 100 ms.
 *       对于16k pcm 编码数据, 16位采样, 建议每发送3200字节 sleep 100 ms.
         对于其它编码格式(OPUS)的数据, 由于传递给SDK的仍然是PCM编码数据,
         按照SDK OPUS/OPU 数据长度限制, 需要每次发送640字节 sleep 20ms.
 */
unsigned int getSendAudioSleepTime(const int dataSize,
                                   const int sampleRate,
                                   const int compressRate) {
  // 仅支持16位采样
  const int sampleBytes = 16;
  // 仅支持单通道
  const int soundChannel = 1;

  // 当前采样率, 采样位数下每秒采样数据的大小
  int bytes = (sampleRate * sampleBytes * soundChannel) / 8;

  // 当前采样率, 采样位数下每毫秒采样数据的大小
  int bytesMs = bytes / 1000;

  // 待发送数据大小除以每毫秒采样数据大小, 以获取sleep时间
  int sleepMs = (dataSize * compressRate) / bytesMs;

  return sleepMs;
}

/**
 * @brief 调用start(), 成功与云端建立连接. 
 *        收到此事件表示服务端已准备好接受音频数据.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h.
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数.
 * @return
 */
void OnRecognitionStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnRecognitionStarted userId: " << tmpParam->userId
    << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

  std::cout << "OnRecognitionStarted: "
    << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码, 成功为0或者20000000, 失败时对应失败的错误码.
    << ", task id: " << cbEvent->getTaskId()   // 当前任务的task id, 方便定位问题, 建议输出.
    << std::endl;
  // std::cout << "OnRecognitionStarted: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息.

  // pid, add, run, success
  struct ParamStatistics params;
  params.running = true;
  params.success_flag = false;
  params.audio_ms = 0;
  vectorSetParams(tmpParam->userId, true, params);
}

/**
 * @brief 此为语音识别中间结果, 随着语音数据增多, 语音识别结果发生了变化. 
 *        客户端可以根据这个事件更新UI状态, 比如打字机效果.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h.
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数.
 * @return
 */
void OnRecognitionResultChanged(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
#if 0
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnRecognitionResultChanged userId: " << tmpParam->userId
    << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

  std::cout << "OnRecognitionResultChanged: "
    << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码, 成功为0或者20000000,失败时对应失败的错误码.
    << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id, 方便定位问题, 建议输出.
    << ", result: " << cbEvent->getResult()     // 获取中间识别结果.
    << std::endl;
  // std::cout << "OnRecognitionResultChanged: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息.
#endif
}

/**
 * @brief 用于通知客户端识, 最终的识别结果已经确定, 不会再发生变化. 
 *        客户端可以根据这个事件更新UI状态.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h.
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数.
 * @return
 */
void OnRecognitionCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnRecognitionCompleted userId: " << tmpParam->userId
    << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

  std::cout << "OnRecognitionCompleted: "
    << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码, 成功为0或者20000000, 失败时对应失败的错误码.
    << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id, 方便定位问题, 建议输出.
    << ", result: " << cbEvent->getResult()  // 获取中间识别结果.
    << std::endl;
  // std::cout << "OnRecognitionCompleted: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息.
}

/**
 * @brief 用于通知客户端用户的意图已经被确认.
 * @note 收到事件之后, SDK内部会关闭识别连接通道.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h.
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数.
 * @return
 */
void OnDialogResultGenerated(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnDialogResultGenerated userId: " << tmpParam->userId
    << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

  std::cout << "OnDialogResultGenerated: "
    << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码, 成功为0或者20000000, 失败时对应失败的错误码.
    << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id, 方便定位问题, 建议输出.
    << std::endl;
  std::cout << "OnDialogResultGenerated: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息.

  vectorSetResult(tmpParam->userId, true);
}

/**
 * @brief 识别过程(包含start(), sendAudio(), stop())发生异常时, sdk内部线程上报TaskFailed事件.
 * @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道. 此时调用sendAudio会返回负值, 请停止发送.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h.
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数.
 * @return
 */
void OnRecognitionTaskFailed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnRecognitionTaskFailed userId: " << tmpParam->userId
    << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

  std::cout << "OnRecognitionTaskFailed: "
    << "status code: " << cbEvent->getStatusCode() // 获取消息的状态码, 成功为0或者20000000, 失败时对应失败的错误码.
    << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id, 方便定位问题, 建议输出.
    << ", error message: " << cbEvent->getErrorMessage()
    << std::endl;
  // std::cout << "OnRecognitionTaskFailed: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息.

  vectorSetResult(tmpParam->userId, false);
  vectorSetFailed(tmpParam->userId, true);
}

/**
 * @brief 识别结束或发生异常时, 会关闭连接通道, sdk内部线程上报ChannelCloseed事件.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h.
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数.
 * @return
 */
void OnRecognitionChannelClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnRecognitionChannelClosed userId: "
    << tmpParam->userId << ", "
    << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

  std::cout << "OnRecognitionChannelClosed: All response:"
    << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息.

  vectorSetRunning(tmpParam->userId, false);

  //通知发送线程, 最终识别结果已经返回, 可以调用stop()
  pthread_mutex_lock(&(tmpParam->mtxWord));
  pthread_cond_signal(&(tmpParam->cvWord));
  pthread_mutex_unlock(&(tmpParam->mtxWord));

//  delete tmpParam; //识别流程结束,释放回调参数
}

/**
 * @brief 用于通知客户端，语音唤醒的二次确认已经结束。
 *        如果唤醒被拒绝，服务端会直接结束当前任务并重置连接状态.
 * @note 不允许在回调函数内部调用stop(), 
 *       releaseDialogAssistantRequest()对象操作, 否则会异常.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h.
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数.
 * @return
 */
void OnWakeWordVerificationCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

  std::cout << "CbParam: " << tmpParam->userId
      << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例
  std::cout << "Wake accepted: " << cbEvent->getWakeWordAccepted() << std::endl;
  std::cout << "OnWakeWordVerificationCompleted: All response:"
      << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息.
}

void* autoCloseFunc(void* arg) {
  int timeout = 50;

  while (!global_run && timeout-- > 0) {
    usleep(100 * 1000);
  }
  timeout = loop_timeout;
  while (timeout-- > 0 && global_run) {
    usleep(1000 * 1000);
  }
  global_run = false;

  return NULL;
}

//文本进文本出
void* TextDialogPthreadFunc(void* arg) {
  // 0: 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct* tst = (ParamStruct*)arg;
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  //初始化自定义回调参数, 以下两变量仅作为示例表示参数传递, 在demo中不起任何作用
  //回调参数在堆中分配之后, SDK在销毁requesr对象时会一并销毁, 外界无需在释放
  ParamCallBack *cbParam = NULL;
  cbParam = new ParamCallBack;
  cbParam->userId = 1234;
  strcpy(cbParam->userInfo, "User.");

  /*
   * 2: 创建一句话识别DialogAssistantRequest对象
   */
  AlibabaNls::DialogAssistantRequest* request =
      AlibabaNls::NlsClient::getInstance()->createDialogAssistantRequest();
  if (request == NULL) {
    std::cout << "createRecognizerRequest failed." << std::endl;
    return NULL;
  }
  // 设置异常识别回调函数.
  request->setOnTaskFailed(OnRecognitionTaskFailed, cbParam);
  // 设置识别通道关闭回调函数.
  request->setOnChannelClosed(OnRecognitionChannelClosed, cbParam);
  // 设置意图确认回调函数.
  request->setOnDialogResultGenerated(OnDialogResultGenerated, cbParam);

  // 设置服务端url, 必填参数.
  if (!tst->url.empty()) {
    std::cout << "setUrl: " << tst->url << std::endl;
    request->setUrl(tst->url.c_str());
  }
  // 设置AppKey, 必填参数, 请参照官网申请.
  request->setAppKey(tst->appkey.c_str());
  std::cout << "setAppKey: " << tst->appkey << std::endl;
  // 设置对话服务的会话ID, 必填参数.
  request->setSessionId("9bc0eab5c69045c78a209d804eada674");
  // 设置对话的输入文本, 必填参数.
  request->setQuery(tst->text.c_str());

  // 设置账号校验token, 必填参数
  if (!tst->token.empty()) {
    request->setToken(tst->token.c_str());
    std::cout << "setToken: " << tst->token << std::endl;
  }

  //    request->setQueryParams("[{\"test\":\"test_value\",\"test1\":\"test_value1\"},{\"test2\":\"test_value2\",\"test3\":\"test_value3\"}]");
  request->setQueryContext("{\"line\":\"01\",\"station\":\"15\"}");

  request->setPayloadParam("{\"test1\":\"01\", \"test2\":\"15\"}");
  request->setContextParam("{\"network\":{\"ip\":\"100.101.102.103\"}, \"custom\":{\"dingding_user_id\":\"xxx\"}}");

  /*
   * 3: queryText.
   */
  if (request->queryText() < 0) {
    std::cout << "queryText failed." << std::endl;
  }

  // 4: 识别结束, 释放request对象.
  AlibabaNls::NlsClient::getInstance()->releaseDialogAssistantRequest(request);

  return NULL;
}

//音频进,文本出
void* AudioDialogPthreadFunc(void* arg) {
  int sleepMs = 0;

  // 0: 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct* tst = (ParamStruct*)arg;
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  // 打开音频文件, 获取数据.
  std::ifstream fs;
  fs.open(tst->fileName.c_str(), std::ios::binary | std::ios::in);
  if (!fs) {
    std::cout << tst->fileName << " isn't exist.." << std::endl;
    return NULL;
  } else {
    fs.seekg(0, std::ios::end);
    int len = fs.tellg();

    struct ParamStatistics params;
    params.running = false;
    params.success_flag = false;
    params.audio_ms = len / 640 * 20;
    vectorSetParams(pthread_self(), true, params);
  }

  //初始化自定义回调参数, 以下两变量仅作为示例表示参数传递, 在demo中不起任何作用
  //回调参数在堆中分配之后, SDK在销毁requesr对象时会一并销毁, 外界无需在释放
  ParamCallBack *cbParam = NULL;
  cbParam = new ParamCallBack;
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  strcpy(cbParam->userInfo, "User.");

  while (global_run) {
    /*
     * 2: 创建一句话识别DialogAssistantRequest对象
     */
    AlibabaNls::DialogAssistantRequest* request =
        AlibabaNls::NlsClient::getInstance()->createDialogAssistantRequest();
    if (request == NULL) {
      std::cout << "createRecognizerRequest failed." << std::endl;
      //    return NULL;
      break;
    }

    // 设置start()成功回调函数.
    request->setOnRecognitionStarted(OnRecognitionStarted, cbParam);
    // 设置异常识别回调函数.
    request->setOnTaskFailed(OnRecognitionTaskFailed, cbParam);
    // 设置识别通道关闭回调函数.
    request->setOnChannelClosed(OnRecognitionChannelClosed, cbParam);
    // 设置中间结果回调函数.
    request->setOnRecognitionResultChanged(OnRecognitionResultChanged, cbParam);
    // 设置最终识别结果回调函数.
    request->setOnRecognitionCompleted(OnRecognitionCompleted, cbParam);
    // 设置意图确认回调函数.
    request->setOnDialogResultGenerated(OnDialogResultGenerated, cbParam);

    // 设置服务端url, 必填参数.
    if (!tst->url.empty()) {
      std::cout << "setUrl: " << tst->url << std::endl;
      request->setUrl(tst->url.c_str());
    }
    // 设置AppKey, 必填参数, 请参照官网申请.
    std::cout << "setAppKey: " << tst->appkey << std::endl;
    request->setAppKey(tst->appkey.c_str());
    // 设置账号校验token, 必填参数
    request->setToken(tst->token.c_str());
    std::cout << "setToken: " << tst->token << std::endl;

    // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm
    if (encoder_type == ENCODER_OPUS) {
      request->setFormat("opus");
    } else if (encoder_type == ENCODER_OPU) {
      request->setFormat("opu");
    } else {
      request->setFormat("pcm");
    }

    // 设置音频数据采样率, 可选参数, 目前支持16000, 8000. 默认是16000.
    request->setSampleRate(SAMPLE_RATE);
    // 设置对话服务的会话ID, 必填参数.
    request->setSessionId("9bc0eab5c69045c78a209d804eada674");

    /*
     * 3: start()为异步操作
     */
    vectorStartStore(pthread_self());
    std::cout << "start ->" << std::endl;
    int ret = request->start();
    if (ret < 0) {
      std::cout << "start failed(" << ret << ")." << std::endl;
      break;
    } else {
      std::cout << "start success." << std::endl;
      bool running_flag = vectorGetRunning(cbParam->userId);
      int timeout = 40;
      // 等待收到started事件后再往下跑
      while (timeout-- > 0 && !running_flag) {
        usleep(50 * 1000);
        running_flag = vectorGetRunning(cbParam->userId);
      }
    }

    // 文件是否读取完毕, 或者接收到TaskFailed, closed, completed回调, 终止send.
    while (!fs.eof()) {
      uint8_t data[FRAME_SIZE] = {0};

      fs.read((char *)data, sizeof(uint8_t) * FRAME_SIZE);
      size_t nlen = fs.gcount();
      if (nlen <= 0) {
        continue;
      }

      /*
       * 4: 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败, 
       *    需要停止发送; 返回0 为成功.
       *    notice : 返回值非成功发送字节数.
       *    若希望用省流量的opus格式上传音频数据, 则第三参数传入ENCODER_OPU
       *    ENCODER_OPU/ENCODER_OPUS模式时,nlen必须为640
       */
      int ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      if (ret < 0) {
        // 发送失败, 退出循环数据发送.
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
      }

      /*
       *语音数据发送控制:
       *语音数据是实时的, 不用sleep控制速率, 直接发送即可.
       *语音数据来自文件, 发送时需要控制速率,
       *使单位时间内发送的数据大小接近单位时间原始语音数据存储的大小.
       */
      // 根据发送数据大小, 采样率, 数据压缩比来获取sleep时间.
      sleepMs = getSendAudioSleepTime(nlen, SAMPLE_RATE, 1);

      /*
       * 5: 语音数据发送延时控制.
       */
      usleep(sleepMs * 1000);
    }

    /*
     * 6: 数据发送结束, 关闭识别连接通道.
     */
    std::cout << "stop ->" << std::endl;
    // stop()后会收到所有回调，若想立即停止则调用cancel()
    ret = request->stop();
    std::cout << "stop done" << "\n" << std::endl;

    // 7: 识别结束, 释放request对象.
    if (ret == 0) {
      std::cout << "wait closed callback." << std::endl;
      struct timeval now;
      struct timespec outtime;
      gettimeofday(&now, NULL);
      outtime.tv_sec = now.tv_sec + 10;
      outtime.tv_nsec = now.tv_usec * 1000;
      // 等待closed事件后再进行释放, 否则会出现崩溃
      pthread_mutex_lock(&(cbParam->mtxWord));
      pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
      pthread_mutex_unlock(&(cbParam->mtxWord));
    } else {
      std::cout << "stop ret is " << ret << std::endl;
    }
    AlibabaNls::NlsClient::getInstance()->releaseDialogAssistantRequest(request);

    if (vectorGetFailed(cbParam->userId)) break;
  }  // while global_run

  usleep(5 * 1000 * 1000);

  // 关闭音频文件.
  fs.close();
  if (cbParam) delete cbParam;

  return NULL;
}


/**
 * 识别多个音频数据;
 * sdk多线程指一个音频数据源对应一个线程, 非一个音频数据对应多个线程.
 * 示例代码为同时开启4个线程识别4个文件;
 * 免费用户并发连接不能超过10个;
 */
#define AUDIO_FILE_NUMS 4
#define AUDIO_FILE_NAME_LENGTH 32
int dialogAssistantMultFile(const char* appkey, int threads) {
  /**
   * 获取当前系统时间戳，判断token是否过期
   */
  std::time_t curTime = std::time(0);
  if (g_expireTime - curTime < 10) {
    std::cout << "the token will be expired, please generate new token by AccessKey-ID and AccessKey-Secret." << std::endl;
    int ret = generateToken(g_akId, g_akSecret, &g_token, &g_expireTime);
    if (ret < 0) {
      std::cout << "generate token failed" << std::endl;
      return -1;
    } else {
      if (g_token.empty() || g_expireTime < 0) {
        std::cout << "generate empty token" << std::endl;
        return -2;
      }
    }
  }

#ifdef SELF_TESTING_TRIGGER
  pthread_t p_id;
  pthread_create(&p_id, NULL, &autoCloseFunc, NULL);
  pthread_detach(p_id);
#endif

  char audioFileNames[AUDIO_FILE_NUMS][AUDIO_FILE_NAME_LENGTH] =
  {
#if 0
    "dialogAssistant0.wav",
    "dialogAssistant1.wav",
    "dialogAssistant2.wav",
    "dialogAssistant3.wav"
#else
    "test0.wav",
    "test1.wav",
    "test2.wav",
    "test3.wav"
#endif
  };
  char texts[AUDIO_FILE_NUMS][AUDIO_FILE_NAME_LENGTH] =
  {
    "一杯拿铁", "我要两杯卡布奇诺", "三杯美式", "四杯摩卡"
  };

  ParamStruct pa[threads];

  for (int i = 0; i < threads; i ++) {
    int num = i % AUDIO_FILE_NUMS;
    pa[i].token = g_token;
    pa[i].appkey = appkey;
    pa[i].fileName = audioFileNames[num];
    pa[i].text = texts[num];
    if (!g_url.empty()) {
      pa[i].url = g_url;
    }
  }

  global_run = true;
  std::vector<pthread_t> pthreadId(threads);
  // 启动threads个工作线程, 同时识别threads个音频文件
  for (int j = 0; j < threads; j++) {
    pthread_create(&pthreadId[j], NULL, &AudioDialogPthreadFunc, (void *)&(pa[j])); //音频进文本出.
    //pthread_create(&pthreadId[j], NULL, &TextDialogPthreadFunc, (void *)&(pa[j])); //文本进文本出.
  }

  for (int j = 0; j < threads; j++) {
    pthread_join(pthreadId[j], NULL);
  }

  return 0;
}

int invalied_argv(int index, int argc) {
  if (index >= argc) {
    std::cout << "invalid params..." << std::endl;
    return 1;
  }
  return 0;
}

int parse_argv(int argc, char* argv[]) {
  int index = 1;
  while (index < argc) {
    if (!strcmp(argv[index], "--appkey")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_appkey = argv[index];
    } else if (!strcmp(argv[index], "--akId")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_akId = argv[index];
    } else if (!strcmp(argv[index], "--akSecret")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_akSecret = argv[index];
    } else if (!strcmp(argv[index], "--token")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_token = argv[index];
    } else if (!strcmp(argv[index], "--url")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_url = argv[index];
    } else if (!strcmp(argv[index], "--threads")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_threads = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--time")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      loop_timeout = atoi(argv[index]);
    }
    index++;
  }

  if (g_akId.empty() && getenv("NLS_AK_ENV")) {
    g_akId.assign(getenv("NLS_AK_ENV"));
  }
  if (g_akSecret.empty() && getenv("NLS_SK_ENV")) {
    g_akSecret.assign(getenv("NLS_SK_ENV"));
  }
  if (g_appkey.empty() && getenv("NLS_APPKEY_ENV")) {
    g_appkey.assign(getenv("NLS_APPKEY_ENV"));
  }

  if ((g_token.empty() && (g_akId.empty() || g_akSecret.empty())) ||
      g_appkey.empty()) {
    std::cout << "short of params..." << std::endl;
    std::cout << "if ak/sk is empty, please setenv NLS_AK_ENV&NLS_SK_ENV&NLS_APPKEY_ENV" << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (parse_argv(argc, argv)) {
    std::cout << "params is not valid.\n"
      << "Usage:\n"
      << "  --appkey <appkey>\n"
      << "  --akId <AccessKey ID>\n"
      << "  --akSecret <AccessKey Secret>\n"
      << "  --token <Token>\n"
      << "  --url <Url>\n"
      << "  --threads <Thread Numbers, default 1>\n"
      << "  --time <Timeout secs, default 60 seconds>\n"
      << "eg:\n"
      << "  ./daDemo --appkey xxxxxx --token xxxxxx\n"
      << "  ./daDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx --threads 4 --time 3600\n"
      << std::endl;
    return -1;
  }

  signal(SIGINT, signal_handler_int);
  signal(SIGQUIT, signal_handler_quit);

  std::cout << " appKey: " << g_appkey << std::endl;
  std::cout << " akId: " << g_akId << std::endl;
  std::cout << " akSecret: " << g_akSecret << std::endl;
  std::cout << " threads: " << g_threads << std::endl;
  std::cout << "\n" << std::endl;

  pthread_mutex_init(&params_mtx, NULL);

  // 根据需要设置SDK输出日志, 可选. 
  // 此处表示SDK日志输出至log-recognizer.txt, LogDebug表示输出所有级别日志.
  // 需要最早调用
  int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
      "log-dialogAssistant", AlibabaNls::LogDebug, 1000);
  if (ret < 0) {
    std::cout << "set log failed." << std::endl;
    return -1;
  }

  // 设置运行环境需要的套接口地址类型, 默认为AF_INET
  // 必须在startWorkThread()前调用
  //AlibabaNls::NlsClient::getInstance()->setAddrInFamily("AF_INET");

  // 私有云部署的情况下进行直连IP的设置
  // 必须在startWorkThread()前调用
  //AlibabaNls::NlsClient::getInstance()->setDirectHost("106.15.83.44");

  // 存在部分设备在设置了dns后仍然无法通过SDK的dns获取可用的IP,
  // 可调用此接口主动启用系统的getaddrinfo来解决这个问题.
  //AlibabaNls::NlsClient::getInstance()->setUseSysGetAddrInfo(true);

  // 启动工作线程, 在创建请求和启动前必须调用此函数
  // 入参为负时, 启动当前系统中可用的核数
  // 高并发的情况下推荐4, 单请求的情况推荐为1
  AlibabaNls::NlsClient::getInstance()->startWorkThread(1);

  // 识别多个音频数据
  ret = dialogAssistantMultFile(g_appkey.c_str(), g_threads);
  if (ret) {
    std::cout << "dialogAssistantMultFile failed." << std::endl;
    AlibabaNls::NlsClient::releaseInstance();
    pthread_mutex_destroy(&params_mtx);
    return -2;
  }

  // 所有工作完成, 进程退出前, 释放nlsClient.
  AlibabaNls::NlsClient::releaseInstance();

  int size = g_statistics.size();
  int run_count = 0;
  int success_count = 0;
  if (size > 0) {
    std::map<unsigned long, struct ParamStatistics *>::iterator it;
    std::cout << "\n" << std::endl;
    pthread_mutex_lock(&params_mtx);
    for (it = g_statistics.begin(); it != g_statistics.end(); ++it) {
      run_count++;
      if (it->second->success_flag) success_count++;

      std::cout << "pid: " << it->first
        << "; run_flag: " << it->second->running
        << "; success_flag: " << it->second->success_flag
        << "; audio_file: " << it->second->audio_ms << "ms "
        << std::endl;
      if (it->second->s_cnt > 0) {
        std::cout << "average time: "
          << (loop_timeout * 1000 / it->second->s_cnt)
          << "ms" << std::endl;
      }
    }
    pthread_mutex_unlock(&params_mtx);

    std::cout << "run count:" << run_count
      << " success count:" << success_count << std::endl;

    usleep(3000 * 1000);

    pthread_mutex_lock(&params_mtx);
    std::map<unsigned long, struct ParamStatistics *>::iterator iter;
    for (iter = g_statistics.begin(); iter != g_statistics.end();) {
      struct ParamStatistics *second = iter->second;
      if (second) {
        delete second;
        second = NULL;
      }
      g_statistics.erase(iter++);
    }
    g_statistics.clear();
    pthread_mutex_unlock(&params_mtx);
  }

  pthread_mutex_destroy(&params_mtx);

  return 0;
}
