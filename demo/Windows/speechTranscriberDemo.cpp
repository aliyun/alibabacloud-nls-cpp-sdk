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

#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "nlsClient.h"
#include "nlsEvent.h"
#include "nlsToken.h"
#include "speechTranscriberRequest.h"

#define SELF_TESTING_TRIGGER
#define FRAME_16K_20MS     640
#define FRAME_16K_100MS    3200
#define FRAME_8K_20MS      320
#define SAMPLE_RATE_8K     8000
#define SAMPLE_RATE_16K    16000

#define LOOP_TIMEOUT       60
#define DEFAULT_STRING_LEN 128

/* 自定义线程参数 */
struct ParamStruct {
  char fileName[DEFAULT_STRING_LEN];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  pthread_mutex_t mtx;
};

/* 自定义事件回调参数 */
class ParamCallBack {
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

  pthread_t userId;
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

/* 统计参数 */
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
static pthread_mutex_t params_mtx; /* 全局统计参数g_statistics的操作锁 */
static std::map<unsigned long, struct ParamStatistics*> g_statistics;
static int frame_size = FRAME_16K_100MS;
static int sample_rate = SAMPLE_RATE_16K;
static int encoder_type = ENCODER_NONE;
static int logLevel = AlibabaNls::LogDebug; /* 0:为关闭log */
static int run_cnt = 0;
static int run_success = 0;
static int run_fail = 0;

static void gettimeofday(struct timeval* tv, void* dummy) {
  FILETIME ftime;
  uint64_t n;

  GetSystemTimeAsFileTime(&ftime);
  n = (((uint64_t)ftime.dwHighDateTime << 32) + (uint64_t)ftime.dwLowDateTime);
  if (n) {
    n /= 10;
    n -= ((369 * 365 + 89) * (uint64_t)86400) * 1000000;
  }

  tv->tv_sec = n / 1000000;
  tv->tv_usec = n % 1000000;
}

std::string timestamp_str() {
  char buf[64];
  struct timeval tv;
  struct tm ltm;

  time_t clock;
  SYSTEMTIME wtm;
  GetLocalTime(&wtm);
  ltm.tm_year = wtm.wYear - 1900;
  ltm.tm_mon = wtm.wMonth - 1;
  ltm.tm_mday = wtm.wDay;
  ltm.tm_hour = wtm.wHour;
  ltm.tm_min = wtm.wMinute;
  ltm.tm_sec = wtm.wSecond;
  ltm.tm_isdst = -1;
  clock = mktime(&ltm);
  tv.tv_sec = clock;
  tv.tv_usec = wtm.wMilliseconds * 1000;

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
    /* 已经存在 */
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
    /* 已经存在 */
    iter->second->running = params.running;
    iter->second->success_flag = params.success_flag;
    iter->second->failed_flag = false;
    if (params.audio_ms > 0) {
      iter->second->audio_ms = params.audio_ms;
    }
  } else {
    /* 不存在, 新的pid */
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
    /* 已经存在 */
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
    /* 已经存在 */
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
    /* 已经存在 */
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
    /* 存在 */
    result = iter->second->running;
  } else {
    /* 不存在, 新的pid */
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
    /* 存在 */
    result = iter->second->failed_flag;
  } else {
    /* 不存在, 新的pid */
  }

  pthread_mutex_unlock(&params_mtx);
  return result;
}

/**
 * 根据AccessKey ID和AccessKey Secret重新生成一个token，
 * 并获取其有效期时间戳
 */
int generateToken(std::string akId, std::string akSecret, std::string* token,
                  long* expireTime) {
  AlibabaNlsCommon::NlsToken nlsTokenRequest;
  nlsTokenRequest.setAccessKeyId(akId);
  nlsTokenRequest.setKeySecret(akSecret);

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
 *                     非压缩数据则为1
 * @return 返回sendAudio之后需要sleep的时间
 * @note 对于8k pcm 编码数据, 16位采样，建议每发送1600字节 sleep 100 ms.
                 对于16k pcm 编码数据, 16位采样，建议每发送3200字节 sleep 100
 ms. 对于其它编码格式(OPUS)的数据, 由于传递给SDK的仍然是PCM编码数据, 按照SDK
 OPUS/OPU 数据长度限制, 需要每次发送640字节 sleep 20ms.
 */
unsigned int getSendAudioSleepTime(const int dataSize, const int sampleRate,
                                   const int compressRate) {
  /* 仅支持16位采样 */
  const int sampleBytes = 16;
  /* 仅支持单通道 */
  const int soundChannel = 1;

  /* 当前采样率, 采样位数下每秒采样数据的大小 */
  int bytes = (sampleRate * sampleBytes * soundChannel) / 8;

  /* 当前采样率, 采样位数下每毫秒采样数据的大小 */
  int bytesMs = bytes / 1000;

  /* 待发送数据大小除以每毫秒采样数据大小, 以获取sleep时间 */
  int sleepMs = (dataSize * compressRate) / bytesMs;

  return sleepMs;
}

/**
 * @brief 调用start(), 成功与云端建立连接, sdk内部线程上报started事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onTranscriptionStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "onTranscriptionStarted: "
            << "  status code: " << cbEvent->getStatusCode()
            << "  task id: " << cbEvent->getTaskId()
            << "  onTranscriptionStarted: All response:"
            << cbEvent->getAllResponse() << std::endl;

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;
    std::cout << "onTranscriptionStarted "
              << "  userId: " << tmpParam->userId.x << std::endl;

    gettimeofday(&(tmpParam->startedTv), NULL);

    // unsigned long long timeValue1 =
    //     tmpParam->startedTv.tv_sec - tmpParam->startTv.tv_sec;
    // unsigned long long timeValue2 =
    //     tmpParam->startedTv.tv_usec - tmpParam->startTv.tv_usec;
    // unsigned long long timeValue = 0;
    // if (timeValue1 > 0) {
    //   timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    // } else {
    //   timeValue = (timeValue2 / 1000);
    // }

    // pid, add, run, success
    struct ParamStatistics params;
    params.running = true;
    params.success_flag = false;
    params.audio_ms = 0;
    vectorSetParams(tmpParam->userId.x, true, params);

    /* 通知发送线程start()成功, 可以继续发送数据 */
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
  }
}

/**
 * @brief 服务端检测到了一句话的开始, sdk内部线程上报SentenceBegin事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onSentenceBegin(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
#if 1
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  std::cout << "CbParam: " << tmpParam->userId.x << ", " << tmpParam->userInfo
            << std::endl; /* 仅表示自定义参数示例 */
  std::cout
      << "onSentenceBegin: "
      << "status code: "
      << cbEvent->getStatusCode() /* 获取消息的状态码, 成功为0或者20000000,
                                     失败时对应失败的错误码 */
      << ", task id: "
      << cbEvent->getTaskId() /* 当前任务的task id, 方便定位问题，建议输出 */
      << ", index: " << cbEvent->getSentenceIndex() /* 句子编号, 从1开始递增 */
      << ", time: "
      << cbEvent->getSentenceTime() /* 当前已处理的音频时长, 单位是毫秒 */
      << std::endl;

  std::cout << "onSentenceBegin: All response:" << cbEvent->getAllResponse()
            << std::endl; /* 获取服务端返回的全部信息 */
#endif
}

/**
 * @brief 服务端检测到了一句话结束, sdk内部线程上报SentenceEnd事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onSentenceEnd(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
#if 1
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  std::cout << "CbParam: " << tmpParam->userId.x << ", " << tmpParam->userInfo
            << std::endl; /* 仅表示自定义参数示例 */

  std::cout
      << "onSentenceEnd: "
      << "status code: "
      << cbEvent
             ->getStatusCode() /* 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
                                */
      << ", task id: "
      << cbEvent->getTaskId() /* 当前任务的task id，方便定位问题，建议输出 */
      << ", result: " << cbEvent->getResult() /* 当前句子的完成识别结果 */
      << ", index: " << cbEvent->getSentenceIndex() /* 当前句子的索引编号 */
      << ", time: " << cbEvent->getSentenceTime() /* 当前句子的音频时长 */
      << ", begin_time: "
      << cbEvent->getSentenceBeginTime() /* 对应的SentenceBegin事件的时间 */
      << ", confidence: "
      << cbEvent
             ->getSentenceConfidence() /* 结果置信度,取值范围[0.0,1.0]，值越大表示置信度越高
                                        */
      << ", stashResult begin_time: "
      << cbEvent->getStashResultBeginTime() /* 下一句话开始时间 */
      << ", stashResult current_time: "
      << cbEvent->getStashResultCurrentTime() /* 下一句话当前时间 */
      << ", stashResult Sentence_id: "
      << cbEvent->getStashResultSentenceId()  // sentence Id
      << ", stashResult Text: "
      << cbEvent->getStashResultText() /* 下一句话前缀 */
      << std::endl;

  /* 这里的start_time表示调用start后开始sendAudio传递Abytes音频时
   * 发现这句话的起点. 即调用start后传递start_time出现这句话的起点.
   * 这里的end_time表示调用start后开始sendAudio传递Bbytes音频时
   * 发现这句话的结尾. 即调用start后传递end_time出现这句话的结尾.
   */
  std::cout << "onSentenceEnd: All response:" << cbEvent->getAllResponse()
            << std::endl; /* 获取服务端返回的全部信息 */
#endif
}

/**
 * @brief 识别结果发生了变化, sdk在接收到云端返回到最新结果时,
 *        sdk内部线程上报ResultChanged事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onTranscriptionResultChanged(AlibabaNls::NlsEvent* cbEvent,
                                  void* cbParam) {
  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    std::cout << "onTranscriptionResultChanged userId: " << tmpParam->userId.x
              << ", " << tmpParam->userInfo
              << std::endl; /* 仅表示自定义参数示例 */
  }

  std::cout
      << "onTranscriptionResultChanged: "
      << "status code: "
      << cbEvent
             ->getStatusCode() /* 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
                                */
      << ", task id: "
      << cbEvent->getTaskId() /* 当前任务的task id，方便定位问题，建议输出 */
      << ", result: " << cbEvent->getResult() /* 当前句子的中间识别结果 */
      << ", index: " << cbEvent->getSentenceIndex() /* 当前句子的索引编号 */
      << ", time: " << cbEvent->getSentenceTime() /* 当前句子的音频时长 */
      << std::endl;
  std::cout << "onTranscriptionResultChanged: All response:"
            << cbEvent->getAllResponse()
            << std::endl; /* 获取服务端返回的全部信息 */
}

/**
 * @brief 服务端停止实时音频流识别时, sdk内部线程上报Completed事件
 * @note 上报Completed事件之后, SDK内部会关闭识别连接通道.
                 此时调用sendAudio会返回负值, 请停止发送.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数, 默认为NULL, 可以根据需求自定义参数
 * @return
*/
void onTranscriptionCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_success++;

  std::cout
      << "onTranscriptionCompleted: "
      << " task id: " << cbEvent->getTaskId() << ", "
      << "status code: "
      << cbEvent->getStatusCode() /* 获取消息的状态码, 成功为0或者20000000,
                                     失败时对应失败的错误码 */
      << std::endl;
  std::cout << "onTranscriptionCompleted: All response:"
            << cbEvent->getAllResponse()
            << std::endl; /* 获取服务端返回的全部信息 */

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;

    std::cout << "onTranscriptionCompleted "
              << " userId: " << tmpParam->userId.x << std::endl;

    gettimeofday(&(tmpParam->completedTv), NULL);

    // unsigned long long timeValue1 =
    //     tmpParam->completedTv.tv_sec - tmpParam->startTv.tv_sec;
    // unsigned long long timeValue2 =
    //     tmpParam->completedTv.tv_usec - tmpParam->startTv.tv_usec;
    // unsigned long long timeValue = 0;
    // if (timeValue1 > 0) {
    //   timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    // } else {
    //   timeValue = (timeValue2 / 1000);
    // }

    vectorSetResult(tmpParam->userId.x, true);
  }
}

/**
 * @brief 识别过程(包含start(), sendAudio(), stop())发生异常时,
 * sdk内部线程上报TaskFailed事件
 * @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道.
 * 此时调用sendAudio会返回负值, 请停止发送
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onTaskFailed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_fail++;

  FILE* failed_stream = fopen("transcriptionTaskFailed.log", "ab");
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

  std::cout
      << "onTaskFailed: "
      << "status code: " << cbEvent->getStatusCode() << ", task id: "
      << cbEvent->getTaskId() /* 当前任务的task id, 方便定位问题，建议输出 */
      << ", error message: " << cbEvent->getErrorMessage() << std::endl;
  std::cout << "onTaskFailed: All response:" << cbEvent->getAllResponse()
            << std::endl; /* 获取服务端返回的全部信息 */

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;

    std::cout << "onTaskFailed userId " << tmpParam->userId.x << ", "
              << tmpParam->userInfo << std::endl; /* 仅表示自定义参数示例 */

    vectorSetResult(tmpParam->userId.x, false);
    vectorSetFailed(tmpParam->userId.x, true);
  }
}

/**
 * @brief 识别结束或发生异常时，会关闭连接通道,
 * sdk内部线程上报ChannelCloseed事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onChannelClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "OnChannelCloseed: All response: " << cbEvent->getAllResponse()
            << std::endl; /* getResponse() 可以通道关闭信息 */

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;

    gettimeofday(&(tmpParam->closedTv), NULL);

    // unsigned long long timeValue1 =
    //     tmpParam->closedTv.tv_sec - tmpParam->startTv.tv_sec;
    // unsigned long long timeValue2 =
    //     tmpParam->closedTv.tv_usec - tmpParam->startTv.tv_usec;
    // unsigned long long timeValue = 0;
    // if (timeValue1 > 0) {
    //   timeValue = (((timeValue1 * 1000000) + timeValue2) / 1000);
    // } else {
    //   timeValue = (timeValue2 / 1000);
    // }

    std::cout << "OnChannelCloseed: userId " << tmpParam->userId.x << ", "
              << tmpParam->userInfo << std::endl; /* 仅表示自定义参数示例 */

    /* 通知发送线程, 最终识别结果已经返回, 可以调用stop() */
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
  }
}

void* autoCloseFunc(void* arg) {
  int timeout = 50;

  while (!global_run && timeout-- > 0) {
    Sleep(100);
  }
  timeout = loop_timeout;
  while (timeout-- > 0 && global_run) {
    Sleep(1000);
  }
  global_run = false;

  return NULL;
}

/* 工作线程 */
void* pthreadFunction(void* arg) {
  int sleepMs = 0;

  ParamStruct* tst = static_cast<ParamStruct*>(arg);
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  pthread_mutex_init(&(tst->mtx), NULL);

  /* 打开音频文件, 获取数据 */
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
    vectorSetParams(pthread_self().x, true, params);
  }

  /* 退出线程前释放 */
  ParamCallBack* cbParam = NULL;
  cbParam = new ParamCallBack(tst);
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  strcpy(cbParam->userInfo, "User.");

  do {
    /*
     * 2: 创建实时音频流识别SpeechTranscriberRequest对象
     */
    AlibabaNls::SpeechTranscriberRequest* request =
        AlibabaNls::NlsClient::getInstance()->createTranscriberRequest();
    if (request == NULL) {
      std::cout << "createTranscriberRequest failed." << std::endl;
      return NULL;
    }

    /* 设置识别启动回调函数 */
    request->setOnTranscriptionStarted(onTranscriptionStarted, cbParam);
    /* 设置识别结果变化回调函数 */
    request->setOnTranscriptionResultChanged(onTranscriptionResultChanged,
                                             cbParam);
    /* 设置语音转写结束回调函数 */
    request->setOnTranscriptionCompleted(onTranscriptionCompleted, cbParam);
    /* 设置一句话开始回调函数 */
    request->setOnSentenceBegin(onSentenceBegin, cbParam);
    /* 设置一句话结束回调函数 */
    request->setOnSentenceEnd(onSentenceEnd, cbParam);
    /* 设置异常识别回调函数 */
    request->setOnTaskFailed(onTaskFailed, cbParam);
    /* 设置识别通道关闭回调函数 */
    request->setOnChannelClosed(onChannelClosed, cbParam);

    /* 设置AppKey, 必填参数, 请参照官网申请 */
    if (strlen(tst->appkey) > 0) {
      request->setAppKey(tst->appkey);
    }
    /* 设置账号校验token, 必填参数 */
    if (strlen(tst->token) > 0) {
      request->setToken(tst->token);
    }
    if (strlen(tst->url) > 0) {
      request->setUrl(tst->url);
    }

    /* 参数设置, 如指定声学模型 */
    // request->setPayloadParam("{\"model\":\"test-regression-model\"}");

    /* 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm */
    if (encoder_type == ENCODER_OPUS) {
      request->setFormat("opus");
    } else if (encoder_type == ENCODER_OPU) {
      request->setFormat("opu");
    } else {
      request->setFormat("pcm");
    }
    /* 设置音频数据采样率, 可选参数，目前支持16000, 8000. 默认是16000 */
    request->setSampleRate(sample_rate);
    /* 设置是否返回中间识别结果, 可选参数. 默认false */
    request->setIntermediateResult(true);
    /* 设置是否在后处理中添加标点, 可选参数. 默认false */
    request->setPunctuationPrediction(true);
    /* 设置是否在后处理中执行数字转写, 可选参数. 默认false */
    request->setInverseTextNormalization(true);

    /* 语音断句检测阈值，一句话之后静音长度超过该值，即本句结束，合法参数范围200～2000(ms)，默认值800ms
     */
    // request->setMaxSentenceSilence(800);
    // request->setCustomizationId("TestId_123"); /* 定制模型id, 可选. */
    // request->setVocabularyId("TestId_456"); /* 定制泛热词id, 可选. */

    fs.clear();
    fs.seekg(0, std::ios::beg);

    gettimeofday(&(cbParam->startTv), NULL);
    int ret = request->start();
    run_cnt++;
    if (ret < 0) {
      std::cout << "start() failed." << std::endl;
      /* start()失败，释放request对象 */
      AlibabaNls::NlsClient::getInstance()->releaseTranscriberRequest(request);
      break;
    }

    /* 等待started事件返回, 在发送 */
    std::cout << "wait started callback." << std::endl;
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 10;
    outtime.tv_nsec = now.tv_usec * 1000;
    pthread_mutex_lock(&(cbParam->mtxWord));
    pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
    pthread_mutex_unlock(&(cbParam->mtxWord));

    uint64_t sendAudio_us = 0;
    uint32_t sendAudio_cnt = 0;
    while (!fs.eof()) {
      const int c_frame_size = FRAME_16K_100MS;
      uint8_t data[c_frame_size];
      memset(data, 0, c_frame_size);

      fs.read((char*)data, sizeof(uint8_t) * c_frame_size);
      size_t nlen = fs.gcount();
      if (nlen == 0) {
        continue;
      }

      struct timeval tv0, tv1;
      gettimeofday(&tv0, NULL);
      /*
       * 4: 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败,
       *    需要停止发送; 返回0 为成功.
       *    notice : 返回值非成功发送字节数.
       *    若希望用省流量的opus格式上传音频数据, 则第三参数传入ENCODER_OPU
       *    ENCODER_OPU/ENCODER_OPUS模式时,nlen必须为640
       */
      ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      if (ret < 0) {
        /* 发送失败, 退出循环数据发送 */
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
      }
      gettimeofday(&tv1, NULL);
      uint64_t tmp_us =
          (tv1.tv_sec - tv0.tv_sec) * 1000000 + tv1.tv_usec - tv0.tv_usec;
      sendAudio_us += tmp_us;
      sendAudio_cnt++;

      /*
       * 语音数据发送控制：
       * 语音数据是实时的, 不用sleep控制速率, 直接发送即可.
       * 语音数据来自文件, 发送时需要控制速率,
       * 使单位时间内发送的数据大小接近单位时间原始语音数据存储的大小.
       */
      /* 根据 发送数据大小，采样率，数据压缩比 来获取sleep时间 */
      sleepMs = getSendAudioSleepTime(nlen, sample_rate, 1);

      /*
       * 5: 语音数据发送延时控制
       */
      if (sleepMs * 1000 - tmp_us > 0) {
        Sleep(sleepMs);
      }
    }  // while

    /*
     * 6: 数据发送结束，关闭识别连接通道.
     * stop()为异步操作.
     */
    if (sendAudio_cnt > 0) {
      std::cout << "sendAudio ave: " << (sendAudio_us / sendAudio_cnt) << "us"
                << std::endl;
    }
    std::cout << "stop ->" << std::endl;
    /* stop()后会收到所有回调，若想立即停止则调用cancel() */
    ret = request->stop();
    std::cout << "stop done. ret " << ret << "\n" << std::endl;

    /*
     * 7: 识别结束, 释放request对象
     */
    if (ret == 0) {
      std::cout << "wait closed callback." << std::endl;
      gettimeofday(&now, NULL);
      outtime.tv_sec = now.tv_sec + 10;
      outtime.tv_nsec = now.tv_usec * 1000;
      /* 等待closed事件后再进行释放，否则会出现崩溃 */
      pthread_mutex_lock(&(cbParam->mtxWord));
      pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
      pthread_mutex_unlock(&(cbParam->mtxWord));
    } else {
      std::cout << "ret is " << ret << std::endl;
    }
    AlibabaNls::NlsClient::getInstance()->releaseTranscriberRequest(request);

    //    if (vectorGetFailed(cbParam->userId)) break;
  } while (global_run);

  Sleep(5 * 1000);

  /* 关闭音频文件 */
  fs.close();

  pthread_mutex_destroy(&(tst->mtx));

  if (cbParam) {
    delete cbParam;
    cbParam = NULL;
  }

  return NULL;
}

/**
 * 识别多个音频数据;
 * sdk多线程指一个音频数据对应一个线程, 非一个音频数据对应多个线程.
 * 示例代码为同时开启threads个线程识别4个文件;
 * 免费用户并发连接不能超过2个;
 * notice: Linux高并发用户注意系统最大文件打开数限制, 详见README.md
 */
#define AUDIO_FILE_NUMS        4
#define AUDIO_FILE_NAME_LENGTH 1024
int speechTranscriberMultFile(const char* appkey, int threads) {
  /**
   * 获取当前系统时间戳, 判断token是否过期
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
      "D:\\code\\NlsCppSdk\\build\\build_win64\\nlsCppSdk\\x64\\Release\\test0."
      "wav",
      "D:\\code\\NlsCppSdk\\build\\build_win64\\nlsCppSdk\\x64\\Release\\test1."
      "wav",
      "D:\\code\\NlsCppSdk\\build\\build_win64\\nlsCppSdk\\x64\\Release\\test2."
      "wav",
      "D:\\code\\NlsCppSdk\\build\\build_win64\\nlsCppSdk\\x64\\Release\\test3."
      "wav"};
  const int c_threads = 2;
  ParamStruct pa[c_threads] = {0};

  for (int i = 0; i < c_threads; i++) {
    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());
    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));
    if (!g_url.empty()) {
      memset(pa[i].url, 0, DEFAULT_STRING_LEN);
      memcpy(pa[i].url, g_url.c_str(), g_url.length());
    }

    int num = i % AUDIO_FILE_NUMS;
    memset(pa[i].fileName, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].fileName, audioFileNames[num], strlen(audioFileNames[num]));
  }

  global_run = true;
  std::vector<pthread_t> pthreadId(c_threads);
  /* 启动threads个工作线程, 同时识别threads个音频文件 */
  for (int j = 0; j < c_threads; j++) {
    pthread_create(&pthreadId[j], NULL, &pthreadFunction, (void*)&(pa[j]));
  }

  for (int j = 0; j < c_threads; j++) {
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

  // unsigned long long sendTotalCount = 0;
  // unsigned long long sendTotalTime = 0;
  unsigned long long sendAveTime = 0;

  sAveTime /= c_threads;
  eAveTime /= c_threads;
  cAveTime /= c_threads;
  // if (sendTotalCount > 0) {
  //   sendAveTime = sendTotalTime / sendTotalCount;
  // }

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
        frame_size = FRAME_16K_100MS;
      } else if (strcmp(argv[index], "opu") == 0) {
        encoder_type = ENCODER_OPU;
        frame_size = FRAME_16K_20MS;
      } else if (strcmp(argv[index], "opus") == 0) {
        encoder_type = ENCODER_OPUS;
        frame_size = FRAME_16K_20MS;
      }
    } else if (!strcmp(argv[index], "--log")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      logLevel = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--sample_rate")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      sample_rate = atoi(argv[index]);
      if (sample_rate == SAMPLE_RATE_8K) {
        frame_size = FRAME_8K_20MS;
      } else if (sample_rate == SAMPLE_RATE_16K) {
        frame_size = FRAME_16K_20MS;
      }
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
  system("chcp 65001");

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
              << "  ./stDemo --appkey xxxxxx --token xxxxxx\n"
              << "  ./stDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
                 "--threads 4 --time 3600\n"
              << std::endl;
    return -1;
  }

  std::cout << " appKey: " << g_appkey << std::endl;
  std::cout << " akId: " << g_akId << std::endl;
  std::cout << " akSecret: " << g_akSecret << std::endl;
  std::cout << " threads: " << g_threads << std::endl;
  std::cout << "\n" << std::endl;

  pthread_mutex_init(&params_mtx, NULL);

  /*
   * 根据需要设置SDK输出日志, 可选.
   * 此处表示SDK日志输出至log-Transcriber.txt,  LogDebug表示输出所有级别日志
   */
  if (logLevel > 0) {
    int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
        "log-transcriber", (AlibabaNls::LogLevel)logLevel, 400, 50);
    if (ret < 0) {
      std::cout << "set log failed." << std::endl;
      return -1;
    }
  }

  std::cout << "nls version: "
            << AlibabaNls::NlsClient::getInstance()->getVersion() << std::endl;

  /*
   * 设置运行环境需要的套接口地址类型, 默认为AF_INET
   */
  // AlibabaNls::NlsClient::getInstance()->setAddrInFamily("AF_INET");

  /*
   * 启动工作线程, 在创建请求和启动前必须调用此函数
   * 入参为负时, 启动当前系统中可用的核数
   */
  AlibabaNls::NlsClient::getInstance()->startWorkThread(1);

  /* 识别多个音频数据 */
  speechTranscriberMultFile(g_appkey.c_str(), g_threads);

  /*
   * 所有工作完成，进程退出前，释放nlsClient.
   * 请注意, releaseInstance()非线程安全
  .*/
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

    Sleep(3000);

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
