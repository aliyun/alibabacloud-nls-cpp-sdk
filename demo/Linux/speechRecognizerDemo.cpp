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

#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <ctime>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "nlsToken.h"
#include "speechRecognizerRequest.h"
#include "profile_scan.h"

#define SELF_TESTING_TRIGGER
#define FRAME_16K_20MS 640
#define SAMPLE_RATE_16K 16000
#define DEFAULT_STRING_LEN 512

#define LOOP_TIMEOUT 60

// 自定义线程参数
struct ParamStruct {
  char fileName[DEFAULT_STRING_LEN];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  uint64_t startedConsumed;   /*started事件完成次数*/
  uint64_t firstConsumed;     /*首包完成次数*/
  uint64_t completedConsumed; /*completed事件次数*/
  uint64_t closeConsumed;     /*closed事件次数*/

  uint64_t failedConsumed;    /*failed事件次数*/
  uint64_t requestConsumed;   /*发起请求次数*/

  uint64_t sendConsumed;      /*sendAudio调用次数*/

  uint64_t startTotalValue;   /*所有started完成时间总和*/
  uint64_t startAveValue;     /*started完成平均时间*/
  uint64_t startMaxValue;     /*调用start()到收到started事件最大用时*/
  uint64_t startMinValue;     /*调用start()到收到started事件最小用时*/

  uint64_t firstTotalValue;   /*所有收到首包用时总和*/
  uint64_t firstAveValue;     /*收到首包平均时间*/
  uint64_t firstMaxValue;     /*调用start()到收到首包最大用时*/
  uint64_t firstMinValue;     /*调用start()到收到首包最小用时*/
  bool     firstFlag;         /*是否收到首包的标记*/

  uint64_t endTotalValue;     /*start()到completed事件的总用时*/
  uint64_t endAveValue;       /*start()到completed事件的平均用时*/
  uint64_t endMaxValue;       /*start()到completed事件的最大用时*/
  uint64_t endMinValue;       /*start()到completed事件的最小用时*/

  uint64_t closeTotalValue;   /*start()到closed事件的总用时*/
  uint64_t closeAveValue;     /*start()到closed事件的平均用时*/
  uint64_t closeMaxValue;     /*start()到closed事件的最大用时*/
  uint64_t closeMinValue;     /*start()到closed事件的最小用时*/

  uint64_t sendTotalValue;    /*单线程调用sendAudio总耗时*/

  uint64_t audioFileTimeLen;  /*灌入音频文件的音频时长*/

  uint64_t s50Value;          /*start()到started用时50ms以内*/
  uint64_t s100Value;         /*start()到started用时100ms以内*/
  uint64_t s200Value;
  uint64_t s500Value;
  uint64_t s1000Value;
  uint64_t s2000Value;

  pthread_mutex_t mtx;
};

// 自定义事件回调参数
struct ParamCallBack {
 public:
  ParamCallBack(ParamStruct* param) {
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
  struct timeval firstTv;
  struct timeval completedTv;
  struct timeval closedTv;
  struct timeval failedTv;

  ParamStruct* tParam;
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
std::string g_domain = "";
std::string g_api_version = "";
std::string g_url = "";
std::string g_audio_path = "";
int g_threads = 1;
int g_cpu = 1;
int g_sync_timeout = 0;
bool g_save_audio = false;
static int loop_timeout = LOOP_TIMEOUT; /*循环运行的时间, 单位s*/
static int loop_count = 0; /*循环测试某音频文件的次数, 设置后loop_timeout无效*/

long g_expireTime = -1;
volatile static bool global_run = false;
static std::map<unsigned long, struct ParamStatistics *> g_statistics;
static pthread_mutex_t params_mtx; /*全局统计参数g_statistics的操作锁*/
static int sample_rate = SAMPLE_RATE_16K;
static int frame_size = FRAME_16K_20MS; /*每次推送音频字节数.*/
static int encoder_type = ENCODER_OPUS;
static int logLevel = AlibabaNls::LogDebug; /* 0:为关闭log */
static int run_cnt = 0;
static int run_start_failed = 0;
static int run_cancel = 0;
static int run_success = 0;
static int run_fail = 0;

static bool global_sys = true;
static PROFILE_INFO g_ave_percent;
static PROFILE_INFO g_min_percent;
static PROFILE_INFO g_max_percent;

static int profile_scan = -1;
static int cur_profile_scan = -1;
static PROFILE_INFO * g_sys_info = NULL;
static bool longConnection = false;
static bool sysAddrinfo = false;
static bool noSleepFlag = false;

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
           ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
           ltm.tm_hour, ltm.tm_min, ltm.tm_sec,
           tv.tv_usec);
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
    std::cout << "vectorStartStore start:"
              << iter->second->start_ms << std::endl;
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
int generateToken(std::string akId, std::string akSecret,
                  std::string* token, long* expireTime) {
  AlibabaNlsCommon::NlsToken nlsTokenRequest;
  nlsTokenRequest.setAccessKeyId(akId);
  nlsTokenRequest.setKeySecret(akSecret);
  if (!g_domain.empty()) {
    nlsTokenRequest.setDomain(g_domain);
  }
  if (!g_api_version.empty()) {
    nlsTokenRequest.setServerVersion(g_api_version);
  }

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

unsigned int getAudioFileTimeMs(const int dataSize,
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
  int fileMs = (dataSize * compressRate) / bytesMs;

  return fileMs;
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
unsigned int getSendAudioSleepTime(const int dataSize,
                                   const int sampleRate,
                                   const int compressRate) {
  int sleepMs = getAudioFileTimeMs(dataSize, sampleRate, compressRate);
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
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    std::cout << "OnRecognitionStarted userId: " << tmpParam->userId
            << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

    gettimeofday(&(tmpParam->startedTv), NULL);
    tmpParam->tParam->startedConsumed ++;

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

    //min
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

    // first package flag init
    tmpParam->tParam->firstFlag = false;

    // pid, add, run, success
    struct ParamStatistics params;
    params.running = true;
    params.success_flag = false;
    params.audio_ms = 0;
    vectorSetParams(tmpParam->userId, true, params);

    // 通知发送线程start()成功, 可以继续发送数据
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
  }

  std::cout << "  OnRecognitionStarted: "
            << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
            << ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
            << std::endl;

  // std::cout << "OnRecognitionStarted: All response:"
  //           << cbEvent->getAllResponse()
  //           << std::endl; // 获取服务端返回的全部信息
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
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  #if 0
    std::cout << "OnRecognitionResultChanged resultChanged CbParam: "
        << tmpParam->userId << ", "
        << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例
  #endif

    if (tmpParam->tParam->firstFlag == false) {
      tmpParam->tParam->firstConsumed++;
      tmpParam->tParam->firstFlag = true;

      gettimeofday(&(tmpParam->firstTv), NULL);

      unsigned long long timeValue1 =
          tmpParam->firstTv.tv_sec - tmpParam->startTv.tv_sec;
      unsigned long long timeValue2 =
          tmpParam->firstTv.tv_usec - tmpParam->startTv.tv_usec;
      unsigned long long timeValue = 0;
      if (timeValue1 > 0) {
        timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
      } else {
        timeValue = (timeValue2 / 1000);
      }

      // max
      if (timeValue > tmpParam->tParam->firstMaxValue) {
        tmpParam->tParam->firstMaxValue = timeValue;
      }
      // min
      if (tmpParam->tParam->firstMinValue == 0) {
        tmpParam->tParam->firstMinValue = timeValue;
      } else {
        if (timeValue < tmpParam->tParam->firstMinValue) {
          tmpParam->tParam->firstMinValue = timeValue;
        }
      }
      // ave
      tmpParam->tParam->firstTotalValue += timeValue;
      if (tmpParam->tParam->firstConsumed > 0) {
        tmpParam->tParam->firstAveValue =
            tmpParam->tParam->firstTotalValue / tmpParam->tParam->firstConsumed;
      }
    } // firstFlag
  }

  std::cout << "OnRecognitionResultChanged: "
            << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
            << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id，方便定位问题，建议输出
            << ", result: " << cbEvent->getResult()     // 获取中间识别结果
            << std::endl;
#if 0
  std::cout << "  OnRecognitionResultChanged: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息
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
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    if (!tmpParam->tParam) return;
    std::cout << "OnRecognitionCompleted: userId " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

    gettimeofday(&(tmpParam->completedTv), NULL);
    tmpParam->tParam->completedConsumed ++;

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
    if (tmpParam->tParam->completedConsumed > 0) {
      tmpParam->tParam->endAveValue =
          tmpParam->tParam->endTotalValue / tmpParam->tParam->completedConsumed;
    }

    vectorSetResult(tmpParam->userId, true);
  }

  std::cout << "  OnRecognitionCompleted: "
            << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
            << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id，方便定位问题，建议输出
            << ", result: " << cbEvent->getResult()  // 获取中间识别结果
            << std::endl;

  std::cout << "  OnRecognitionCompleted: All response:"
      << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息
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
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    std::cout << "TaskFailed userId: " << tmpParam->userId
        << ", " << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例

    tmpParam->tParam->failedConsumed ++;

    vectorSetResult(tmpParam->userId, false);
    vectorSetFailed(tmpParam->userId, true);
  }

  std::cout << "  OnRecognitionTaskFailed: "
            << "status code: " << cbEvent->getStatusCode() // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
            << ", task id: " << cbEvent->getTaskId()    // 当前任务的task id，方便定位问题，建议输出
            << ", error message: " << cbEvent->getErrorMessage()
            << std::endl;

  std::cout << "  OnRecognitionTaskFailed: All response:"
      << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息

  FILE *failed_stream = fopen("recognitionTaskFailed.log", "a+");
  if (failed_stream) {
    std::string ts = timestamp_str();
    char outbuf[1024] = {0};
    snprintf(outbuf, sizeof(outbuf),
        "%s status code:%d task id:%s error mesg:%s\n",
        ts.c_str(),
        cbEvent->getStatusCode(),
        cbEvent->getTaskId(),
        cbEvent->getErrorMessage()
        );
    fwrite(outbuf, strlen(outbuf), 1, failed_stream);
    fclose(failed_stream);
  }
}

/**
 * @brief 服务端返回的所有信息会通过此回调反馈,
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
*/
void onRecognitionMessage(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "onRecognitionMessage: All response:"
      << cbEvent->getAllResponse() << std::endl;
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
      << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息
  if (cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    if (!tmpParam->tParam) {
      std::cout << "  OnRecognitionChannelClosed tParam is nullptr" << std::endl;
      return;
    }

    std::cout << "  OnRecognitionChannelClosed CbParam: "
              << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl; // 仅表示自定义参数示例
    vectorSetRunning(tmpParam->userId, false);

    tmpParam->tParam->closeConsumed ++;
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

    //max
    if (timeValue > tmpParam->tParam->closeMaxValue) {
      tmpParam->tParam->closeMaxValue = timeValue;
    }
    //min
    if (tmpParam->tParam->closeMinValue == 0) {
      tmpParam->tParam->closeMinValue = timeValue;
    } else {
      if (timeValue < tmpParam->tParam->closeMinValue) {
        tmpParam->tParam->closeMinValue = timeValue;
      }
    }
    //ave
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

    if (g_sys_info) {
      int cur = -1;
      if (cur_profile_scan == -1) {
        cur = 0;
      } else if (cur_profile_scan == 0) {
        continue;
      } else {
        cur = cur_profile_scan;
      }
      PROFILE_INFO cur_sys_info;
      get_profile_info("srDemo", &cur_sys_info);
      std::cout << cur << ": cur_usr_name: " << cur_sys_info.usr_name
        << " CPU: " << cur_sys_info.ave_cpu_percent << "%"
        << " MEM: " << cur_sys_info.ave_mem_percent << "%"
        << std::endl;

      PROFILE_INFO *cur_info = &(g_sys_info[cur]);
      if (cur_info->ave_cpu_percent == 0) {
        strcpy(cur_info->usr_name, cur_sys_info.usr_name);
        cur_info->ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        cur_info->ave_mem_percent = cur_sys_info.ave_mem_percent;
        cur_info->eAveTime = 0;
      } else {
        if (cur_info->ave_cpu_percent < cur_sys_info.ave_cpu_percent) {
          cur_info->ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        }
        if (cur_info->ave_mem_percent < cur_sys_info.ave_mem_percent) {
          cur_info->ave_mem_percent = cur_sys_info.ave_mem_percent;
        }
      }
    }

    if (global_sys) {
      PROFILE_INFO cur_sys_info;
      get_profile_info("srDemo", &cur_sys_info);

      if (g_ave_percent.ave_cpu_percent == 0) {
        strcpy(g_ave_percent.usr_name, cur_sys_info.usr_name);
        strcpy(g_min_percent.usr_name, cur_sys_info.usr_name);
        strcpy(g_max_percent.usr_name, cur_sys_info.usr_name);

        g_ave_percent.ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        g_ave_percent.ave_mem_percent = cur_sys_info.ave_mem_percent;
        g_ave_percent.eAveTime = 0;

        g_min_percent.ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        g_min_percent.ave_mem_percent = cur_sys_info.ave_mem_percent;
        g_min_percent.eAveTime = 0;

        g_max_percent.ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        g_max_percent.ave_mem_percent = cur_sys_info.ave_mem_percent;
        g_max_percent.eAveTime = 0;
      } else {
        // record min info
        if (cur_sys_info.ave_cpu_percent < g_min_percent.ave_cpu_percent) {
          g_min_percent.ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        }
        if (cur_sys_info.ave_mem_percent < g_min_percent.ave_mem_percent) {
          g_min_percent.ave_mem_percent = cur_sys_info.ave_mem_percent;
        }
        // record max info
        if (cur_sys_info.ave_cpu_percent > g_max_percent.ave_cpu_percent) {
          g_max_percent.ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        }
        if (cur_sys_info.ave_mem_percent > g_max_percent.ave_mem_percent) {
          g_max_percent.ave_mem_percent = cur_sys_info.ave_mem_percent;
        }
        // record ave info
        g_ave_percent.ave_cpu_percent =
            (g_ave_percent.ave_cpu_percent + cur_sys_info.ave_cpu_percent) / 2;
        g_ave_percent.ave_mem_percent =
            (g_ave_percent.ave_mem_percent + cur_sys_info.ave_mem_percent) / 2;
      }
    }
  }
  global_run = false;
  std::cout << "autoCloseFunc exit..." << pthread_self() << std::endl;
  return NULL;
}

/**
 * @brief 短链接模式下工作线程
 *        以 createRecognizerRequest           <----|
 *                   |                              |
 *           request->start()                       |
 *                   |                              |
 *           request->sendAudio()                   |
 *                   |                              |
 *           request->stop()                        |
 *                   |                              |
 *           收到OnRecognitionChannelClosed回调     |
 *                   |                              |
 *           releaseRecognizerRequest(request)  ----|
 *        进行循环。
 */
void* pthreadFunction(void* arg) {
  int sleepMs = 0; // 根据发送音频数据帧长度计算sleep时间，用于模拟真实录音情景
  int testCount = 0; // 运行次数计数，用于超过设置的loop次数后退出
  ParamCallBack *cbParam = NULL;
  uint64_t sendAudio_us = 0;
  uint32_t sendAudio_cnt = 0;
  bool timedwait_flag = false;

  // 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct *tst = (ParamStruct *)arg;
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
    tst->audioFileTimeLen = getAudioFileTimeMs(len, sample_rate, 1);

    struct ParamStatistics params;
    params.running = false;
    params.success_flag = false;
    params.audio_ms = len / 640 * 20;
    vectorSetParams(pthread_self(), true, params);
  }

  // 退出线程前释放
  cbParam = new ParamCallBack(tst);
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  memset(cbParam->userInfo, 0, 8);
  strcpy(cbParam->userInfo, "User.");

  while (global_run) {
    /*
     * 1. 创建一句话识别SpeechRecognizerRequest对象
     */
    AlibabaNls::SpeechRecognizerRequest *request =
        AlibabaNls::NlsClient::getInstance()->createRecognizerRequest(
            "cpp", longConnection);
    if (request == NULL) {
      std::cout << "createRecognizerRequest failed." << std::endl;
      break;
    }

    /*
     * 2. 设置用于接收结果的回调
     */
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
    // 设置所有服务端返回信息回调函数
    //request->setOnMessage(onRecognitionMessage, cbParam);
    //request->setEnableOnMessage(true);

    /*
     * 3. 设置request的相关参数
     */
    // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm
    if (encoder_type == ENCODER_OPUS) {
      request->setFormat("opus");
    } else if (encoder_type == ENCODER_OPU) {
      request->setFormat("opu");
    } else {
      request->setFormat("pcm");
    }
    // 设置音频数据采样率, 可选参数, 目前支持16000, 8000. 默认是16000
    request->setSampleRate(sample_rate);
    // 设置是否返回中间识别结果, 可选参数. 默认false
    request->setIntermediateResult(true);
    // 设置是否在后处理中添加标点, 可选参数. 默认false
    request->setPunctuationPrediction(true);
    // 设置是否在后处理中执行ITN, 可选参数. 默认false
    request->setInverseTextNormalization(true);

    // 是否启动语音检测, 可选, 默认是False
    // request->setEnableVoiceDetection(true);

    // 允许的最大开始静音, 可选, 单位是毫秒, 
    // 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
    // 注意: 需要先设置enable_voice_detection为true
    // request->setMaxStartSilence(800);

    // 允许的最大结束静音, 可选, 单位是毫秒, 
    // 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
    // 注意: 需要先设置enable_voice_detection为true
    // request->setMaxEndSilence(800);

    // request->setCustomizationId("TestId_123"); //定制模型id, 可选.
    // request->setVocabularyId("TestId_456"); //定制泛热词id, 可选.

    // 设置AppKey, 必填参数, 请参照官网申请
    if (strlen(tst->appkey) > 0) {
      request->setAppKey(tst->appkey);
      std::cout << "setAppKey: " << tst->appkey << std::endl;
    }
    // 设置账号校验token, 必填参数
    if (strlen(tst->token) > 0) {
      request->setToken(tst->token);
      std::cout << "setToken: " << tst->token << std::endl;
    }
    if (strlen(tst->url) > 0) {
      std::cout << "setUrl: " << tst->url << std::endl;
      request->setUrl(tst->url);
    }
    // 获取返回文本的编码格式
    const char* output_format = request->getOutputFormat();
    std::cout << "text format: " << output_format << std::endl;

    std::cout << "begin sendAudio. "
      << pthread_self()
      << std::endl;

    int timeout = 40;
    bool running_flag = false;

    fs.clear();
    fs.seekg(0, std::ios::beg);

    /*
     * 4. start()为同步/异步两种操作，默认异步。由于异步模式通过回调判断request是否成功运行有修改门槛，且部分旧版本为同步接口。
     *    为了能较为平滑的更新升级SDK，提供了同步/异步两种调用方式。
     *    异步情况：默认未调用setSyncCallTimeout()的时候，start()调用立即返回，
     *            且返回值并不代表request成功开始工作，需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
     *    同步情况：调用setSyncCallTimeout()设置同步接口的超时时间，并启动同步模式。start()调用后不会立即返回，
     *            直到内部得到成功(同时也会触发started事件回调)或失败(同时也会触发TaskFailed事件回调)后返回。
     *            此方法方便旧版本SDK
     */
    vectorStartStore(pthread_self());
    std::cout << "start ->" << std::endl;
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&(cbParam->startTv), NULL);
    int ret = request->start();
    run_cnt++;
    testCount++;
    if (ret < 0) {
      std::cout << "start failed(" << ret << ")." << std::endl;
      run_start_failed++;
      AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);
      break;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 4.1. g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *      需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         * 
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        std::cout << "    wait started callback." << std::endl;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 2;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime)) {
          std::cout << "start timeout." << std::endl;
          std::cout << "current request task_id: " << request->getTaskId() << std::endl;
          timedwait_flag = true;
          pthread_mutex_unlock(&(cbParam->mtxWord));
          request->cancel();
          run_cancel++;
          AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);
          continue;;
        }
        pthread_mutex_unlock(&(cbParam->mtxWord));
        std::cout << "current request task_id:" << request->getTaskId() << " ret:" << ret << std::endl;
      } else {
        /*
         * 4.2. g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用start()
         *      返回值0即表示启动成功。
         */
      }
    }

    sendAudio_us = 0;
    sendAudio_cnt = 0;

    /*
     * 5. 从文件取音频数据循环发送音频
     */
    while (!fs.eof()) {
      uint8_t data[frame_size];
      memset(data, 0, frame_size);

      fs.read((char *)data, sizeof(uint8_t) * frame_size);
      size_t nlen = fs.gcount();
      if (nlen == 0) {
        continue;
      }
      if (g_save_audio) {
        // 以追加形式将二进制音频数据写入文件
        std::string dir = "./original_audio";
        if (access(dir.c_str(), 0) == -1) {
          mkdir(dir.c_str(), S_IRWXU);
        }
        char file_name[256] = {0};
        snprintf(file_name, 256, "%s/%s.pcm", dir.c_str(),
                 request->getTaskId());
        FILE *audio_stream = fopen(file_name, "a+");
        if (audio_stream) {
          fwrite(data, nlen, 1, audio_stream);
          fclose(audio_stream);
        }
      }

      struct timeval tv0, tv1;
      gettimeofday(&tv0, NULL);
      /*
       * 5.1. 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败, 需要停止发送;
       *      返回大于0 为成功. 
       *      若希望用省流量的opus格式上传音频数据, 则第三参数传入ENCODER_OPU/ENCODER_OPUS
       *
       * ENCODER_OPU/ENCODER_OPUS模式时, 会占用一定的CPU进行音频压缩
       */
      ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      if (ret < 0) {
        // 发送失败, 退出循环数据发送
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
      } else {
        // std::cout << "send data " << nlen << "bytes, return " << ret << " bytes." << std::endl;
      }
      gettimeofday(&tv1, NULL);
      uint64_t tmp_us =
          (tv1.tv_sec - tv0.tv_sec) * 1000000 + tv1.tv_usec - tv0.tv_usec;
      sendAudio_us += tmp_us;
      sendAudio_cnt++;

      if (noSleepFlag) {
        /*
         * 不进行sleep, 用于测试性能.
         */
      } else {
        /*
         * 实际使用中, 语音数据是实时的, 不用sleep控制速率, 直接发送即可.
         * 此处是用语音数据来自文件的方式进行模拟, 故发送时需要控制速率来模拟真实录音场景.
         */
        // 根据发送数据大小，采样率，数据压缩比来获取sleep时间
        sleepMs = getSendAudioSleepTime(nlen, sample_rate, 1);

        /*
         * 语音数据发送延时控制, 实际使用中无需sleep.
         */
        if (sleepMs * 1000 > tmp_us) {
          usleep(sleepMs * 1000 - tmp_us);
        }
      }
    }  // while - sendAudio

    tst->sendConsumed += sendAudio_cnt;
    tst->sendTotalValue += sendAudio_us;
    if (sendAudio_cnt > 0) {
      std::cout << "sendAudio ave: " << (sendAudio_us / sendAudio_cnt)
                << "us" << std::endl;
    }

    /*
     * 6. 通知云端数据发送结束.
     *    stop()为同步/异步两种操作，默认异步。由于异步模式通过回调判断request是否成功运行有修改门槛，且部分旧版本为同步接口。
     *    为了能较为平滑的更新升级SDK，提供了同步/异步两种调用方式。
     *    异步情况：默认未调用setSyncCallTimeout()的时候，stop()调用立即返回，
     *            且返回值并不代表request成功结束，需要等待返回closed事件表示结束。
     *    同步情况：调用setSyncCallTimeout()设置同步接口的超时时间，并启动同步模式。stop()调用后不会立即返回，
     *            直到内部完成工作，并触发closed事件回调后返回。
     *            此方法方便旧版本SDK。
     */
    std::cout << "stop ->" << std::endl;
    ret = request->stop(); // stop()后会收到所有回调，若想立即停止则调用cancel()
    std::cout << "stop done" << "\n" << std::endl;
    if (ret < 0) {
      std::cout << "stop failed(" << ret << ")." << std::endl;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 6.1. g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *      需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         * 
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        // 等待closed事件后再进行释放, 否则会出现崩溃
        // 若调用了setSyncCallTimeout()启动了同步调用模式, 则可以不等待closed事件。
        std::cout << "wait closed callback." << std::endl;
        /*
         * 语音服务器存在来不及处理当前请求, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 错误信息为:
         * "Gateway:IDLE_TIMEOUT:Websocket session is idle for too long time, the last directive is 'StopRecognition'!"
         * 所以需要设置一个超时机制.
         */
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 5;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime)) {
          std::cout << "stop timeout" << std::endl;
          timedwait_flag = true;
          pthread_mutex_unlock(&(cbParam->mtxWord));
          AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);
          continue;;
        }
        pthread_mutex_unlock(&(cbParam->mtxWord));
      } else {
        /*
         * 6.2. g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用stop()
         *      返回值0即表示启动成功。
         */
      }
    }

    /*
     * 7. 完成所有工作后释放当前请求。
     *    请在closed事件(确定完成所有工作)后再释放, 否则容易破坏内部状态机, 会强制卸载正在运行的请求。
     */
    AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);

    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    }
  }  // while global_run

  pthread_mutex_destroy(&(tst->mtx));

  // 关闭音频文件
  fs.close();

  if (timedwait_flag) {
    /*
     * stop超时的情况下, 会在10s后返回TaskFailed和Closed回调.
     * 若在回调前delete cbParam, 会导致回调中对cbParam的操作变成野指针操作，
     * 故若存在cbParam, 则在这里等一会
     */
    usleep(10 * 1000 * 1000);
  }

  if (cbParam) {
    delete cbParam;
    cbParam = NULL;
  }

  return NULL;
}

/**
 * @brief 长链接模式下工作线程
 *                  createRecognizerRequest
 *                          |
 *        然后以    request->start()           <-----------|
 *                          |                              |
 *                  request->sendAudio()                   |
 *                          |                              |
 *                  request->stop()                        |
 *                          |                              |
 *                  收到OnRecognitionChannelClosed回调  ---|
 *        进行循环          |
 *                  releaseRecognizerRequest(request)
 */
void* pthreadLongConnectionFunction(void* arg) {
  int sleepMs = 0; // 根据发送音频数据帧长度计算sleep时间，用于模拟真实录音情景
  int testCount = 0; // 运行次数计数，用于超过设置的loop次数后退出
  ParamCallBack *cbParam = NULL;
  struct ParamStatistics params;
  uint64_t sendAudio_us = 0;
  uint32_t sendAudio_cnt = 0;
  bool timedwait_flag = false;

  // 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct *tst = (ParamStruct *)arg;
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  // 退出线程前释放
  cbParam = new ParamCallBack(tst);
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  strcpy(cbParam->userInfo, "User.");

  pthread_mutex_init(&(tst->mtx), NULL);

  /*
   * 1. 创建一句话识别SpeechRecognizerRequest对象
   */
  AlibabaNls::SpeechRecognizerRequest *request =
      AlibabaNls::NlsClient::getInstance()->createRecognizerRequest(
          "cpp", longConnection);
  if (request == NULL) {
    std::cout << "createRecognizerRequest failed." << std::endl;
    delete cbParam;
    cbParam = NULL;
    return NULL;
  }

  /*
   * 2. 设置用于接收结果的回调
   */
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
  // 设置所有服务端返回信息回调函数
  //request->setOnMessage(onRecognitionMessage, cbParam);
  //request->setEnableOnMessage(true);

  /*
   * 3. 设置request的相关参数
   */
  // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm, 推荐opus
  if (encoder_type == ENCODER_OPUS) {
    request->setFormat("opus");
  } else if (encoder_type == ENCODER_OPU) {
    request->setFormat("opu");
  } else {
    request->setFormat("pcm");
  }
  // 设置音频数据采样率, 可选参数, 目前支持16000, 8000. 默认是16000
  request->setSampleRate(sample_rate);
  // 设置是否返回中间识别结果, 可选参数. 默认false
  request->setIntermediateResult(true);
  // 设置是否在后处理中添加标点, 可选参数. 默认false
  request->setPunctuationPrediction(true);
  // 设置是否在后处理中执行ITN, 可选参数. 默认false
  request->setInverseTextNormalization(true);

  // 是否启动语音检测, 可选, 默认是False
  //request->setEnableVoiceDetection(true);
  // 允许的最大开始静音, 可选, 单位是毫秒,
  // 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
  // 注意: 需要先设置enable_voice_detection为true
  //request->setMaxStartSilence(800);
  // 允许的最大结束静音, 可选, 单位是毫秒,
  // 超出后服务端将会发送RecognitionCompleted事件, 结束本次识别.
  // 注意: 需要先设置enable_voice_detection为true
  //request->setMaxEndSilence(800);
  //request->setCustomizationId("TestId_123"); //定制模型id, 可选.
  //request->setVocabularyId("TestId_456"); //定制泛热词id, 可选.

  // 设置AppKey, 必填参数, 请参照官网申请
  if (strlen(tst->appkey) > 0) {
    request->setAppKey(tst->appkey);
    std::cout << "setAppKey: " << tst->appkey << std::endl;
  }
  // 设置账号校验token, 必填参数
  if (strlen(tst->token) > 0) {
    request->setToken(tst->token);
    std::cout << "setToken: " << tst->token << std::endl;
  }
  if (strlen(tst->url) > 0) {
    std::cout << "setUrl: " << tst->url << std::endl;
    request->setUrl(tst->url);
  }
  // 获取返回文本的编码格式
  const char* output_format = request->getOutputFormat();
  std::cout << "text format: " << output_format << std::endl;


  /*
   * 4. 循环读音频文件，将音频数据送给request，以模拟真实录音场景。
   */
  while (global_run) {
    // 打开音频文件, 获取数据
    std::ifstream fs;
    fs.open(tst->fileName, std::ios::binary | std::ios::in);
    if (!fs) {
      std::cout << tst->fileName << " isn't exist.." << std::endl;
      break;
    } else {
      fs.seekg(0, std::ios::end);
      int len = fs.tellg();
      tst->audioFileTimeLen = getAudioFileTimeMs(len, sample_rate, 1);

      params.running = false;
      params.success_flag = false;
      params.audio_ms = len / 640 * 20;
      vectorSetParams(pthread_self(), true, params);
    }

    fs.clear();
    fs.seekg(0, std::ios::beg);

    vectorStartStore(pthread_self());
    std::cout << "start ->" << std::endl;
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&(cbParam->startTv), NULL);

    /*
     * 4.1. start()为同步/异步两种操作，默认异步。由于异步模式通过回调判断request是否成功运行有修改门槛，且部分旧版本为同步接口。
     *      为了能较为平滑的更新升级SDK，提供了同步/异步两种调用方式。
     *      异步情况：默认未调用setSyncCallTimeout()的时候，start()调用立即返回，
     *              且返回值并不代表request成功开始工作，需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
     *      同步情况：调用setSyncCallTimeout()设置同步接口的超时时间，并启动同步模式。start()调用后不会立即返回，
     *              直到内部得到成功(同时也会触发started事件回调)或失败(同时也会触发TaskFailed事件回调)后返回。
     *              此方法方便旧版本SDK
     */
    int ret = request->start();
    run_cnt++;
    testCount++;
    if (ret < 0) {
      std::cout << "start failed(" << ret << ")." << std::endl;
      run_start_failed++;
      break;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 4.1.1. g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *        需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         * 
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        std::cout << "wait started callback." << std::endl;
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 5; // 设置5s超时
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime)) {
          std::cout << "start timeout" << std::endl;
          timedwait_flag = true;
          pthread_mutex_unlock(&(cbParam->mtxWord));
          // start()调用超时，cancel()取消当次请求。
          request->cancel();
          run_cancel++;
          break;
        }
        pthread_mutex_unlock(&(cbParam->mtxWord));
      } else {
        /*
         * 4.1.2. g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用start()
         *        返回值0即表示启动成功。
         */
      }
    }

    sendAudio_us = 0;
    sendAudio_cnt = 0;

    /*
     * 4.2 从文件取音频数据循环发送音频
     */
    while (!fs.eof()) {
      uint8_t data[frame_size];
      memset(data, 0, frame_size);

      fs.read((char *)data, sizeof(uint8_t) * frame_size);
      size_t nlen = fs.gcount();
      if (nlen == 0) {
        continue;
      }
      if (g_save_audio) {
        // 以追加形式将二进制音频数据写入文件
        std::string dir = "./original_audio";
        if (access(dir.c_str(), 0) == -1) {
          mkdir(dir.c_str(), S_IRWXU);
        }
        char file_name[256] = {0};
        snprintf(file_name, 256, "%s/%s.pcm", dir.c_str(),
                 request->getTaskId());
        FILE *audio_stream = fopen(file_name, "a+");
        if (audio_stream) {
          fwrite(data, nlen, 1, audio_stream);
          fclose(audio_stream);
        }
      }

      struct timeval tv0, tv1;
      gettimeofday(&tv0, NULL);
      /*
       * 4.2.1. 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败, 需要停止发送;
       *        返回大于0 为成功. 
       *        若希望用省流量的opus格式上传音频数据, 则第三参数传入ENCODER_OPU/ENCODER_OPUS
       *
       * ENCODER_OPU/ENCODER_OPUS模式时, 会占用一定的CPU进行音频压缩
       */
      ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      if (ret < 0) {
        // 发送失败, 退出循环数据发送
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
      } else {
        // std::cout << "send data " << ret << " bytes." << std::endl;
      }
      gettimeofday(&tv1, NULL);
      uint64_t tmp_us =
          (tv1.tv_sec - tv0.tv_sec) * 1000000 + tv1.tv_usec - tv0.tv_usec;
      sendAudio_us += tmp_us;
      sendAudio_cnt++;
      
      if (noSleepFlag) {
        /*
         * 不进行sleep, 用于测试性能.
         */
      } else {
        /*
         * 实际使用中, 语音数据是实时的, 不用sleep控制速率, 直接发送即可.
         * 此处是用语音数据来自文件的方式进行模拟, 故发送时需要控制速率来模拟真实录音场景.
         */
        // 根据发送数据大小，采样率，数据压缩比来获取sleep时间
        sleepMs = getSendAudioSleepTime(nlen, sample_rate, 1);

        /*
         * 语音数据发送延时控制, 实际使用中无需sleep.
         */
        if (sleepMs * 1000 > tmp_us) {
          usleep(sleepMs * 1000 - tmp_us);
        }
      }
    }  // while - sendAudio

    // 关闭音频文件
    fs.close();

    tst->sendConsumed += sendAudio_cnt;
    tst->sendTotalValue += sendAudio_us;
    if (sendAudio_cnt > 0) {
      std::cout << "sendAudio ave: " << (sendAudio_us / sendAudio_cnt)
                << "us" << std::endl;
    }

    /*
     * 4.3. 通知云端数据发送结束.
     *      stop()为同步/异步两种操作，默认异步。由于异步模式通过回调判断request是否成功运行有修改门槛，且部分旧版本为同步接口。
     *      为了能较为平滑的更新升级SDK，提供了同步/异步两种调用方式。
     *      异步情况：默认未调用setSyncCallTimeout()的时候，stop()调用立即返回，
     *              且返回值并不代表request成功结束，需要等待返回closed事件表示结束。
     *      同步情况：调用setSyncCallTimeout()设置同步接口的超时时间，并启动同步模式。stop()调用后不会立即返回，
     *              直到内部完成工作，并触发closed事件回调后返回。
     *              此方法方便旧版本SDK。
     */
    std::cout << "stop ->" << std::endl; // stop()后会收到所有回调，若想立即停止则调用cancel()
    ret = request->stop();
    std::cout << "stop done" << "\n" << std::endl;
    if (ret < 0) {
      std::cout << "stop failed(" << ret << ")." << std::endl;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 4.3.1. g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *        需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         * 
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        // 等待closed事件后再进行释放, 否则会出现崩溃
        // 若调用了setSyncCallTimeout()启动了同步调用模式, 则可以不等待closed事件。
        std::cout << "wait closed callback." << std::endl;
        /*
         * 语音服务器存在来不及处理当前请求, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 错误信息为:
         * "Gateway:IDLE_TIMEOUT:Websocket session is idle for too long time, the last directive is 'StopRecognition'!"
         * 所以需要设置一个超时机制.
         */
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + 5;
        outtime.tv_nsec = now.tv_usec * 1000;
        // 等待closed事件后再进行释放, 否则会出现崩溃
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime)) {
          std::cout << "stop timeout" << std::endl;
          timedwait_flag = true;
          pthread_mutex_unlock(&(cbParam->mtxWord));
          break;
        }
        pthread_mutex_unlock(&(cbParam->mtxWord));
      } else {
        /*
         * 4.3.2. g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用stop()
         *        返回值0即表示启动成功。
         */
      }
    }

    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    }
  } // while

  /*
   * 5. 完成所有工作后释放当前请求。
   *    请在closed事件(确定完成所有工作)后再释放, 否则容易破坏内部状态机, 会强制卸载正在运行的请求。
   */
  AlibabaNls::NlsClient::getInstance()->releaseRecognizerRequest(request);
  request = NULL;

  pthread_mutex_destroy(&(tst->mtx));

  if (timedwait_flag) {
    /*
     * stop超时的情况下, 会在10s后返回TaskFailed和Closed回调.
     * 若在回调前delete cbParam, 会导致回调中对cbParam的操作变成野指针操作，
     * 故若存在cbParam, 则在这里等一会
     */
    usleep(10 * 1000 * 1000);
  }

  if (cbParam) {
    delete cbParam;
    cbParam = NULL;
  }

  return NULL;
}

/**
 * 识别多个音频数据;
 * sdk多线程指一个音频数据源对应一个线程, 非一个音频数据对应多个线程.
 * 示例代码为同时开启threads个线程识别threads个文件;
 * 免费用户并发连接不能超过10个;
 * notice: Linux高并发用户注意系统最大文件打开数限制, 详见README.md
 */
#define AUDIO_FILE_NUMS 4
#define AUDIO_FILE_NAME_LENGTH 32
int speechRecognizerMultFile(const char* appkey, int threads) {
  /**
   * 获取当前系统时间戳，判断token是否过期
   */
  std::time_t curTime = std::time(0);
  if (g_token.empty()) {
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
  }

#ifdef SELF_TESTING_TRIGGER
  if (loop_count == 0) {
    pthread_t p_id;
    pthread_create(&p_id, NULL, &autoCloseFunc, NULL);
    pthread_detach(p_id);
  }
#endif

  char audioFileNames[AUDIO_FILE_NUMS][AUDIO_FILE_NAME_LENGTH] =
  {
    "test0.wav", "test1.wav", "test2.wav", "test3.wav"
  };
  ParamStruct pa[threads];

  // init ParamStruct
  for (int i = 0; i < threads; i ++) {
    int num = i % AUDIO_FILE_NUMS;

    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());

    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));

    memset(pa[i].fileName, 0, DEFAULT_STRING_LEN);
    if (g_audio_path.empty()) {
      strncpy(pa[i].fileName, audioFileNames[num], strlen(audioFileNames[num]));
    } else {
      strncpy(pa[i].fileName, g_audio_path.c_str(), DEFAULT_STRING_LEN);
    }

    memset(pa[i].url, 0, DEFAULT_STRING_LEN);
    if (!g_url.empty()) {
      memcpy(pa[i].url, g_url.c_str(), g_url.length());
    }

    pa[i].startedConsumed = 0;
    pa[i].firstConsumed = 0;
    pa[i].completedConsumed = 0;
    pa[i].closeConsumed = 0;
    pa[i].failedConsumed = 0;
    pa[i].requestConsumed = 0;
    pa[i].sendConsumed = 0;

    pa[i].startTotalValue = 0;
    pa[i].startAveValue = 0;
    pa[i].startMaxValue = 0;
    pa[i].startMinValue = 0;

    pa[i].firstTotalValue = 0;
    pa[i].firstAveValue = 0;
    pa[i].firstMaxValue = 0;
    pa[i].firstMinValue = 0;
    pa[i].firstFlag = false;

    pa[i].endTotalValue = 0;
    pa[i].endAveValue = 0;
    pa[i].endMaxValue = 0;
    pa[i].endMinValue = 0;

    pa[i].closeTotalValue = 0;
    pa[i].closeAveValue = 0;
    pa[i].closeMaxValue = 0;
    pa[i].closeMinValue = 0;
    pa[i].sendTotalValue = 0;

    pa[i].audioFileTimeLen = 0;

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
    if (longConnection) {
      pthread_create(&pthreadId[j], NULL, &pthreadLongConnectionFunction, (void *)&(pa[j]));
    } else {
      pthread_create(&pthreadId[j], NULL, &pthreadFunction, (void *)&(pa[j]));
    }
  }

  for (int j = 0; j < threads; j++) {
    pthread_join(pthreadId[j], NULL);
  }

  unsigned long long sTotalCount = 0; /*started总次数*/
  unsigned long long iTotalCount = 0; /*首包总次数*/
  unsigned long long eTotalCount = 0; /*completed总次数*/
  unsigned long long fTotalCount = 0; /*failed总次数*/
  unsigned long long cTotalCount = 0; /*closed总次数*/
  unsigned long long rTotalCount = 0; /*总请求数*/

  unsigned long long sMaxTime = 0;
  unsigned long long sMinTime = 0;
  unsigned long long sAveTime = 0;

  unsigned long long fMaxTime = 0; /*首包最大耗时*/
  unsigned long long fMinTime = 0; /*首包最小耗时*/
  unsigned long long fAveTime = 0; /*首包平均耗时*/

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

  unsigned long long audioFileAveTimeLen = 0;

  for (int i = 0; i < threads; i ++) {
    sTotalCount += pa[i].startedConsumed;
    iTotalCount += pa[i].firstConsumed;
    eTotalCount += pa[i].completedConsumed;
    fTotalCount += pa[i].failedConsumed;
    cTotalCount += pa[i].closeConsumed;
    rTotalCount += pa[i].requestConsumed;
    sendTotalCount += pa[i].sendConsumed;
    sendTotalTime += pa[i].sendTotalValue; // us, 所有线程sendAudio耗时总和
    audioFileAveTimeLen += pa[i].audioFileTimeLen;

    //std::cout << "Closed:" << pa[i].closeConsumed << std::endl;

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

    // first pack
    if (pa[i].firstMaxValue > fMaxTime) {
      fMaxTime = pa[i].firstMaxValue;
    }

    if (fMinTime == 0) {
      fMinTime = pa[i].firstMinValue;
    } else {
      if (pa[i].firstMinValue < fMinTime) {
        fMinTime = pa[i].firstMinValue;
      }
    }

    fAveTime += pa[i].firstAveValue;

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

    //close
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
  fAveTime /= threads;
  audioFileAveTimeLen /= threads;

  int cur = -1;
  if (cur_profile_scan == -1) {
    cur = 0;
  } else if (cur_profile_scan == 0) {
  } else {
    cur = cur_profile_scan;
  }
  if (g_sys_info && cur >= 0 && cur_profile_scan != 0) {
    PROFILE_INFO *cur_info = &(g_sys_info[cur]);
    cur_info->eAveTime = eAveTime;
  }

  if (sendTotalCount > 0) {
    sendAveTime = sendTotalTime / sendTotalCount;
  }

  for (int i = 0; i < threads; i ++) {
    std::cout << "-----" << std::endl;
    std::cout << "No." << i
      << " Max started time: " << pa[i].startMaxValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Min started time: " << pa[i].startMinValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Ave started time: " << pa[i].startAveValue << " ms"
      << std::endl;

    std::cout << "No." << i
      << " Max first package time: " << pa[i].firstMaxValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Min first package time: " << pa[i].firstMinValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Ave first package time: " << pa[i].firstAveValue << " ms"
      << std::endl;

    std::cout << "No." << i
      << " Max completed time: " << pa[i].endMaxValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Min completed time: " << pa[i].endMinValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Ave completed time: " << pa[i].endAveValue << " ms"
      << std::endl;

    std::cout << "No." << i
      << " Max closed time: " << pa[i].closeMaxValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Min closed time: " << pa[i].closeMinValue << " ms"
      << std::endl;
    std::cout << "No." << i
      << " Ave closed time: " << pa[i].closeAveValue << " ms"
      << std::endl;

    std::cout << "No." << i
      << " Audio File duration: " << pa[i].audioFileTimeLen << " ms"
      << std::endl;
  }

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Total. " << std::endl;
  std::cout << "Final Request: " << rTotalCount << std::endl;
  std::cout << "Final Started: " << sTotalCount << std::endl;
  std::cout << "Final Completed: " << eTotalCount << std::endl;
  std::cout << "Final Failed: " << fTotalCount << std::endl;
  std::cout << "Final Closed: " << cTotalCount << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Max started time: " << sMaxTime << " ms" << std::endl;
  std::cout << "Final Min started time: " << sMinTime << " ms" << std::endl;
  std::cout << "Final Ave started time: " << sAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Started time <= 50 ms: " << s50Count << std::endl;
  std::cout << "Started time <= 100 ms: " << s100Count << std::endl;
  std::cout << "Started time <= 200 ms: " << s200Count << std::endl;
  std::cout << "Started time <= 500 ms: " << s500Count << std::endl;
  std::cout << "Started time <= 1000 ms: " << s1000Count << std::endl;
  std::cout << "Started time > 1000 ms: " << s2000Count << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max first package time: " << fMaxTime << " ms" << std::endl;
  std::cout << "Min first package time: " << fMinTime << " ms" << std::endl;
  std::cout << "Ave first package time: " << fAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Max completed time: " << eMaxTime << " ms" << std::endl;
  std::cout << "Final Min completed time: " << eMinTime << " ms" << std::endl;
  std::cout << "Final Ave completed time: " << eAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Ave sendAudio time: " << sendAveTime << " us" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max closed time: " << cMaxTime << " ms" << std::endl;
  std::cout << "Min closed time: " << cMinTime << " ms" << std::endl;
  std::cout << "Ave closed time: " << cAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Ave audio file duration: " << audioFileAveTimeLen
            << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  usleep(2 * 1000 * 1000);

  std::cout << "speechRecognizerMultFile exit..." << std::endl;
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
    } else if (!strcmp(argv[index], "--tokenDomain")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_domain = argv[index];
    } else if (!strcmp(argv[index], "--tokenApiVersion")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_api_version = argv[index];
    } else if (!strcmp(argv[index], "--url")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_url = argv[index];
    } else if (!strcmp(argv[index], "--threads")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_threads = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--cpu")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_cpu = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--time")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      loop_timeout = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--loop")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      loop_count = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--type")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (strcmp(argv[index], "pcm") == 0) {
        encoder_type = ENCODER_NONE;
      } else if (strcmp(argv[index], "opu") == 0) {
        encoder_type = ENCODER_OPU;
      } else if (strcmp(argv[index], "opus") == 0) {
        encoder_type = ENCODER_OPUS;
      }
    } else if (!strcmp(argv[index], "--log")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      logLevel = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--sampleRate")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      sample_rate = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--frameSize")) {
      index++;
      frame_size = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--save")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        g_save_audio = true;
      } else {
        g_save_audio = false;
      }
    } else if (!strcmp(argv[index], "--NlsScan")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      profile_scan = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--long")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        longConnection = true;
      } else {
        longConnection = false;
      }
    } else if (!strcmp(argv[index], "--sys")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        sysAddrinfo = true;
      } else {
        sysAddrinfo = false;
      }
    } else if (!strcmp(argv[index], "--noSleep")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        noSleepFlag = true;
      } else {
        noSleepFlag = false;
      }
    } else if (!strcmp(argv[index], "--audioFile")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_audio_path = argv[index];
    } else if (!strcmp(argv[index], "--sync_timeout")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_sync_timeout = atoi(argv[index]);
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
    std::cout
        << "params is not valid.\n"
        << "Usage:\n"
        << "  --appkey <appkey>\n"
        << "  --akId <AccessKey ID>\n"
        << "  --akSecret <AccessKey Secret>\n"
        << "  --token <Token>\n"
        << "  --tokenDomain <the domain of token>\n"
        << "      mcos: mcos.cn-shanghai.aliyuncs.com\n"
        << "  --tokenApiVersion <the ApiVersion of token>\n"
        << "      mcos:  2022-08-11\n"
        << "  --url <the url of NLS Server>\n"
        << "      public(default): "
           "wss://nls-gateway.cn-shanghai.aliyuncs.com/ws/v1\n"
        << "      internal: "
           "ws://nls-gateway.cn-shanghai-internal.aliyuncs.com/ws/v1\n"
        << "      mcos: wss://mcos-cn-shanghai.aliyuncs.com/ws/v1\n"
        << "  --threads <Thread Numbers, default 1>\n"
        << "  --time <Timeout secs, default 60 seconds>\n"
        << "  --type <audio type, default pcm>\n"
        << "  --log <logLevel, default LogDebug = 4, closeLog = 0>\n"
        << "  --sampleRate <sample rate, 16K or 8K>\n"
        << "  --long <long connection: 1, short connection: 0, default 0>\n"
        << "  --sys <use system getaddrinfo(): 1, evdns_getaddrinfo(): 0>\n"
        << "  --noSleep <use sleep after sendAudio(), default 0>\n"
        << "  --audioFile <the absolute path of audio file>\n"
        << "  --frameSize <audio data size, 640 ~ 16384bytes>\n"
        << "  --save <save input audio flag, default 0>\n"
        << "  --loop <loop count>\n"
        << "  --sync_timeout <Use sync invoke, set timeout_ms, default 0, "
           "invoke is async.>\n"
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
  std::cout << " domain for token: " << g_domain << std::endl;
  std::cout << " apiVersion for token: " << g_api_version << std::endl;
  std::cout << " threads: " << g_threads << std::endl;
  if (!g_audio_path.empty()) {
    std::cout << " audio files path: " << g_audio_path << std::endl;
  }
  std::cout << " loop timeout: " << loop_timeout << std::endl;
  std::cout << " loop count: " << loop_count << std::endl;
  std::cout << "\n" << std::endl;

  pthread_mutex_init(&params_mtx, NULL);

  if (profile_scan > 0) {
    g_sys_info = new PROFILE_INFO[profile_scan + 1];
    memset(g_sys_info, 0, sizeof(PROFILE_INFO) * (profile_scan + 1));

    // 启动 profile扫描, 同时关闭sys数据打印
    global_sys = false;
  } else {
    // 不进行性能扫描时, profile_scan赋为0, cur_profile_scan默认-1,
    // 即后续只跑一次startWorkThread
    profile_scan = 0;
  }

  for (cur_profile_scan = -1;
       cur_profile_scan < profile_scan;
       cur_profile_scan++) {

    if (cur_profile_scan == 0) continue;

    // 根据需要设置SDK输出日志, 可选.
    // 此处表示SDK日志输出至log-recognizer.txt, LogDebug表示输出所有级别日志
    // 需要最早调用
    if (logLevel > 0) {
      int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
          "log-recognizer", (AlibabaNls::LogLevel)logLevel, 400, 50); //"log-recognizer"
      if (ret < 0) {
        std::cout << "set log failed." << std::endl;
        return -1;
      }
    }

    // 设置运行环境需要的套接口地址类型, 默认为AF_INET
    // 必须在startWorkThread()前调用
    //AlibabaNls::NlsClient::getInstance()->setAddrInFamily("AF_INET");

    // 私有云部署的情况下进行直连IP的设置
    // 必须在startWorkThread()前调用
    //AlibabaNls::NlsClient::getInstance()->setDirectHost("106.15.83.44");

    // 存在部分设备在设置了dns后仍然无法通过SDK的dns获取可用的IP,
    // 可调用此接口主动启用系统的getaddrinfo来解决这个问题.
    if (sysAddrinfo) {
      AlibabaNls::NlsClient::getInstance()->setUseSysGetAddrInfo(true);
    }

    // g_sync_timeout等于0，即默认未调用setSyncCallTimeout()
    // 异步方式调用
    //   start(): 需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
    //   stop(): 需要等待返回closed事件则表示完成此次交互。
    // 同步方式调用
    //   start()/stop() 调用返回即表示交互启动/结束。
    if (g_sync_timeout > 0) {
      AlibabaNls::NlsClient::getInstance()->setSyncCallTimeout(g_sync_timeout);
    }

    std::cout << "startWorkThread begin... " << std::endl;

    // 启动工作线程, 在创建请求和启动前必须调用此函数
    // 入参为负时, 启动当前系统中可用的核数
    if (cur_profile_scan == -1) {
      // 高并发的情况下推荐4, 单请求的情况推荐为1
      // 若高并发CPU占用率较高, 则可填-1启用所有CPU核
      AlibabaNls::NlsClient::getInstance()->startWorkThread(g_cpu);
    } else {
      AlibabaNls::NlsClient::getInstance()->startWorkThread(cur_profile_scan);
    }

    std::cout << "startWorkThread finish" << std::endl;

    // 识别多个音频数据
    int ret = speechRecognizerMultFile(g_appkey.c_str(), g_threads);
    if (ret) {
      std::cout << "speechRecognizerMultFile failed." << std::endl;
      AlibabaNls::NlsClient::releaseInstance();
      break;
    }

    // 所有工作完成，进程退出前，释放nlsClient.
    // 请注意, releaseInstance()非线程安全.
    AlibabaNls::NlsClient::releaseInstance();

    int size = g_statistics.size();
    int success_count = 0;
    if (size > 0) {
      std::map<unsigned long, struct ParamStatistics *>::iterator it;
      std::cout << "\n" << std::endl;

      std::cout << "Threads count:" << g_threads
        << ", Requests count:" << run_cnt << std::endl;
      std::cout << "    success:" << run_success
        << " cancel:" << run_cancel
        << " fail:" << run_fail
        << " start_failed:" << run_start_failed << std::endl;

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

    run_cnt = 0;
    run_start_failed = 0;
    run_success = 0;
    run_fail = 0;
    std::cout << "===============================" << std::endl;
  }  // for

  if (g_sys_info) {
    int k = 0;
    for (k = 0; k < profile_scan + 1; k++) {
      PROFILE_INFO *cur_info = &(g_sys_info[k]);
      if (k == 0) {
        std::cout << "WorkThread: " << k - 1
          << " USER: " << cur_info->usr_name
          << " CPU: " << cur_info->ave_cpu_percent << "% "
          << " MEM: " << cur_info->ave_mem_percent << "% "
          << " Average Time: " << cur_info->eAveTime << "ms"
          << std::endl;
      } else {
        std::cout << "WorkThread: " << k
          << " USER: " << cur_info->usr_name
          << " CPU: " << cur_info->ave_cpu_percent << "% "
          << " MEM: " << cur_info->ave_mem_percent << "% "
          << " Average Time: " << cur_info->eAveTime << "ms"
          << std::endl;
      }
    }

    delete[] g_sys_info;
    g_sys_info = NULL;
  }

  if (global_sys) {
    std::cout << "WorkThread: " << g_cpu << std::endl;
    std::cout << "  USER: " << g_ave_percent.usr_name << std::endl;
    std::cout << "    Min: " << std::endl;
    std::cout << "      CPU: " << g_min_percent.ave_cpu_percent
      << " %" << std::endl;
    std::cout << "      MEM: " << g_min_percent.ave_mem_percent
      << " %" << std::endl;
    std::cout << "    Max: " << std::endl;
    std::cout << "      CPU: " << g_max_percent.ave_cpu_percent
      << " %" << std::endl;
    std::cout << "      MEM: " << g_max_percent.ave_mem_percent
      << " %" << std::endl;
    std::cout << "    Average: " << std::endl;
    std::cout << "      CPU: " << g_ave_percent.ave_cpu_percent
      << " %" << std::endl;
    std::cout << "      MEM: " << g_ave_percent.ave_mem_percent
      << " %" << std::endl;
    std::cout << "===============================" << std::endl;
  }

  pthread_mutex_destroy(&params_mtx);

  return 0;
}
