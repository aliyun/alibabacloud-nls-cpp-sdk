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

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "nlsClient.h"
#include "nlsEvent.h"
#include "nlsToken.h"
#include "speechRecognizerRequest.h"

#define SELF_TESTING_TRIGGER
#define FRAME_20MS         640
#define FRAME_100MS        3200
#define SAMPLE_RATE        16000
#define DEFAULT_STRING_LEN 128

#define LOOP_TIMEOUT       60

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey Secret重新生成一个token，
 * 并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，
 * 只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
 */
// 自定义线程参数
struct ParamStruct {
  char fileName[DEFAULT_STRING_LEN];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  uint64_t startedConsumed;   /*started事件完成次数*/
  uint64_t completedConsumed; /*completed事件次数*/
  uint64_t closeConsumed;     /*closed事件次数*/

  uint64_t failedConsumed;  /*failed事件次数*/
  uint64_t requestConsumed; /*发起请求次数*/

  uint64_t sendConsumed; /*sendAudio调用次数*/

  uint64_t startTotalValue; /*所有started完成时间总和*/
  uint64_t startAveValue;   /*started完成平均时间*/
  uint64_t startMaxValue;   /*调用start()到收到started事件最大用时*/
  uint64_t startMinValue;   /*调用start()到收到started事件最小用时*/

  uint64_t endTotalValue; /*start()到completed事件的总用时*/
  uint64_t endAveValue;   /*start()到completed事件的平均用时*/
  uint64_t endMaxValue;   /*start()到completed事件的最大用时*/
  uint64_t endMinValue;   /*start()到completed事件的最小用时*/

  uint64_t closeTotalValue; /*start()到closed事件的总用时*/
  uint64_t closeAveValue;   /*start()到closed事件的平均用时*/
  uint64_t closeMaxValue;   /*start()到closed事件的最大用时*/
  uint64_t closeMinValue;   /*start()到closed事件的最小用时*/

  uint64_t sendTotalValue; /*单线程调用sendAudio总耗时*/

  uint64_t s50Value;  /*start()到started用时50ms以内*/
  uint64_t s100Value; /*start()到started用时100ms以内*/
  uint64_t s200Value;
  uint64_t s500Value;
  uint64_t s1000Value;
  uint64_t s2000Value;

  pthread_mutex_t mtx;
};

// 自定义事件回调参数
struct ParamCallBack {
 public:
  explicit ParamCallBack(ParamStruct* param) {
    userId = 0;
    memset(userInfo, 0, 8);
    tParam = param;
    pthread_mutex_init(&mtxWord, NULL);
    pthread_cond_init(&cvWord, NULL);
  };
  ~ParamCallBack() {
    tParam = NULL;
    pthread_mutex_destroy(&mtxWord);
    pthread_cond_destroy(&cvWord);
  };

  unsigned long userId;  // 这里用线程号
  char userInfo[8];

  pthread_mutex_t mtxWord;
  pthread_cond_t cvWord;

  struct timeval startTv;
  struct timeval startedTv;
  struct timeval completedTv;
  struct timeval closedTv;
  struct timeval failedTv;

  ParamStruct* tParam;
};

// 统计参数
struct ParamStatistics {
  ParamStatistics() {
    running = false;
    success_flag = false;
    failed_flag = false;
    audio_ms = 0;
    start_ms = 0;
    end_ms = 0;
    ave_ms = 0;
    s_cnt = 0;
  };

  bool running;
  bool success_flag;
  bool failed_flag;

  uint64_t audio_ms;
  uint64_t start_ms;
  uint64_t end_ms;
  uint64_t ave_ms;

  uint32_t s_cnt;
};

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
static std::map<unsigned long, struct ParamStatistics*> g_statistics;
static pthread_mutex_t params_mtx;
static int frame_size = FRAME_100MS;
static int encoder_type = ENCODER_NONE;
static int logLevel = AlibabaNls::LogDebug; /* 0:为关闭log */
static int run_cnt = 0;
static int run_success = 0;
static int run_fail = 0;

void signal_handler_int(int signo) {
  std::cout << "\nget interrupt mesg\n" << std::endl;
  global_run = false;
}
void signal_handler_quit(int signo) {
  std::cout << "\nget quit mesg\n" << std::endl;
  global_run = false;
}

std::string timestamp_str() {
  char buf[64];
  struct timeval tv;
  struct tm ltm;

  gettimeofday(&tv, NULL);
  localtime_r(&tv.tv_sec, &ltm);
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
           ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour,
           ltm.tm_min, ltm.tm_sec, tv.tv_usec);
  buf[63] = '\0';
  std::string tmp = buf;
  return tmp;
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
    std::cout << "vectorStartStore start:" << iter->second->start_ms
              << std::endl;
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
      struct ParamStatistics* p_tmp = new (struct ParamStatistics);
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
int generateToken(std::string akId, std::string akSecret, std::string* token,
                  long* expireTime) {
  AlibabaNlsCommon::NlsToken nlsTokenRequest;
  nlsTokenRequest.setAccessKeyId(akId);
  nlsTokenRequest.setKeySecret(akSecret);
  //  nlsTokenRequest.setDomain("nls-meta-vpc-pre.aliyuncs.com");

  int retCode = nlsTokenRequest.applyNlsToken();
  /*获取失败原因*/
  if (retCode < 0) {
    std::cout << "Failed error code: " << retCode
              << "  error msg: " << nlsTokenRequest.getErrorMsg() << std::endl;
    return retCode;
  }

  *token = nlsTokenRequest.getToken();
  *expireTime = nlsTokenRequest.getExpireTime();

  return 0;
}

/**
 * @brief 获取sendAudio发送延时时间
 * @param dataSize 待发送数据大小
 * @param sampleRate 采样率 16k/8K
 * @param compressRate 数据压缩率，例如压缩比为10:1的16k opus编码，此时为10；
                       非压缩数据则为1
 * @return 返回sendAudio之后需要sleep的时间
 * @note 对于8k pcm 编码数据, 16位采样，建议每发送1600字节 sleep 100 ms.
         对于16k pcm 编码数据, 16位采样，建议每发送3200字节 sleep 100 ms.
         对于其它编码格式(OPUS)的数据, 由于解码后传递给SDK的仍然是PCM编码数据,
         按照SDK OPUS/OPU 数据长度限制, 需要每次发送640字节 sleep 20ms.
 */
unsigned int getSendAudioSleepTime(const int dataSize, const int sampleRate,
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
void OnRecognitionStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    std::cout << "OnRecognitionStarted userId: " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例

    gettimeofday(&(tmpParam->startedTv), NULL);
    tmpParam->tParam->startedConsumed++;

    unsigned long long timeValue1 =
        tmpParam->startedTv.tv_sec - tmpParam->startTv.tv_sec;
    unsigned long long timeValue2 =
        tmpParam->startedTv.tv_usec - tmpParam->startTv.tv_usec;
    unsigned long long timeValue = 0;
    if (timeValue1 > 0) {
      timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    } else {
      timeValue = (timeValue2 / 1000);
    }

    // max
    if (timeValue > tmpParam->tParam->startMaxValue) {
      tmpParam->tParam->startMaxValue = timeValue;
    }

    unsigned long long tmp = timeValue;
    if (tmp <= 50) {
      tmpParam->tParam->s50Value++;
    } else if (tmp <= 100) {
      tmpParam->tParam->s100Value++;
    } else if (tmp <= 200) {
      tmpParam->tParam->s200Value++;
    } else if (tmp <= 500) {
      tmpParam->tParam->s500Value++;
    } else if (tmp <= 1000) {
      tmpParam->tParam->s1000Value++;
    } else {
      tmpParam->tParam->s2000Value++;
    }

    // min
    if (tmpParam->tParam->startMinValue == 0) {
      tmpParam->tParam->startMinValue = timeValue;
    } else {
      if (timeValue < tmpParam->tParam->startMinValue) {
        tmpParam->tParam->startMinValue = timeValue;
      }
    }

    // ave
    tmpParam->tParam->startTotalValue += timeValue;
    if (tmpParam->tParam->startedConsumed > 0) {
      tmpParam->tParam->startAveValue =
          tmpParam->tParam->startTotalValue / tmpParam->tParam->startedConsumed;
    }

    // pid, add, run, success
    struct ParamStatistics params;
    params.running = true;
    params.success_flag = false;
    params.audio_ms = 0;
    vectorSetParams(tmpParam->userId, true, params);

    //通知发送线程start()成功, 可以继续发送数据
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
  }

  std::cout
      << "OnRecognitionStarted: "
      << "status code: "
      << cbEvent
             ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << ", task id: "
      << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
      << std::endl;

  // std::cout << "OnRecognitionStarted: All response:" <<
  // cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息
}

/**
 * @brief 设置允许返回中间结果参数, sdk在接收到云端返回到中间结果时,
 *        sdk内部线程上报ResultChanged事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnRecognitionResultChanged(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  if (cbParam) {
#if 0
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    std::cout << "resultChanged CbParam: " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例
#endif
  }

#if 0
  std::cout << "OnRecognitionResultChanged: "
            << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
            << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id，方便定位问题，建议输出
            << ", result: " << cbEvent->getResult()     // 获取中间识别结果
            << std::endl;

  std::cout << "OnRecognitionResultChanged: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息
#endif
}

/**
 * @brief sdk在接收到云端返回识别结束消息时, sdk内部线程上报Completed事件
 * @note 上报Completed事件之后, SDK内部会关闭识别连接通道.
 *       此时调用sendAudio会返回负值, 请停止发送.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnRecognitionCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_success++;

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;
    std::cout << "OnRecognitionCompleted: userId " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例

    gettimeofday(&(tmpParam->completedTv), NULL);
    tmpParam->tParam->completedConsumed++;

    unsigned long long timeValue1 =
        tmpParam->completedTv.tv_sec - tmpParam->startTv.tv_sec;
    unsigned long long timeValue2 =
        tmpParam->completedTv.tv_usec - tmpParam->startTv.tv_usec;
    unsigned long long timeValue = 0;
    if (timeValue1 > 0) {
      timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    } else {
      timeValue = (timeValue2 / 1000);
    }

    // max
    if (timeValue > tmpParam->tParam->endMaxValue) {
      tmpParam->tParam->endMaxValue = timeValue;
    }
    // min
    if (tmpParam->tParam->endMinValue == 0) {
      tmpParam->tParam->endMinValue = timeValue;
    } else {
      if (timeValue < tmpParam->tParam->endMinValue) {
        tmpParam->tParam->endMinValue = timeValue;
      }
    }
    // ave
    tmpParam->tParam->endTotalValue += timeValue;
    if (tmpParam->tParam->completedConsumed > 0) {
      tmpParam->tParam->endAveValue =
          tmpParam->tParam->endTotalValue / tmpParam->tParam->completedConsumed;
    }

    vectorSetResult(tmpParam->userId, true);
  }

  std::cout
      << "OnRecognitionCompleted: "
      << "status code: "
      << cbEvent
             ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << ", task id: "
      << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
      << ", result: " << cbEvent->getResult()  // 获取中间识别结果
      << std::endl;

  std::cout << "OnRecognitionCompleted: All response:"
            << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息
}

/**
 * @brief 识别过程发生异常时, sdk内部线程上报TaskFailed事件
 * @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道.
 *       此时调用sendAudio会返回负值, 请停止发送.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnRecognitionTaskFailed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_fail++;
  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    std::cout << "taskFailed userId: " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例

    tmpParam->tParam->failedConsumed++;

    vectorSetResult(tmpParam->userId, false);
    vectorSetFailed(tmpParam->userId, true);
  }

  std::cout
      << "OnRecognitionTaskFailed: "
      << "status code: "
      << cbEvent
             ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << ", task id: "
      << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
      << ", error message: " << cbEvent->getErrorMessage() << std::endl;

  std::cout << "OnRecognitionTaskFailed: All response:"
            << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  FILE* failed_stream = fopen("recognitionTaskFailed.log", "a+");
  if (failed_stream) {
    std::string ts = timestamp_str();
    char outbuf[1024] = {0};
    snprintf(outbuf, sizeof(outbuf),
             "%s status code:%d task id:%s error mesg:%s\n", ts.c_str(),
             cbEvent->getStatusCode(), cbEvent->getTaskId(),
             cbEvent->getErrorMessage());
    fwrite(outbuf, strlen(outbuf), 1, failed_stream);
    fclose(failed_stream);
  }
}

/**
 * @brief 识别结束或发生异常时，会关闭连接通道,
 *        sdk内部线程上报ChannelCloseed事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnRecognitionChannelClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "OnRecognitionChannelClosed: All response:"
            << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息
  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    std::cout << "OnRecognitionChannelClosed CbParam: " << tmpParam->userId
              << ", " << tmpParam->userInfo
              << std::endl;  // 仅表示自定义参数示例
    vectorSetRunning(tmpParam->userId, false);

    tmpParam->tParam->closeConsumed++;
    gettimeofday(&(tmpParam->closedTv), NULL);

    unsigned long long timeValue1 =
        tmpParam->closedTv.tv_sec - tmpParam->startTv.tv_sec;
    unsigned long long timeValue2 =
        tmpParam->closedTv.tv_usec - tmpParam->startTv.tv_usec;
    unsigned long long timeValue = 0;
    if (timeValue1 > 0) {
      timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    } else {
      timeValue = (timeValue2 / 1000);
    }

    // max
    if (timeValue > tmpParam->tParam->closeMaxValue) {
      tmpParam->tParam->closeMaxValue = timeValue;
    }
    // min
    if (tmpParam->tParam->closeMinValue == 0) {
      tmpParam->tParam->closeMinValue = timeValue;
    } else {
      if (timeValue < tmpParam->tParam->closeMinValue) {
        tmpParam->tParam->closeMinValue = timeValue;
      }
    }
    // ave
    tmpParam->tParam->closeTotalValue += timeValue;
    if (tmpParam->tParam->closeConsumed > 0) {
      tmpParam->tParam->closeAveValue =
          tmpParam->tParam->closeTotalValue / tmpParam->tParam->closeConsumed;
    }

    //通知发送线程, 最终识别结果已经返回, 可以调用stop()
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
  }
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

void* pthreadFunction(void* arg) {
  int sleepMs = 0;
  ParamCallBack* cbParam = NULL;

  // 0: 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct* tst = static_cast<ParamStruct*>(arg);
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  pthread_mutex_init(&(tst->mtx), NULL);

  // 打开音频文件, 获取数据
  std::ifstream fs;
  fs.open(tst->fileName, std::ios::binary | std::ios::in);
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

  //初始化自定义回调参数, 以下两变量仅作为示例表示参数传递,
  //在demo中不起任何作用
  //回调参数在堆中分配之后, SDK在销毁requesr对象时会一并销毁, 外界无需在释放
  cbParam = new ParamCallBack(tst);
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  strcpy(cbParam->userInfo, "User.");

  while (global_run) {
    /*
     * 1: 创建一句话识别SpeechRecognizerRequest对象
     */
    AlibabaNls::SpeechRecognizerRequest* request =
        AlibabaNls::NlsClient::getInstance()->createRecognizerRequest();
    if (request == NULL) {
      std::cout << "createRecognizerRequest failed." << std::endl;
      //      return NULL;
      break;
    }

    // 设置start()成功回调函数
    request->setOnRecognitionStarted(OnRecognitionStarted, cbParam);
    // 设置异常识别回调函数
    request->setOnTaskFailed(OnRecognitionTaskFailed, cbParam);
    // 设置识别通道关闭回调函数
    request->setOnChannelClosed(OnRecognitionChannelClosed, cbParam);
    // 设置中间结果回调函数
    request->setOnRecognitionResultChanged(OnRecognitionResultChanged, cbParam);
    // 设置识别结束回调函数
    request->setOnRecognitionCompleted(OnRecognitionCompleted, cbParam);

    // 设置AppKey, 必填参数, 请参照官网申请
    if (strlen(tst->appkey) > 0) {
      request->setAppKey(tst->appkey);
      std::cout << "setAppKey: " << tst->appkey << std::endl;
    }
    // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm
    if (encoder_type == ENCODER_OPUS) {
      request->setFormat("opus");
    } else if (encoder_type == ENCODER_OPU) {
      request->setFormat("opu");
    } else {
      request->setFormat("pcm");
    }
    // 设置音频数据采样率, 可选参数, 目前支持16000, 8000. 默认是16000
    request->setSampleRate(SAMPLE_RATE);
    // 设置是否返回中间识别结果, 可选参数. 默认false
    request->setIntermediateResult(true);
    // 设置是否在后处理中添加标点, 可选参数. 默认false
    request->setPunctuationPrediction(true);
    // 设置是否在后处理中执行ITN, 可选参数. 默认false
    request->setInverseTextNormalization(true);

    //是否启动语音检测, 可选, 默认是False
    // request->setEnableVoiceDetection(true);
    //允许的最大开始静音, 可选, 单位是毫秒,
    //超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
    //注意: 需要先设置enable_voice_detection为true
    // request->setMaxStartSilence(800);
    //允许的最大结束静音, 可选, 单位是毫秒,
    //超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
    //注意: 需要先设置enable_voice_detection为true
    // request->setMaxEndSilence(800);
    // request->setCustomizationId("TestId_123"); //定制模型id, 可选.
    // request->setVocabularyId("TestId_456"); //定制泛热词id, 可选.

    // 设置账号校验token, 必填参数
    if (strlen(tst->token) > 0) {
      request->setToken(tst->token);
      std::cout << "setToken: " << tst->token << std::endl;
    }
    if (strlen(tst->url) > 0) {
      std::cout << "setUrl: " << tst->url << std::endl;
      request->setUrl(tst->url);
    }

    std::cout << "begin sendAudio. " << pthread_self() << std::endl;

    fs.clear();
    fs.seekg(0, std::ios::beg);

    /*
     * 2: start()为异步操作。成功返回started事件。失败返回TaskFailed事件。
     */
    vectorStartStore(pthread_self());
    std::cout << "start ->" << std::endl;
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&(cbParam->startTv), NULL);
    int ret = request->start();
    run_cnt++;
    if (ret < 0) {
      std::cout << "start failed(" << ret << ")." << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);
      break;
    } else {
      //等待started事件返回, 在发送
      std::cout << "wait started callback." << std::endl;
      gettimeofday(&now, NULL);
      outtime.tv_sec = now.tv_sec + 10;
      outtime.tv_nsec = now.tv_usec * 1000;
      pthread_mutex_lock(&(cbParam->mtxWord));
      pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
      pthread_mutex_unlock(&(cbParam->mtxWord));
    }

    uint64_t sendAudio_us = 0;
    uint32_t sendAudio_cnt = 0;
    while (!fs.eof()) {
      uint8_t data[frame_size];
      memset(data, 0, frame_size);

      fs.read((char*)data, sizeof(uint8_t) * frame_size);
      size_t nlen = fs.gcount();
      if (nlen == 0) {
        continue;
      }

      struct timeval tv0, tv1;
      gettimeofday(&tv0, NULL);
      /*
       * 3: 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败,
       * 需要停止发送; 返回0 为成功. notice : 返回值非成功发送字节数.
       *    若希望用省流量的opus格式上传音频数据, 则第三参数传入ENCODER_OPU
       *    ENCODER_OPU/ENCODER_OPUS模式时,nlen必须为640
       */
      ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      if (ret < 0) {
        // 发送失败, 退出循环数据发送
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
      }
      gettimeofday(&tv1, NULL);
      uint64_t tmp_us =
          (tv1.tv_sec - tv0.tv_sec) * 1000000 + tv1.tv_usec - tv0.tv_usec;
      sendAudio_us += tmp_us;
      sendAudio_cnt++;

      /*
       *语音数据发送控制：
       *语音数据是实时的, 不用sleep控制速率, 直接发送即可.
       *语音数据来自文件, 发送时需要控制速率,
       *使单位时间内发送的数据大小接近单位时间原始语音数据存储的大小.
       */
      // 根据发送数据大小，采样率，数据压缩比来获取sleep时间
      sleepMs = getSendAudioSleepTime(nlen, SAMPLE_RATE, 1);
      //      std::cout << "sleepMs:" << sleepMs << std::endl;

      /*
       * 4: 语音数据发送延时控制
       */
      if (sleepMs * 1000 - tmp_us > 0) {
        usleep(sleepMs * 1000 - tmp_us);
      }
    }  // while

    /*
     * 6: 通知云端数据发送结束.
     * stop()为异步操作.失败返回TaskFailed事件
     */
    tst->sendConsumed += sendAudio_cnt;
    tst->sendTotalValue += sendAudio_us;
    if (sendAudio_cnt > 0) {
      std::cout << "sendAudio ave: " << (sendAudio_us / sendAudio_cnt) << "us"
                << std::endl;
    }
    std::cout << "stop ->" << std::endl;
    // stop()后会收到所有回调，若想立即停止则调用cancel()
    ret = request->stop();
    std::cout << "stop done"
              << "\n"
              << std::endl;

    /*
     * 6: 通知SDK释放request.
     */
    if (ret == 0) {
      std::cout << "wait closed callback." << std::endl;
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
    AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);

    //    if (vectorGetFailed(cbParam->userId)) break;
  }  // while global_run

  pthread_mutex_destroy(&(tst->mtx));

  usleep(5 * 1000 * 1000);

  // 关闭音频文件
  fs.close();

  if (cbParam) delete cbParam;

  return NULL;
}

/**
 * 识别多个音频数据;
 * sdk多线程指一个音频数据源对应一个线程, 非一个音频数据对应多个线程.
 * 示例代码为同时开启threads个线程识别threads个文件;
 * 免费用户并发连接不能超过10个;
 * notice: Linux高并发用户注意系统最大文件打开数限制, 详见README.md
 */
#define AUDIO_FILE_NUMS        4
#define AUDIO_FILE_NAME_LENGTH 32
int speechRecognizerMultFile(const char* appkey, int threads) {
  /**
   * 获取当前系统时间戳，判断token是否过期
   */
  std::time_t curTime = std::time(0);
  if (g_token.empty()) {
    if (g_expireTime - curTime < 10) {
      std::cout << "the token will be expired, please generate new token by "
                   "AccessKey-ID and AccessKey-Secret."
                << std::endl;
      if (generateToken(g_akId, g_akSecret, &g_token, &g_expireTime) < 0) {
        return -1;
      }
    }
  }

#ifdef SELF_TESTING_TRIGGER
  pthread_t p_id;
  pthread_create(&p_id, NULL, &autoCloseFunc, NULL);
  pthread_detach(p_id);
#endif

  char audioFileNames[AUDIO_FILE_NUMS][AUDIO_FILE_NAME_LENGTH] = {
      "test0.wav", "test1.wav", "test2.wav", "test3.wav"};
  ParamStruct pa[threads];

  for (int i = 0; i < threads; i++) {
    int num = i % AUDIO_FILE_NUMS;

    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());

    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));

    memset(pa[i].fileName, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].fileName, audioFileNames[num], strlen(audioFileNames[num]));

    if (!g_url.empty()) {
      memset(pa[i].url, 0, DEFAULT_STRING_LEN);
      memcpy(pa[i].url, g_url.c_str(), g_url.length());
    }

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
  }

  global_run = true;
  std::vector<pthread_t> pthreadId(threads);
  // 启动threads个工作线程, 同时识别threads个音频文件
  for (int j = 0; j < threads; j++) {
    pthread_create(&pthreadId[j], NULL, &pthreadFunction, (void*)&(pa[j]));
  }

  for (int j = 0; j < threads; j++) {
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

  unsigned long long cMaxTime = 0;
  unsigned long long cMinTime = 0;
  unsigned long long cAveTime = 0;

  unsigned long long sendTotalCount = 0;
  unsigned long long sendTotalTime = 0;
  unsigned long long sendAveTime = 0;

  for (int i = 0; i < threads; i++) {
    sTotalCount += pa[i].startedConsumed;
    eTotalCount += pa[i].completedConsumed;
    fTotalCount += pa[i].failedConsumed;
    cTotalCount += pa[i].closeConsumed;
    rTotalCount += pa[i].requestConsumed;
    sendTotalCount += pa[i].sendConsumed;
    sendTotalTime += pa[i].sendTotalValue;  // us, 所有线程sendAudio耗时总和

    // std::cout << "Closed:" << pa[i].closeConsumed << std::endl;

    // start
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

    // end
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

    // close
    if (pa[i].closeMaxValue > cMaxTime) {
      cMaxTime = pa[i].closeMaxValue;
    }

    if (cMinTime == 0) {
      cMinTime = pa[i].closeMinValue;
    } else {
      if (pa[i].closeMinValue < cMinTime) {
        cMinTime = pa[i].closeMinValue;
      }
    }

    cAveTime += pa[i].closeAveValue;
  }

  sAveTime /= threads;
  eAveTime /= threads;
  cAveTime /= threads;
  if (sendTotalCount > 0) {
    sendAveTime = sendTotalTime / sendTotalCount;
  }

  for (int i = 0; i < threads; i++) {
    std::cout << "No." << i << " Max completed time: " << pa[i].endMaxValue
              << " ms" << std::endl;
    std::cout << "No." << i << " Min completed time: " << pa[i].endMinValue
              << " ms" << std::endl;
    std::cout << "No." << i << " Ave completed time: " << pa[i].endAveValue
              << " ms" << std::endl;

    std::cout << "No." << i << " Max closed time: " << pa[i].closeMaxValue
              << " ms" << std::endl;
    std::cout << "No." << i << " Min closed time: " << pa[i].closeMinValue
              << " ms" << std::endl;
    std::cout << "No." << i << " Ave closed time: " << pa[i].closeAveValue
              << " ms" << std::endl;
  }

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Total. " << std::endl;
  std::cout << "Request: " << rTotalCount << std::endl;
  std::cout << "Started: " << sTotalCount << std::endl;
  std::cout << "Completed: " << eTotalCount << std::endl;
  std::cout << "Failed: " << fTotalCount << std::endl;
  std::cout << "Closed: " << cTotalCount << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max started time: " << sMaxTime << " ms" << std::endl;
  std::cout << "Min started time: " << sMinTime << " ms" << std::endl;
  std::cout << "Ave started time: " << sAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Started time <= 50: " << s50Count << std::endl;
  std::cout << "Started time <= 100: " << s100Count << std::endl;
  std::cout << "Started time <= 200: " << s200Count << std::endl;
  std::cout << "Started time <= 500: " << s500Count << std::endl;
  std::cout << "Started time <= 1000: " << s1000Count << std::endl;
  std::cout << "Started time > 1000: " << s2000Count << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max completed time: " << eMaxTime << " ms" << std::endl;
  std::cout << "Min completed time: " << eMinTime << " ms" << std::endl;
  std::cout << "Ave completed time: " << eAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Ave sendAudio time: " << sendAveTime << " us" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max closed time: " << cMaxTime << " ms" << std::endl;
  std::cout << "Min closed time: " << cMinTime << " ms" << std::endl;
  std::cout << "Ave closed time: " << cAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

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
    } else if (!strcmp(argv[index], "--type")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (strcmp(argv[index], "pcm") == 0) {
        encoder_type = ENCODER_NONE;
        frame_size = FRAME_100MS;
      } else if (strcmp(argv[index], "opu") == 0) {
        encoder_type = ENCODER_OPU;
        frame_size = FRAME_20MS;
      } else if (strcmp(argv[index], "opus") == 0) {
        encoder_type = ENCODER_OPUS;
        frame_size = FRAME_20MS;
      }
    } else if (!strcmp(argv[index], "--log")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      logLevel = atoi(argv[index]);
    }
    index++;
  }
  if ((g_token.empty() && (g_akId.empty() || g_akSecret.empty())) ||
      g_appkey.empty()) {
    std::cout << "short of params..." << std::endl;
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
              << "  --type <audio type, default pcm>"
              << "  --log <logLevel, default LogDebug = 4, closeLog = 0>"
              << "eg:\n"
              << "  ./srDemo --appkey xxxxxx --token xxxxxx\n"
              << "  ./srDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
                 "--threads 4 --time 3600\n"
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
  // 此处表示SDK日志输出至log-recognizer.txt, LogDebug表示输出所有级别日志
  if (logLevel > 0) {
    int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
        "log-recognizer", (AlibabaNls::LogLevel)logLevel, 400,
        50);  //"log-recognizer"
    if (ret < 0) {
      std::cout << "set log failed." << std::endl;
      return -1;
    }
  }

  // 设置运行环境需要的套接口地址类型, 默认为AF_INET
  // AlibabaNls::NlsClient::getInstance()->setAddrInFamily("AF_INET");

  // 启动工作线程, 在创建请求和启动前必须调用此函数
  // 入参为负时, 启动当前系统中可用的核数
  AlibabaNls::NlsClient::getInstance()->startWorkThread(-1);

  // 识别多个音频数据
  speechRecognizerMultFile(g_appkey.c_str(), g_threads);

  // 所有工作完成，进程退出前，释放nlsClient.
  // 请注意, releaseInstance()非线程安全.
  AlibabaNls::NlsClient::releaseInstance();

  int size = g_statistics.size();
  if (size > 0) {
    int run_count = 0;
    int success_count = 0;
    std::map<unsigned long, struct ParamStatistics*>::iterator it;
    std::cout << "\n" << std::endl;
    pthread_mutex_lock(&params_mtx);
    for (it = g_statistics.begin(); it != g_statistics.end(); ++it) {
      run_count++;
      if (it->second->success_flag) success_count++;

      std::cout << "pid: " << it->first << "; run_flag: " << it->second->running
                << "; success_flag: " << it->second->success_flag
                << "; audio_file: " << it->second->audio_ms << "ms "
                << std::endl;
      if (it->second->s_cnt > 0) {
        std::cout << "average time: "
                  << (loop_timeout * 1000 / it->second->s_cnt) << "ms"
                  << std::endl;
      }
    }
    pthread_mutex_unlock(&params_mtx);

    std::cout << "threads run count:" << run_count
              << " success count:" << success_count << std::endl;
    std::cout << "requests run count:" << run_cnt
              << " success count:" << run_success << " fail count:" << run_fail
              << std::endl;

    usleep(3000 * 1000);

    pthread_mutex_lock(&params_mtx);
    std::map<unsigned long, struct ParamStatistics*>::iterator iter;
    for (iter = g_statistics.begin(); iter != g_statistics.end();) {
      struct ParamStatistics* second = iter->second;
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
