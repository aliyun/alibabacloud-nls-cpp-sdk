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
#include <pthread.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "nlsToken.h"
#include "speechSynthesizerRequest.h"
#include "profile_scan.h"

#define SELF_TESTING_TRIGGER
#define SAMPLE_RATE_16K 16000
#define LOOP_TIMEOUT 60
#define LOG_TRIGGER
#define DEFAULT_STRING_LEN 512
#define AUDIO_TEXT_LENGTH 1024

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey Secret重新生成一个token，并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
 */
// 自定义线程参数
struct ParamStruct {
  char text[AUDIO_TEXT_LENGTH];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char audioFile[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  uint64_t startedConsumed;   /*started事件完成次数*/
  uint64_t firstConsumed;     /*首包完成次数*/
  uint64_t completedConsumed; /*completed事件次数*/
  uint64_t closeConsumed;     /*closed事件次数*/

  uint64_t failedConsumed;    /*failed事件次数*/
  uint64_t requestConsumed;   /*发起请求次数*/

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

  uint64_t audioTotalDuration;/*取到音频文件的音频总时长*/
  uint64_t audioAveDuration;  /*取到音频文件的音频平均时长*/

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

std::string g_appkey = "";
std::string g_akId = "";
std::string g_akSecret = "";
std::string g_token = "";
std::string g_domain = "";
std::string g_api_version = "";
std::string g_url = "";
int g_threads = 1;
int g_cpu = 1;
bool g_save_audio = false;
std::string g_format = "wav";
static int loop_timeout = LOOP_TIMEOUT; /*循环运行的时间, 单位s*/
static int loop_count = 0; /*循环测试某音频文件的次数, 设置后loop_timeout无效*/

long g_expireTime = -1;
volatile static bool global_run = false;
static std::map<unsigned long, struct ParamStatistics *> g_statistics;
static int sample_rate = SAMPLE_RATE_16K;
static pthread_mutex_t params_mtx; /*全局统计参数g_statistics的操作锁*/
static int run_cnt = 0;
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

/**
 * @brief 音频数据长转成对应时长
 * @param dataSize 待发送数据大小
 * @param sampleRate 采样率 16k/8K
 * @param compressRate 数据压缩率，例如压缩比为10:1的16k opus编码，此时为10；
 *                     非压缩数据则为1
 * @return 返回sendAudio之后需要sleep的时间
 * @note 对于8k pcm 编码数据, 16位采样，建议每发送1600字节 sleep 100 ms.
         对于16k pcm 编码数据, 16位采样，建议每发送3200字节 sleep 100 ms.
         对于其它编码格式(OPUS)的数据, 由于传递给SDK的仍然是PCM编码数据,
         按照SDK OPUS/OPU 数据长度限制, 需要每次发送640字节 sleep 20ms.
 */
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
 * @brief sdk在接收到云端返回合成结束消息时, sdk内部线程上报Completed事件
 * @note 上报Completed事件之后，SDK内部会关闭识别连接通道.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnSynthesisCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_success++;
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

  std::string ts = timestamp_str();
  std::cout << "OnSynthesisCompleted: " << ts.c_str()
    << ", userId: " << tmpParam->userId
    << ", status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
    << ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
    << std::endl;
  //std::cout << "OnSynthesisCompleted: All response:"
  //          << cbEvent->getAllResponse()
  //          << std::endl; // 获取服务端返回的全部信息

  if (cbParam && tmpParam->tParam) {
    gettimeofday(&(tmpParam->completedTv), NULL);
    tmpParam->tParam->completedConsumed ++;

    unsigned long long timeValue1 =
        tmpParam->startTv.tv_sec * 1000000 + tmpParam->startTv.tv_usec;
    unsigned long long timeValue2 =
        tmpParam->completedTv.tv_sec * 1000000 + tmpParam->completedTv.tv_usec;
    unsigned long long timeValue = (timeValue2 - timeValue1) / 1000; // ms
    if (timeValue > 0) {
      if (timeValue >= 5000) {
        std::cout << "task id: "<< cbEvent->getTaskId()
                  <<", abnormal request, timestamp: " << timeValue
                  << "ms." << std::endl;
      }
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

    if (tmpParam->tParam->completedConsumed > 0) {
      tmpParam->tParam->audioAveDuration =
          tmpParam->tParam->audioTotalDuration / tmpParam->tParam->completedConsumed;
    }

    vectorSetResult(tmpParam->userId, true);
  }
}

/**
 * @brief 合成过程发生异常时, sdk内部线程上报TaskFailed事件
 * @note 上报TaskFailed事件之后，SDK内部会关闭识别连接通道.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnSynthesisTaskFailed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_fail++;

  FILE *failed_stream = fopen("synthesisTaskFailed.log", "a+");
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

  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

  std::cout << "OnSynthesisTaskFailed userId: " << tmpParam->userId
    << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
    << ", task id: " << cbEvent->getTaskId()   // 当前任务的task id，方便定位问题，建议输出
    << ", error message: " << cbEvent->getErrorMessage()
    << std::endl;
  std::cout << "OnSynthesisTaskFailed: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息

  if (cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    tmpParam->tParam->failedConsumed ++;

    vectorSetResult(tmpParam->userId, false);
    vectorSetFailed(tmpParam->userId, true);
  }
}

/**
 * @brief 识别结束或发生异常时，会关闭连接通道, sdk内部线程上报ChannelCloseed事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnSynthesisChannelClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

  std::string ts = timestamp_str();
  std::cout << "OnSynthesisChannelClosed: " << ts.c_str()
      << ", userId: " << tmpParam->userId
      << ", All response: " << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息

//  tmpParam->audioFile.close();
//  delete tmpParam; //识别流程结束,释放回调参数

  if (cbParam && tmpParam->tParam) {
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

/**
 * @brief 文本上报服务端之后, 收到服务端返回的二进制音频数据, SDK内部线程通过BinaryDataRecved事件上报给用户
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 * @notice 此处切记不可做block操作,只可做音频数据转存. 若在此回调中做过多操作,
 *         会阻塞后续的数据回调和completed事件回调.
 */
void OnBinaryDataRecved(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;

  std::vector<unsigned char> data = cbEvent->getBinaryData(); // getBinaryData() 获取文本合成的二进制音频数据
#if 0
  std::string ts = timestamp_str();
  std::cout << "OnBinaryDataRecved: " << ts.c_str() << ", "
    << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
    << ", userId: " << tmpParam->userId
    << ", taskId: " << cbEvent->getTaskId()        // 当前任务的task id，方便定位问题，建议输出
    << ", data size: " << data.size()              // 数据的大小
    << std::endl;
#endif

  if (g_save_audio && data.size() > 0) {
    // 以追加形式将二进制音频数据写入文件
    std::string dir = "./tts_audio";
    if (access(dir.c_str(), 0) == -1) {
      mkdir(dir.c_str(), S_IRWXU);
    }
    char file_name[256] = {0};
    snprintf(file_name, 256, "%s/%s.%s", dir.c_str(), cbEvent->getTaskId(),
             g_format.c_str());
    FILE* tts_stream = fopen(file_name, "a+");
    if (tts_stream) {
      fwrite((char*)&data[0], data.size(), 1, tts_stream);
      fclose(tts_stream);
    }
  }

  if (cbParam && tmpParam->tParam) {
    tmpParam->tParam->audioTotalDuration +=
        getAudioFileTimeMs(data.size(), sample_rate, 1);

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
}

/**
 * @brief 返回 tts 文本对应的日志信息，增量返回对应的字幕信息
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
*/
void OnMetaInfo(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
#if 0
  std::cout << "OnMetaInfo "
    << "Response: " << cbEvent->getAllResponse()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
    << std::endl;
#endif
}

/**
 * @brief 服务端返回的所有信息会通过此回调反馈,
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
*/
void onMessage(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "onMessage: All response:"
      << cbEvent->getAllResponse() << std::endl;
}

void onExternalLogCallback(const char* timestamp, int level, const char* message) {
  std::cout << "onExternalLogCallback timestamp:"
      << timestamp << std::endl;
  std::cout << "  log level:" << level << std::endl;
  std::cout << "  log message:" << message << std::endl;
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
      get_profile_info("syDemo", &cur_sys_info);
      std::cout << cur << ": cur_usr_name: " << cur_sys_info.usr_name
        << " CPU: " << cur_sys_info.ave_cpu_percent << "%"
        << " MEM: " << cur_sys_info.ave_mem_percent << "%"
        << std::endl;

      PROFILE_INFO *cur_info = &(g_sys_info[cur]);
#if 0
      if (cur_info->ave_cpu_percent == 0) {
        strcpy(cur_info->usr_name, cur_sys_info.usr_name);
        cur_info->ave_cpu_percent = cur_sys_info.ave_cpu_percent;
        cur_info->ave_mem_percent = cur_sys_info.ave_mem_percent;
        cur_info->eAveTime = 0;
      } else {
        cur_info->ave_cpu_percent =
          (cur_info->ave_cpu_percent + cur_sys_info.ave_cpu_percent) / 2;
        cur_info->ave_mem_percent =
          (cur_info->ave_mem_percent + cur_sys_info.ave_mem_percent) / 2;
      }
      std::cout << cur << ": usr_name: " << cur_info->usr_name
        << " CPU: " << cur_info->ave_cpu_percent << "%"
        << " MEM: " << cur_info->ave_mem_percent << "%"
        << std::endl;
#else
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
#endif
    }

    if (global_sys) {
      PROFILE_INFO cur_sys_info;
      get_profile_info("syDemo", &cur_sys_info);

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
 *        以 createSynthesizerRequest          <----|
 *                   |                              |
 *           request->start()                       |
 *                   |                              |
 *           收到OnSynthesisChannelClosed回调       |
 *                   |                              |
 *           releaseSynthesizerRequest(request) ----|
 *        进行循环。
 */
void* pthreadFunc(void* arg) {
  int testCount = 0; // 运行次数计数，用于超过设置的loop次数后退出
  bool timedwait_flag = false;
  int chars_cnt = 0;

  // 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct* tst = (ParamStruct*)arg;
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  pthread_mutex_init(&(tst->mtx), NULL);

  // 初始化自定义回调参数
  ParamCallBack cbParam(tst);
  cbParam.userId = pthread_self();
  strcpy(cbParam.userInfo, "User.");

  while (global_run) {
    /*
     * 1. 创建语音识别SpeechSynthesizerRequest对象.
     *
     * 默认为实时短文本语音合成请求, 支持一次性合成300字符以内的文字,
     * 其中1个汉字、1个英文字母或1个标点均算作1个字符,
     * 超过300个字符的内容将会报错(或者截断).
     * 一次性合成超过300字符可考虑长文本语音合成功能.
     *
     * 实时短文本语音合成文档详见: https://help.aliyun.com/document_detail/84435.html
     * 长文本语音合成文档详见: https://help.aliyun.com/document_detail/130509.html
     */
    chars_cnt = AlibabaNls::NlsClient::getInstance()->calculateUtf8Chars(tst->text);
    std::cout << "pid:" << pthread_self()
      << " this text contains " << chars_cnt << "chars"
      << std::endl;

    AlibabaNls::SpeechSynthesizerRequest* request = NULL;
    if (chars_cnt > 300) {
      // 长文本语音合成
      request = AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest(AlibabaNls::LongTts);
    } else {
      // 短文本语音合成
      request = AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest();
    }
    if (request == NULL) {
      std::cout << "createSynthesizerRequest failed." << std::endl;
      break;
    }

    /*
     * 2. 设置用于接收结果的回调
     */
    // 设置音频合成结束回调函数
    request->setOnSynthesisCompleted(OnSynthesisCompleted, &cbParam);
    // 设置音频合成通道关闭回调函数
    request->setOnChannelClosed(OnSynthesisChannelClosed, &cbParam);
    // 设置异常失败回调函数
    request->setOnTaskFailed(OnSynthesisTaskFailed, &cbParam);
    // 设置文本音频数据接收回调函数
    request->setOnBinaryDataReceived(OnBinaryDataRecved, &cbParam);
    // 设置字幕信息
    request->setOnMetaInfo(OnMetaInfo, &cbParam);
    // 设置所有服务端返回信息回调函数
    //request->setOnMessage(onMessage, &cbParam);
    // 开启所有服务端返回信息回调函数, 其他回调(除了OnBinaryDataRecved)失效
    //request->setEnableOnMessage(true);

    /*
     * 3. 设置request的相关参数
     */
    // 设置待合成文本, 必填参数. 文本内容必须为UTF-8编码
    // 一次性合成超过300字符可考虑长文本语音合成功能.
    // 长文本语音合成文档详见: https://help.aliyun.com/document_detail/130509.html
    request->setText(tst->text);
    // 发音人, 包含"xiaoyun", "ruoxi", "xiaogang"等. 可选参数, 默认是xiaoyun
    request->setVoice("siqi");
    // 访问个性化音色，访问的Voice必须是个人定制音色
    //request->setPayloadParam("{\"enable_ptts\":true}");
    // 音量, 范围是0~100, 可选参数, 默认50
    request->setVolume(50);
    // 音频编码格式, 可选参数, 默认是wav. 支持的格式pcm, wav, mp3
    request->setFormat("wav");
    // 音频采样率, 包含8000, 16000. 可选参数, 默认是16000
    request->setSampleRate(sample_rate);
    // 语速, 范围是-500~500, 可选参数, 默认是0
    request->setSpeechRate(0);
    // 语调, 范围是-500~500, 可选参数, 默认是0
    request->setPitchRate(0);
    // 开启字幕
    request->setEnableSubtitle(true);

    // 设置AppKey, 必填参数, 请参照官网申请
    if (strlen(tst->appkey) > 0) {
      request->setAppKey(tst->appkey);
    }
    // 设置账号校验token, 必填参数
    if (strlen(tst->token) > 0) {
      request->setToken(tst->token);
    }
    if (strlen(tst->url) > 0) {
      request->setUrl(tst->url);
    }
    // 设置链接超时500ms
    request->setTimeout(500);
    // 获取返回文本的编码格式
    const char* output_format = request->getOutputFormat();
    std::cout << "text format: " << output_format << std::endl;

    /*
     * 4. start()为异步操作。成功则开始返回BinaryRecv事件。失败返回TaskFailed事件。
     */
    std::string ts = timestamp_str();
    std::cout << "start -> pid " << pthread_self() << " " << ts.c_str() << std::endl;
    gettimeofday(&(cbParam.startTv), NULL);
    int ret = request->start();
    ts = timestamp_str();
    run_cnt++;
    testCount++;
    if (ret < 0) {
      std::cout << "start failed. pid:" << pthread_self() << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request); // start()失败，释放request对象
      break;
    } else {
      std::cout << "start success. pid " << pthread_self() << " " << ts.c_str() << std::endl;
      cbParam.tParam->startedConsumed++;
      cbParam.tParam->firstFlag = false;
      struct ParamStatistics params;
      params.running = true;
      params.success_flag = false;
      params.audio_ms = 0;
      vectorSetParams(pthread_self(), true, params);
    }

    /*
     * 5. start成功，开始等待接收完所有合成数据。
     *    stop()为无意义接口，调用与否都会跑完全程.
     *    cancel()立即停止工作, 且不会有回调返回, 失败返回TaskFailed事件。
     */
//    ret = request->cancel();
    ret = request->stop();  // always return 0

    /*
     * 开始等待接收完所有合成数据。
     */
    struct timeval now;
    struct timespec outtime;
    if (ret == 0) {
      gettimeofday(&now, NULL);
      std::cout << "wait closed callback. pid " << pthread_self()
        << " tv: " << now.tv_sec << std::endl;
      /*
       * 根据文本长短、接收速度和网络环境，接收完所有合成数据的时间无法确定，
       * 这里设定30s超时只是展示一种超时机制。
       */
      outtime.tv_sec = now.tv_sec + 30;
      outtime.tv_nsec = now.tv_usec * 1000;
      // 等待closed事件后再进行释放, 否则会出现崩溃
      pthread_mutex_lock(&(cbParam.mtxWord));
      if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam.cvWord), &(cbParam.mtxWord), &outtime)) {
        std::cout << "synthesis timeout." << std::endl;
        timedwait_flag = true;
      }
      pthread_mutex_unlock(&(cbParam.mtxWord));
    } else {
      std::cout << "ret is " << ret << ", pid " << pthread_self() << std::endl;
    }
    gettimeofday(&now, NULL);
    std::cout << "current request task_id:" << request->getTaskId() << std::endl;
    std::cout << "stop finished. pid " << pthread_self()
        << " tv: " << now.tv_sec << std::endl;

    /*
     * 6. 完成所有工作后释放当前请求。
     *    请在closed事件(确定完成所有工作)后再释放, 否则容易破坏内部状态机, 会强制卸载正在运行的请求。
     */
    AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request);
    std::cout << "release Synthesizer success. pid "
        << pthread_self() << std::endl;

    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    }
  }  // while global_run

  pthread_mutex_destroy(&(tst->mtx));

  return NULL;
}

/**
 * @brief 长链接模式下工作线程
 *                  createSynthesizerRequest
 *                          |
 *        然后以    request->start()                <-----|
 *                          |                             |
 *                  收到OnSynthesisChannelClosed回调 -----|
 *        进行循环          |
 *                  releaseSynthesizerRequest(request)
 */
void* pthreadLongConnectionFunc(void* arg) {
  int testCount = 0; // 运行次数计数，用于超过设置的loop次数后退出
  int chars_cnt = 0;
  struct ParamStatistics params;
  bool timedwait_flag = false;

  // 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct* tst = (ParamStruct*)arg;
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  pthread_mutex_init(&(tst->mtx), NULL);

  // 初始化自定义回调参数
  ParamCallBack cbParam(tst);
  cbParam.userId = pthread_self();
  strcpy(cbParam.userInfo, "User.");

  /*
   * 1. 创建语音识别SpeechSynthesizerRequest对象.
   *
   * 默认为实时短文本语音合成请求, 支持一次性合成300字符以内的文字,
   * 其中1个汉字、1个英文字母或1个标点均算作1个字符,
   * 超过300个字符的内容将会报错(或者截断).
   * 一次性合成超过300字符可考虑长文本语音合成功能.
   *
   * 实时短文本语音合成文档详见: https://help.aliyun.com/document_detail/84435.html
   * 长文本语音合成文档详见: https://help.aliyun.com/document_detail/130509.html
   */
  chars_cnt = AlibabaNls::NlsClient::getInstance()->calculateUtf8Chars(tst->text);
  std::cout << "pid:" << pthread_self()
    << " this text contains " << chars_cnt << "chars"
    << std::endl;

  AlibabaNls::SpeechSynthesizerRequest* request = NULL;
  if (chars_cnt > 300) {
    // 长文本语音合成
    request = AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest(
        AlibabaNls::LongTts, "cpp", longConnection);
  } else {
    // 短文本语音合成
    request = AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest(
        AlibabaNls::ShortTts, "cpp", longConnection);
  }
  if (request == NULL) {
    std::cout << "createSynthesizerRequest failed." << std::endl;
    return NULL;
  }

  /*
   * 2. 设置用于接收结果的回调
   */
  // 设置音频合成结束回调函数
  request->setOnSynthesisCompleted(OnSynthesisCompleted, &cbParam);
  // 设置音频合成通道关闭回调函数
  request->setOnChannelClosed(OnSynthesisChannelClosed, &cbParam);
  // 设置异常失败回调函数
  request->setOnTaskFailed(OnSynthesisTaskFailed, &cbParam);
  // 设置文本音频数据接收回调函数
  request->setOnBinaryDataReceived(OnBinaryDataRecved, &cbParam);
  // 设置字幕信息
  request->setOnMetaInfo(OnMetaInfo, &cbParam);
  // 设置所有服务端返回信息回调函数
  //request->setOnMessage(onMessage, &cbParam);
  // 开启所有服务端返回信息回调函数, 其他回调(除了OnBinaryDataRecved)失效
  //request->setEnableOnMessage(true);

  /*
   * 3. 设置request的相关参数
   */
  // 设置待合成文本, 必填参数. 文本内容必须为UTF-8编码
  // 一次性合成超过300字符可考虑长文本语音合成功能.
  // 长文本语音合成文档详见: https://help.aliyun.com/document_detail/130509.html
  request->setText(tst->text);
  // 发音人, 包含"xiaoyun", "ruoxi", "xiaogang"等. 可选参数, 默认是xiaoyun
  request->setVoice("siqi");
  // 访问个性化音色，访问的Voice必须是个人定制音色
  //request->setPayloadParam("{\"enable_ptts\":true}");
  // 音量, 范围是0~100, 可选参数, 默认50
  request->setVolume(50);
  // 音频编码格式, 可选参数, 默认是wav. 支持的格式pcm, wav, mp3
  request->setFormat("wav");
  // 音频采样率, 包含8000, 16000. 可选参数, 默认是16000
  request->setSampleRate(sample_rate);
  // 语速, 范围是-500~500, 可选参数, 默认是0
  request->setSpeechRate(0);
  // 语调, 范围是-500~500, 可选参数, 默认是0
  request->setPitchRate(0);
  // 开启字幕
  request->setEnableSubtitle(true);
    if (strlen(tst->appkey) > 0) {
    request->setAppKey(tst->appkey);
  }

  // 设置AppKey, 必填参数, 请参照官网申请
  if (strlen(tst->appkey) > 0) {
    request->setAppKey(tst->appkey);
  }
  // 设置账号校验token, 必填参数
  if (strlen(tst->token) > 0) {
    request->setToken(tst->token);
  }
  if (strlen(tst->url) > 0) {
    request->setUrl(tst->url);
  }
  // 获取返回文本的编码格式
  const char* output_format = request->getOutputFormat();
  std::cout << "text format: " << output_format << std::endl;

  /*
   * 4. 当前request循环多轮语音合成
   */
  while (global_run) {
    /*
     * 4.1. start()为异步操作。成功则开始返回BinaryRecv事件。失败返回TaskFailed事件。
     */
    gettimeofday(&(cbParam.startTv), NULL);
    std::string ts = timestamp_str();
    std::cout << "start -> pid " << pthread_self()
        << " " << ts.c_str() << std::endl;
    int ret = request->start();
    ts = timestamp_str();
    run_cnt++;
    testCount++;
    if (ret < 0) {
      std::cout << "start failed. pid:" << pthread_self() << std::endl;
      break;
    } else {
      std::cout << "start success. pid " << pthread_self()
          << " " << ts.c_str() << std::endl;
      cbParam.tParam->startedConsumed++;
      cbParam.tParam->firstFlag = false;
      params.running = true;
      params.success_flag = false;
      params.audio_ms = 0;
      vectorSetParams(pthread_self(), true, params);
    }

    /*
     * 4.2 start成功，开始等待接收完所有合成数据。
     *     stop()为无意义接口，调用与否都会跑完全程.
     *     cancel()立即停止工作, 且不会有回调返回, 失败返回TaskFailed事件。
     */
    //ret = request->cancel();
    ret = request->stop();  // always return 0

    /*
     * 开始等待接收完所有合成数据。
     */
    struct timeval now;
    struct timespec outtime;
    if (ret == 0) {
      gettimeofday(&now, NULL);
      std::cout << "wait closed callback. pid " << pthread_self()
          << " tv: " << now.tv_sec << std::endl;
      /*
       * 根据文本长短、接收速度和网络环境，接收完所有合成数据的时间无法确定，
       * 这里设定30s超时只是展示一种超时机制。
       */
      outtime.tv_sec = now.tv_sec + 30;
      outtime.tv_nsec = now.tv_usec * 1000;
      // 等待closed事件后再进行释放, 否则会出现崩溃
      pthread_mutex_lock(&(cbParam.mtxWord));
      if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam.cvWord), &(cbParam.mtxWord), &outtime)) {
        std::cout << "synthesis timeout." << std::endl;
        timedwait_flag = true;
      }
      pthread_mutex_unlock(&(cbParam.mtxWord));
    } else {
      std::cout << "stop ret is " << ret << ", pid "
          << pthread_self() << std::endl;
    }
    gettimeofday(&now, NULL);
    std::cout << "stop finished. pid " << pthread_self()
        << " tv: " << now.tv_sec << std::endl;

    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    }
  } // while global_run

  /*
   * 5. 完成所有工作后释放当前请求。
   *    请在closed事件(确定完成所有工作)后再释放, 否则容易破坏内部状态机, 会强制卸载正在运行的请求。
   */
  AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request);
  request = NULL;

  pthread_mutex_destroy(&(tst->mtx));

  return NULL;
}

/**
 * 合成多个文本数据;
 * sdk多线程指一个文本数据对应一个线程, 非一个文本数据对应多个线程.
 * 示例代码为同时开启4个线程合成4个文件;
 * 免费用户并发连接不能超过10个;
 */
#define AUDIO_TEXT_NUMS 4
#define AUDIO_FILE_NAME_LENGTH 32
int speechSynthesizerMultFile(const char* appkey, int threads) {
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

  const char syAudioFiles[AUDIO_TEXT_NUMS][AUDIO_FILE_NAME_LENGTH] =
  {
    "syAudio0.wav", "syAudio1.wav", "syAudio2.wav", "syAudio3.wav"
  };
  /* 不要超过AUDIO_TEXT_LENGTH */
  const char texts[AUDIO_TEXT_NUMS][AUDIO_TEXT_LENGTH] =
  {
    "今日天气真不错，我想去操场踢足球.",
    "今日天气真不错，我想去操场踢足球.",
    "今日天气真不错，我想去操场踢足球.",
    "今日天气真不错，我想去操场踢足球."
  };
	ParamStruct pa[threads];

	for (int i = 0; i < threads; i ++) {
    int num = i % AUDIO_TEXT_NUMS;

    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());

    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));

    memset(pa[i].text, 0, AUDIO_TEXT_LENGTH);
    memcpy(pa[i].text, texts[num], strlen(texts[num]));

    memset(pa[i].audioFile, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].audioFile, syAudioFiles[num], strlen(syAudioFiles[num]));

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

    pa[i].audioTotalDuration = 0;
    pa[i].audioAveDuration = 0;

    pa[i].s50Value = 0;
    pa[i].s100Value = 0;
    pa[i].s200Value = 0;
    pa[i].s500Value = 0;
    pa[i].s1000Value = 0;
    pa[i].s2000Value = 0;
	}

  global_run = true;
  std::vector<pthread_t> pthreadId(threads);
  // 启动四个工作线程, 同时识别四个音频文件
  for (int j = 0; j < threads; j++) {
    if (longConnection) {
      pthread_create(&pthreadId[j], NULL, &pthreadLongConnectionFunc, (void *)&(pa[j]));
    } else {
      pthread_create(&pthreadId[j], NULL, &pthreadFunc, (void *)&(pa[j]));
    }
  }

  std::cout << "start pthread_join..." << std::endl;

  for (int j = 0; j < threads; j++) {
    pthread_join(pthreadId[j], NULL);
  }

  unsigned long long sTotalCount = 0; /*started总次数*/
  unsigned long long iTotalCount = 0; /*首包总次数*/
  unsigned long long eTotalCount = 0; /*completed总次数*/
  unsigned long long fTotalCount = 0; /*failed总次数*/
  unsigned long long cTotalCount = 0; /*closed总次数*/
  unsigned long long rTotalCount = 0; /*总请求数*/

  unsigned long long fMaxTime = 0; /*首包最大耗时*/
  unsigned long long fMinTime = 0; /*首包最小耗时*/
  unsigned long long fAveTime = 0; /*首包平均耗时*/

  unsigned long long eMaxTime = 0;
  unsigned long long eMinTime = 0;
  unsigned long long eAveTime = 0;

  unsigned long long cMaxTime = 0;
  unsigned long long cMinTime = 0;
  unsigned long long cAveTime = 0;

  unsigned long long audioAveTimeLen = 0;

  for (int i = 0; i < threads; i ++) {
    sTotalCount += pa[i].startedConsumed;
    iTotalCount += pa[i].firstConsumed;
    eTotalCount += pa[i].completedConsumed;
    fTotalCount += pa[i].failedConsumed;
    cTotalCount += pa[i].closeConsumed;
    rTotalCount += pa[i].requestConsumed;

    audioAveTimeLen += pa[i].audioAveDuration;

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

  eAveTime /= threads;
  cAveTime /= threads;
  fAveTime /= threads;
  audioAveTimeLen /= threads;

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

  for (int i = 0; i < threads; i ++) {
    std::cout << "-----" << std::endl;
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
      << " Audio Data duration: " << pa[i].audioAveDuration << " ms"
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

  std::cout << "Max first package time: " << fMaxTime << " ms" << std::endl;
  std::cout << "Min first package time: " << fMinTime << " ms" << std::endl;
  std::cout << "Ave first package time: " << fAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Max completed time: " << eMaxTime << " ms" << std::endl;
  std::cout << "Final Min completed time: " << eMinTime << " ms" << std::endl;
  std::cout << "Final Ave completed time: " << eAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Max closed time: " << cMaxTime << " ms" << std::endl;
  std::cout << "Final Min closed time: " << cMinTime << " ms" << std::endl;
  std::cout << "Final Ave closed time: " << cAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Ave audio data duration: " << audioAveTimeLen
            << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  usleep(2 * 1000 * 1000);

  std::cout << "speechSynthesizerMultFile exit..." << std::endl;
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
    } else if (!strcmp(argv[index], "--sampleRate")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      sample_rate = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--format")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_format = argv[index];
    } else if (!strcmp(argv[index], "--save")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        g_save_audio = true;
      } else {
        g_save_audio = false;
      }
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
        << "  --url <Url>\n"
        << "      public(default): "
           "wss://nls-gateway.cn-shanghai.aliyuncs.com/ws/v1\n"
        << "      internal: "
           "ws://nls-gateway.cn-shanghai-internal.aliyuncs.com/ws/v1\n"
        << "      mcos: wss://mcos-cn-shanghai.aliyuncs.com/ws/v1\n"
        << "  --threads <Thread Numbers, default 1>\n"
        << "  --time <Timeout secs, default 60 seconds>\n"
        << "  --format <audio format pcm opu or opus, default wav>\n"
        << "  --save <save audio flag, default 0>\n"
        << "  --log <logLevel, default LogDebug = 4, closeLog = 0>\n"
        << "  --sampleRate <sample rate, 16K or 8K>\n"
        << "  --long <long connection: 1, short connection: 0, default 0>\n"
        << "  --sys <use system getaddrinfo(): 1, evdns_getaddrinfo(): 0>\n"
        << "eg:\n"
        << "  ./syDemo --appkey xxxxxx --token xxxxxx\n"
        << "  ./syDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
           "--threads 4 --time 3600\n"
        << "  ./syDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
           "--threads 4 --time 3600 --format wav --save 1\n"
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

    // 根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-Synthesizer.txt,
    // LogDebug表示输出所有级别日志
    // 需要最早调用
    int ret = 0;
  #ifdef LOG_TRIGGER
    ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
        "log-synthesizer", AlibabaNls::LogDebug, 400, 50, NULL);
    if (ret < 0) {
      std::cout << "set log failed." << std::endl;
      return -1;
    }
  #endif

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

    // 合成多个文本
    ret = speechSynthesizerMultFile(g_appkey.c_str(), g_threads);
    if (ret) {
      std::cout << "speechSynthesizerMultFile failed." << std::endl;
      AlibabaNls::NlsClient::releaseInstance();
      break;
    }

    // 所有工作完成，进程退出前，释放nlsClient.
    // 请注意, releaseInstance()非线程安全.
    std::cout << "releaseInstance -> " << std::endl;
    AlibabaNls::NlsClient::releaseInstance();
    std::cout << "releaseInstance done." << std::endl;

    int size = g_statistics.size();
    int run_count = 0;
    int success_count = 0;
    if (size > 0) {
      std::map<unsigned long, struct ParamStatistics *>::iterator it;
      std::cout << "\n" << std::endl;
  #if 0
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
  #endif

      std::cout << "threads run count:" << g_threads
        << " success count:" << success_count << std::endl;
      std::cout << "requests run count:" << run_cnt
        << " success count:" << run_success
        << " fail count:" << run_fail << std::endl;

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
    run_success = 0;
    run_fail = 0;

    std::cout << "===============================" << std::endl;

  } // for

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
