/*
 * Copyright 2025 Alibaba Group Holding Limited
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
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "dashParaformerTranscriberRequest.h"
#include "dashToken.h"
#include "demo_utils.h"
#include "nlsClient.h"
#include "nlsEvent.h"
#include "profile_scan.h"

#define SELF_TESTING_TRIGGER
#define FRAME_16K_20MS 640
#define SAMPLE_RATE_16K 16000

#define OPERATION_TIMEOUT_S 6
#define LOOP_TIMEOUT 60
#define DEFAULT_STRING_LEN 512

// 自定义线程参数
struct ParamStruct {
  char fileName[DEFAULT_STRING_LEN];
  char apikey[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  uint64_t startedConsumed;   /*started事件完成次数*/
  uint64_t firstConsumed;     /*首包完成次数*/
  uint64_t completedConsumed; /*completed事件次数*/
  uint64_t closeConsumed;     /*closed事件次数*/

  uint64_t failedConsumed;  /*failed事件次数*/
  uint64_t requestConsumed; /*发起请求次数*/

  uint64_t sendConsumed; /*sendAudio调用次数*/

  uint64_t startTotalValue; /*所有started完成时间总和*/
  uint64_t startAveValue;   /*started完成平均时间*/
  uint64_t startMaxValue;   /*调用start()到收到started事件最大用时*/
  uint64_t startMinValue;   /*调用start()到收到started事件最小用时*/

  uint64_t firstTotalValue; /*所有收到首包用时总和*/
  uint64_t firstAveValue;   /*收到首包平均时间*/
  uint64_t firstMaxValue;   /*调用start()到收到首包最大用时*/
  uint64_t firstMinValue;   /*调用start()到收到首包最小用时*/
  bool firstFlag;           /*是否收到首包的标记*/

  uint64_t endTotalValue; /*start()到completed事件的总用时*/
  uint64_t endAveValue;   /*start()到completed事件的平均用时*/
  uint64_t endMaxValue;   /*start()到completed事件的最大用时*/
  uint64_t endMinValue;   /*start()到completed事件的最小用时*/

  uint64_t closeTotalValue; /*start()到closed事件的总用时*/
  uint64_t closeAveValue;   /*start()到closed事件的平均用时*/
  uint64_t closeMaxValue;   /*start()到closed事件的最大用时*/
  uint64_t closeMinValue;   /*start()到closed事件的最小用时*/

  uint64_t startApiTimeDiffUs; /* 调用start()的总耗时 */
  uint64_t stopApiTimeDiffUs;  /* 调用stop()的总耗时 */
  uint64_t stopApiConsumed;    /* 调用stop()次数 */
  uint64_t sendTotalValue;     /*单线程调用sendAudio总耗时*/

  uint64_t audioFileTimeLen;       /*灌入音频文件的音频时长*/
  uint64_t audioFileDataBytes;     /*灌入音频文件的音频字节数*/
  uint64_t audioFileDataUsedBytes; /*灌入音频文件的已经消耗的字节数*/

  uint64_t s50Value;  /*start()到started用时50ms以内*/
  uint64_t s100Value; /*start()到started用时100ms以内*/
  uint64_t s200Value;
  uint64_t s500Value;
  uint64_t s1000Value;
  uint64_t s1500Value;
  uint64_t s2000Value;
  uint64_t sToValue;

  uint32_t connectWithSSL;
  uint32_t connectWithDirectIP;
  uint32_t connectWithIpCache;
  uint32_t connectWithLongConnect;
  uint32_t connectWithPrestartedPool;
  uint32_t connectWithPreconnectedPool;

  pthread_mutex_t mtx;
};

struct SentenceParamStruct {
  uint32_t sentenceId;
  std::string text;
  uint64_t beginTime;
  uint64_t endTime;
  struct timeval endTv;
};

// 自定义事件回调参数
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

  unsigned long userId;
  char userInfo[8];

  pthread_mutex_t mtxWord;
  pthread_cond_t cvWord;

  struct timeval startTv;
  struct timeval startedTv;
  struct timeval startAudioTv;
  struct timeval firstTv;
  struct timeval completedTv;
  struct timeval closedTv;
  struct timeval failedTv;

  ParamStruct* tParam;

  std::vector<struct SentenceParamStruct> sentenceParam;
};

std::string g_apikey = "";
std::string g_token = "";
std::string g_url = "";
std::string g_model = "paraformer-realtime-v2";
std::string g_audio_path = "";
std::string g_audio_basename = "";
std::string g_transcription_path = "";
std::string g_transcription_tmp_path = "";
std::string g_audio_dir = "";
std::vector<std::string> g_wav_files;
std::string g_log_file = "log-paraformer-transcriber";
int g_log_count = 20;
int g_threads = 1;
int g_lived_threads = 0;
int g_cpu = 1;
int g_sync_timeout = 0;
bool g_save_audio = false;
bool g_on_message_flag = false;
bool g_continued_flag = false;
bool g_loop_file_flag = false;
static int loop_timeout = LOOP_TIMEOUT; /*循环运行的时间, 单位s*/
static int loop_count = 0; /*循环测试某音频文件的次数, 设置后loop_timeout无效*/
int g_setrlimit = 0; /*设置文件描述符的限制*/
int g_start_gradually_ms = 0;
int g_break_time_each_round_s = 0;

long g_expireTime = -1;
volatile static bool global_run = false;
static int sample_rate = SAMPLE_RATE_16K;
static int frame_size = FRAME_16K_20MS; /*每次推送音频字节数.*/
static int encoder_type = ENCODER_OPUS;
static std::string audio_format = "pcm";
static int logLevel = AlibabaNls::LogDebug; /* 0:为关闭log */
static int max_sentence_silence = 0; /*最大静音断句时间, 单位ms. 默认不设置.*/
static int run_cnt = 0;              /* 调用start()总次数 */
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
static PROFILE_INFO* g_sys_info = NULL;
static bool longConnection = false;
static bool sysAddrinfo = false;
static bool noSleepFlag = false;
static int sleepMultiple = 0;
static bool enableIntermediateResult = false;
static bool preconnectedPool = false;
static bool enableTranscription = false;
uint64_t g_tokenExpirationS = 0;
uint32_t g_requestTimeoutMs = 7500;
static bool simplifyLog = false;

void signal_handler_int(int signo) {
  std::cout << "\nget interrupt mesg\n" << std::endl;
  global_run = false;
}
void signal_handler_quit(int signo) {
  std::cout << "\nget quit mesg\n" << std::endl;
  global_run = false;
}

/**
 * 根据API Key重新生成一个临时 API Key，并获取其有效期时间戳
 */
int generateToken(std::string apiKey, std::string* token, long* expireTime) {
  AlibabaNlsCommon::DashToken dashTokenRequest;
  dashTokenRequest.setAPIKey(apiKey);
  int retCode = dashTokenRequest.applyDashToken();
  // dashTokenRequest.setExpireInSeconds(1800); /* [1 - 1800], 60 is default. */
  /*获取失败原因*/
  if (retCode < 0) {
    std::cout << "Failed error code: " << retCode
              << "  error msg: " << dashTokenRequest.getErrorMsg() << std::endl;
    return retCode;
  }

  *token = dashTokenRequest.getToken();
  *expireTime = dashTokenRequest.getExpireTime();

  return 0;
}

void tryToGetToken() {
  /**
   * 获取当前系统时间戳，判断token是否过期
   */
  std::time_t curTime = std::time(0);
  if (g_expireTime - curTime < 10) {
    std::cout << "the token will be expired, please generate new token by "
                 "API Key."
              << std::endl;
    int ret = generateToken(g_apikey, &g_token, &g_expireTime);
    if (ret < 0) {
      std::cout << "generate token failed" << std::endl;
    } else {
      if (g_token.empty() || g_expireTime < 0) {
        std::cout << "generate empty token" << std::endl;
      } else {
        std::cout << "get token: " << g_token << std::endl;
        std::cout << "get expire time: " << g_expireTime << std::endl;
      }
    }
  }
}

/**
 * @brief 调用start(), 成功与云端建立连接, sdk内部线程上报started事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onTranscriptionStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "onTranscriptionStarted:"
            << "  status code: " << cbEvent->getStatusCode()
            << "  task id: " << cbEvent->getTaskId()
            << "  all response:" << cbEvent->getAllResponse() << std::endl;

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;
    if (!simplifyLog) {
      std::cout << "  onTranscriptionStarted Max Time: "
                << tmpParam->tParam->startMaxValue
                << "  userId: " << tmpParam->userId << std::endl;
    }

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

    if (timeValue > 2000) {
      tmpParam->tParam->sToValue++;
    } else if (timeValue <= 2000 && timeValue > 1500) {
      tmpParam->tParam->s2000Value++;
    } else if (timeValue <= 1500 && timeValue > 1000) {
      tmpParam->tParam->s1500Value++;
    } else if (timeValue <= 1000 && timeValue > 500) {
      tmpParam->tParam->s1000Value++;
    } else if (timeValue <= 500 && timeValue > 200) {
      tmpParam->tParam->s500Value++;
    } else if (timeValue <= 200 && timeValue > 100) {
      tmpParam->tParam->s200Value++;
    } else if (timeValue <= 100 && timeValue > 50) {
      tmpParam->tParam->s100Value++;
    } else if (timeValue <= 50) {
      tmpParam->tParam->s50Value++;
    }

    // first package flag init
    tmpParam->tParam->firstFlag = false;

    // 通知发送线程start()成功, 可以继续发送数据
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
  }
}

/**
 * @brief 服务端检测到了一句话的开始, sdk内部线程上报SentenceBegin事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onSentenceBegin(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
#if 1
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (!simplifyLog) {
    std::cout << "onSentenceBegin CbParam: " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例
    std::cout
        << "  onSentenceBegin: "
        << "status code: "
        << cbEvent
               ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
        << ", task id: "
        << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
        << ", index: " << cbEvent->getSentenceIndex()  //句子编号，从1开始递增
        << ", time: "
        << cbEvent->getSentenceTime()  //当前已处理的音频时长，单位是毫秒
        << std::endl;

    std::cout << "  onSentenceBegin: All response:" << cbEvent->getAllResponse()
              << std::endl;  // 获取服务端返回的全部信息
  }
#endif
}

/**
 * @brief 服务端检测到了一句话结束, sdk内部线程上报SentenceEnd事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onSentenceEnd(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
#if 1
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (!simplifyLog) {
    std::cout << "onSentenceEnd CbParam: " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例

    std::cout
        << "  onSentenceEnd: "
        << "status code: "
        << cbEvent
               ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
        << ", task id: "
        << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
        << ", result: " << cbEvent->getResult()  // 当前句子的完成识别结果
        << ", index: " << cbEvent->getSentenceIndex()  // 当前句子的索引编号
        << ", begin_time: "
        << cbEvent->getSentenceBeginTime()  // 对应的SentenceBegin事件的时间
        << ", time: " << cbEvent->getSentenceTime()  // 当前句子的音频时长
        << ", confidence: "
        << cbEvent
               ->getSentenceConfidence()  // 结果置信度,取值范围[0.0,1.0]，值越大表示置信度越高
        << ", stashResult begin_time: "
        << cbEvent->getStashResultBeginTime()  //下一句话开始时间
        << ", stashResult current_time: "
        << cbEvent->getStashResultCurrentTime()  //下一句话当前时间
        << ", stashResult Sentence_id: "
        << cbEvent->getStashResultSentenceId()  // sentence Id
        << std::endl;

    /* 这里的start_time表示调用start后开始sendAudio传递Abytes音频时
     * 发现这句话的起点. 即调用start后传递start_time出现这句话的起点.
     * 这里的end_time表示调用start后开始sendAudio传递Bbytes音频时
     * 发现这句话的结尾. 即调用start后传递end_time出现这句话的结尾.
     */
    std::cout << "  onSentenceEnd: All response:" << cbEvent->getAllResponse()
              << std::endl;  // 获取服务端返回的全部信息
  }

  if (!g_transcription_tmp_path.empty()) {
    FILE* transcription_stream = NULL;
    transcription_stream = fopen(g_transcription_tmp_path.c_str(), "a+");
    if (transcription_stream) {
      if (cbEvent->getResult() != NULL && strlen(cbEvent->getResult()) > 0) {
        fwrite(cbEvent->getResult(), strlen(cbEvent->getResult()), 1,
               transcription_stream);
      }
      fclose(transcription_stream);
    }
  }

  if (tmpParam->tParam->audioFileDataBytes > 0) {
    float used_percents = tmpParam->tParam->audioFileDataUsedBytes * 100 /
                          tmpParam->tParam->audioFileDataBytes;
    std::string ts0 = timestampStr(NULL, NULL);
    std::cout << ts0 << " Send " << used_percents << "\% data." << std::endl;
  }

  struct SentenceParamStruct param;
  param.sentenceId = cbEvent->getSentenceIndex();
  param.text.assign(cbEvent->getResult());
  param.beginTime = cbEvent->getSentenceBeginTime();
  param.endTime = cbEvent->getSentenceTime();
  gettimeofday(&(param.endTv), NULL);
  tmpParam->sentenceParam.push_back(param);
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
    if (!simplifyLog) {
      std::cout << "onTranscriptionResultChanged userId: " << tmpParam->userId
                << ", " << tmpParam->userInfo
                << std::endl;  // 仅表示自定义参数示例
    }

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
    }  // firstFlag
  }

  if (!simplifyLog) {
    std::cout
        << "  onTranscriptionResultChanged: "
        << "status code: "
        << cbEvent
               ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
        << ", task id: "
        << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
        << ", result: " << cbEvent->getResult()  // 当前句子的中间识别结果
        << ", index: " << cbEvent->getSentenceIndex()  // 当前句子的索引编号
        << ", time: " << cbEvent->getSentenceTime()  // 当前句子的音频时长
        << std::endl;
    // std::cout << "onTranscriptionResultChanged: All response:"
    //     << cbEvent->getAllResponse() << std::endl; //
    //     获取服务端返回的全部信息
  }
}

/**
 * @brief 服务端停止实时音频流识别时, sdk内部线程上报Completed事件
 * @note 上报Completed事件之后，SDK内部会关闭识别连接通道.
         此时调用sendAudio会返回负值, 请停止发送.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
*/
void onTranscriptionCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_success++;

  std::cout
      << "onTranscriptionCompleted: "
      << " task id: " << cbEvent->getTaskId() << ", status code: "
      << cbEvent
             ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << ", All response:" << cbEvent->getAllResponse()
      << std::endl;  // 获取服务端返回的全部信息

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;

    if (!simplifyLog) {
      std::cout << "  onTranscriptionCompleted Max Time: "
                << tmpParam->tParam->endMaxValue
                << " userId: " << tmpParam->userId << std::endl;
    }

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
  }

  if (!g_transcription_tmp_path.empty()) {
    std::ifstream tmp_in(g_transcription_tmp_path.c_str());
    if (tmp_in.is_open()) {
      std::string content((std::istreambuf_iterator<char>(tmp_in)),
                          std::istreambuf_iterator<char>());
      tmp_in.close();

      if (!g_transcription_path.empty()) {
        FILE* transcription_stream = fopen(g_transcription_path.c_str(), "a+");
        if (transcription_stream) {
          if (content.length() > 0) {
            fwrite(g_audio_basename.c_str(), strlen(g_audio_basename.c_str()),
                   1, transcription_stream);
            fwrite(" ", 1, 1, transcription_stream);
            fwrite(content.c_str(), strlen(content.c_str()), 1,
                   transcription_stream);
            fwrite("\n", 1, 1, transcription_stream);
          }
          fclose(transcription_stream);
        }
      }
    }
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

  FILE* failed_stream = fopen("transcriptionTaskFailed.log", "a+");
  if (failed_stream) {
    std::string ts = timestampStr(NULL, NULL);
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
      << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
      << ", error message: " << cbEvent->getErrorMessage() << std::endl;
  std::cout << "onTaskFailed: All response:" << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;

    tmpParam->tParam->failedConsumed++;

    std::cout << "  onTaskFailed userId " << tmpParam->userId << ", "
              << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例
  }
}

/**
 * @brief 服务端返回的所有信息会通过此回调反馈
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onMessage(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "onMessage: All response:" << cbEvent->getAllResponse()
            << std::endl;
  std::cout << "onMessage: msg tyep:" << cbEvent->getMsgType() << std::endl;

  // 这里需要解析json
  int result = cbEvent->parseJsonMsg(true);
  if (result) {
    std::cout << "onMessage: parseJsonMsg failed:" << result << std::endl;
  } else {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    FILE* failed_stream = NULL;
    switch (cbEvent->getMsgType()) {
      case AlibabaNls::NlsEvent::TaskFailed:
        run_fail++;

        failed_stream = fopen("transcriptionTaskFailed.log", "a+");
        if (failed_stream) {
          std::string ts = timestampStr(NULL, NULL);
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
            << cbEvent
                   ->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
            << ", error message: " << cbEvent->getErrorMessage() << std::endl;
        std::cout << "onTaskFailed: All response:" << cbEvent->getAllResponse()
                  << std::endl;  // 获取服务端返回的全部信息

        if (tmpParam) {
          if (!tmpParam->tParam) return;

          tmpParam->tParam->failedConsumed++;

          std::cout << "  onTaskFailed userId " << tmpParam->userId << ", "
                    << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例
        }
        break;
      case AlibabaNls::NlsEvent::TranscriptionStarted:
        // 通知发送线程start()成功, 可以继续发送数据
        pthread_mutex_lock(&(tmpParam->mtxWord));
        pthread_cond_signal(&(tmpParam->cvWord));
        pthread_mutex_unlock(&(tmpParam->mtxWord));
        break;
      case AlibabaNls::NlsEvent::Close:
        std::cout << "  OnChannelCloseed: userId " << tmpParam->userId << ", "
                  << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例

        //通知发送线程, 最终识别结果已经返回, 可以调用stop()
        pthread_mutex_lock(&(tmpParam->mtxWord));
        pthread_cond_signal(&(tmpParam->cvWord));
        pthread_mutex_unlock(&(tmpParam->mtxWord));
        break;
    }
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
  const char* allResponse = cbEvent->getAllResponse();
  std::cout << "OnChannelClosed: All response: " << allResponse
            << std::endl;  // getResponse() 可以通道关闭信息

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) {
      std::cout << "  OnChannelCloseed tParam is nullptr" << std::endl;
      return;
    }

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

    if (!simplifyLog) {
      std::cout << "  OnChannelCloseed: userId " << tmpParam->userId << ", "
                << tmpParam->userInfo << std::endl;  // 仅表示自定义参数示例

      int vec_len = tmpParam->sentenceParam.size();
      if (vec_len > 0) {
        std::cout << "  \n=================================" << std::endl;
        std::cout << "  |  max sentence silence: " << max_sentence_silence
                  << "ms" << std::endl;
        std::cout << "  |  frame size: " << frame_size << "bytes" << std::endl;
        std::cout << "  --------------------------------" << std::endl;
        unsigned long long timeValue0 =
            tmpParam->startTv.tv_sec * 1000 + tmpParam->startTv.tv_usec / 1000;
        std::cout << "  |  start tv: " << timeValue0 << "ms" << std::endl;

        unsigned long long timeValue1 = tmpParam->startedTv.tv_sec * 1000 +
                                        tmpParam->startedTv.tv_usec / 1000;
        std::cout << "  |  started tv: " << timeValue1 << "ms" << std::endl;
        std::cout << "  |    started duration: " << timeValue1 - timeValue0
                  << "ms" << std::endl;

        unsigned long long timeValue2 = tmpParam->startAudioTv.tv_sec * 1000 +
                                        tmpParam->startAudioTv.tv_usec / 1000;
        std::cout << "  |  start audio tv: " << timeValue2 << "ms" << std::endl;
        std::cout << "  |    start audio duration: " << timeValue2 - timeValue0
                  << "ms" << std::endl;
        std::cout << "  --------------------------------" << std::endl;
        for (int i = 0; i < vec_len; i++) {
          struct SentenceParamStruct tmp = tmpParam->sentenceParam[i];
          std::cout << "  |  index: " << tmp.sentenceId << std::endl;
          std::cout << "  |  sentence duration: " << tmp.beginTime << " - "
                    << tmp.endTime << "ms = " << (tmp.endTime - tmp.beginTime)
                    << "ms" << std::endl;
          unsigned long long endTimeValue =
              tmp.endTv.tv_sec * 1000 + tmp.endTv.tv_usec / 1000;
          std::cout << "  |  end tv duration: " << timeValue2 << " - "
                    << endTimeValue << "ms = " << (endTimeValue - timeValue2)
                    << "ms" << std::endl;
          std::cout << "  |  text: " << tmp.text << std::endl;
          std::cout << "  --------------------------------" << std::endl;
        }
        std::cout << "  =================================\n" << std::endl;

        for (int j = 0; j < vec_len; j++) {
          tmpParam->sentenceParam.pop_back();
        }
        tmpParam->sentenceParam.clear();
      }
    }

    std::string connectType = findConnectType(allResponse);
    if (!connectType.empty()) {
      if (connectType == "connect_with_SSL") {
        tmpParam->tParam->connectWithSSL++;
      } else if (connectType == "connect_with_direct_IP") {
        tmpParam->tParam->connectWithDirectIP++;
      } else if (connectType == "connect_with_IP_from_cache") {
        tmpParam->tParam->connectWithIpCache++;
      } else if (connectType == "connect_with_long_connection") {
        tmpParam->tParam->connectWithLongConnect++;
      } else if (connectType == "connect_with_SSL_from_PrestartedPool") {
        tmpParam->tParam->connectWithPrestartedPool++;
      } else if (connectType == "connect_with_SSL_from_PreconnectedPool") {
        tmpParam->tParam->connectWithPreconnectedPool++;
      }
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

  struct timeval startTv;
  struct timeval currentTv;

  gettimeofday(&startTv, NULL);

  bool loop = true;
  while (loop && global_run) {
    // std::cout << "autoClose -->>" << std::endl;
    sleep(1);
    gettimeofday(&currentTv, NULL);
    uint64_t timeDiff = currentTv.tv_sec - startTv.tv_sec;
    if (timeDiff >= loop_timeout) {
      loop = false;
    }

    std::cout << " autoClose countdown: " << timeDiff << "/" << loop_timeout
              << "s."
              << " lived threads: " << g_lived_threads << "/" << g_threads
              << " run count: " << run_success << "/" << run_cnt << std::endl;

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
      get_profile_info("stDemo", &cur_sys_info);
      std::cout << cur << ": cur_usr_name: " << cur_sys_info.usr_name
                << " CPU: " << cur_sys_info.ave_cpu_percent << "%"
                << " MEM: " << cur_sys_info.ave_mem_percent << "%" << std::endl;

      PROFILE_INFO* cur_info = &(g_sys_info[cur]);
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
      get_profile_info("stDemo", &cur_sys_info);

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
  std::cout << "autoCloseFunc exit ... thread:" << pthread_self() << std::endl;

  return NULL;
}

/**
 * @brief 短链接模式下工作线程
 *   以 createDashParaformerTranscriberRequest  <----|
 *                   |                               |
 *           request->start()                        |
 *                   |                               |
 *           request->sendAudio()                    |
 *                   |                               |
 *           request->stop()                         |
 *                   |                               |
 *           收到onChannelClosed回调                  |
 *                   |                               |
 *   releaseDashParaformerTranscriberRequest(request)  ----|
 *        进行循环。
 */
void* pthreadFunction(void* arg) {
  int sleepMs = 0;  // 根据发送音频数据帧长度计算sleep时间，用于模拟真实录音情景
  int testCount = 0;  // 运行次数计数，用于超过设置的loop次数后退出
  ParamCallBack* cbParam = NULL;
  bool timedwait_flag = false;

  // 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct* tst = static_cast<ParamStruct*>(arg);
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  pthread_mutex_init(&(tst->mtx), NULL);

  // 打开音频文件, 获取数据
  std::string cur_file_name = getWavFile(g_wav_files);
  if (cur_file_name.empty()) {
    cur_file_name = tst->fileName;
  }
  std::ifstream fs(cur_file_name.c_str());
  if (!fs.is_open()) {
    std::cout << cur_file_name << " isn't exist.." << std::endl;
    return NULL;
  } else {
    fs.seekg(0, std::ios::end);
    int len = fs.tellg();
    tst->audioFileDataBytes = len;
    tst->audioFileTimeLen = getAudioFileTimeMs(len, sample_rate, 1);
  }

  // 退出线程前释放
  cbParam = new ParamCallBack(tst);
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  memset(cbParam->userInfo, 0, 8);
  strcpy(cbParam->userInfo, "User.");

  do {
    cbParam->tParam->requestConsumed++;

    /*
     * 1. 创建实时音频流识别DashParaformerTranscriberRequest对象
     */
    AlibabaNls::DashParaformerTranscriberRequest* request =
        AlibabaNls::NlsClient::getInstance()
            ->createDashParaformerTranscriberRequest("cpp", longConnection);
    if (request == NULL) {
      std::cout << "createDashParaformerTranscriberRequest failed."
                << std::endl;
      delete cbParam;
      cbParam = NULL;
      return NULL;
    }

    /*
     * 2. 设置用于接收结果的回调
     */
    // 设置识别启动回调函数
    request->setOnTranscriptionStarted(onTranscriptionStarted, cbParam);
    // 设置识别结果变化回调函数
    request->setOnTranscriptionResultChanged(onTranscriptionResultChanged,
                                             cbParam);
    // 设置语音转写结束回调函数
    request->setOnTranscriptionCompleted(onTranscriptionCompleted, cbParam);
    // 设置一句话开始回调函数
    request->setOnSentenceBegin(onSentenceBegin, cbParam);
    // 设置一句话结束回调函数
    request->setOnSentenceEnd(onSentenceEnd, cbParam);
    // 设置异常识别回调函数
    request->setOnTaskFailed(onTaskFailed, cbParam);
    // 设置识别通道关闭回调函数
    request->setOnChannelClosed(onChannelClosed, cbParam);
    if (g_on_message_flag) {
      // 设置所有服务端返回信息回调函数
      request->setOnMessage(onMessage, cbParam);
      // 开启所有服务端返回信息回调函数, 其他回调(除了OnBinaryDataRecved)失效
      request->setEnableOnMessage(true);
    }
    // 是否开启重连机制
    request->setEnableContinued(g_continued_flag);

    /*
     * 3. 设置request的相关参数
     */
    request->setModel(g_model.c_str());

    if (strlen(tst->url) > 0) {
      request->setUrl(tst->url);
    }
    // 设置链接超时500ms
    request->setTimeout(500);
    // 获取返回文本的编码格式
    // const char* output_format = request->getOutputFormat();
    // std::cout << "text format: " << output_format << std::endl;

    // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm
    if (encoder_type == ENCODER_OPUS) {
      request->setFormat("opus");
    } else if (encoder_type == ENCODER_OPU) {
      request->setFormat("opu");
    } else {
      if (strcmp(audio_format.c_str(), "wav") == 0) {
        request->setFormat("wav");
      } else {
        request->setFormat("pcm");
      }
    }
    // 设置音频数据采样率, 可选参数，目前支持16000, 8000. 默认是16000
    request->setSampleRate(sample_rate);

    // 语音断句检测阈值，一句话之后静音长度超过该值，即本句结束，合法参数范围200～2000(ms)，默认值800ms
    if (max_sentence_silence > 0) {
      if (max_sentence_silence > 2000 || max_sentence_silence < 200) {
        std::cout << "max sentence silence: " << max_sentence_silence
                  << " is invalid" << std::endl;
      } else {
        request->setMaxSentenceSilence(max_sentence_silence);
      }
    }

    tryToGetToken();
    if (!g_token.empty()) {
      request->setAPIKey(g_token.c_str());
    } else {
      if (strlen(tst->apikey) > 0) {
        request->setAPIKey(tst->apikey);
      }
    }

    struct timeval now;
    if (g_tokenExpirationS > 0) {
      gettimeofday(&now, NULL);
      uint64_t expirationMs =
          now.tv_sec * 1000 + now.tv_usec / 1000 + g_tokenExpirationS * 1000;
      request->setTokenExpirationTime(expirationMs);
    }

    // 设置链接超时时间
    // request->setTimeout(5000);
    // 设置发送超时时间
    // request->setSendTimeout(5000);
    // 设置是否开启接收超时
    // request->setEnableRecvTimeout(false);

    fs.clear();
    fs.seekg(0, std::ios::beg);

    tst->audioFileDataUsedBytes = 0;

    /*
     * 4.
     * start()为同步/异步两种操作，默认异步。由于异步模式通过回调判断request是否成功运行有修改门槛，且部分旧版本为同步接口。
     *    为了能较为平滑的更新升级SDK，提供了同步/异步两种调用方式。
     *    异步情况：默认未调用setSyncCallTimeout()的时候，start()调用立即返回，
     *            且返回值并不代表request成功开始工作，需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
     *    同步情况：调用setSyncCallTimeout()设置同步接口的超时时间，并启动同步模式。start()调用后不会立即返回，
     *            直到内部得到成功(同时也会触发started事件回调)或失败(同时也会触发TaskFailed事件回调)后返回。
     *            此方法方便旧版本SDK
     */
    if (!simplifyLog) {
      std::cout << "start ->" << std::endl;
    }
    struct timespec outtime;
    uint64_t startApiBeginUs = getTimestampUs(&(cbParam->startTv));
    int ret = request->start();
    cbParam->tParam->startApiTimeDiffUs +=
        (getTimestampUs(NULL) - startApiBeginUs);
    run_cnt++;
    testCount++;
    if (ret < 0) {
      std::cout << "start failed(" << ret << ")." << std::endl;
      run_start_failed++;
      // start()失败，释放request对象
      const char* request_info = request->dumpAllInfo();
      if (request_info) {
        std::cout << "  all info: " << request_info << std::endl;
      }
      AlibabaNls::NlsClient::getInstance()
          ->releaseDashParaformerTranscriberRequest(request);
      break;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 4.1.
         * g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *      需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         *
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        if (!simplifyLog) {
          std::cout << "wait started callback." << std::endl;
        }
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + OPERATION_TIMEOUT_S;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord),
                                                &(cbParam->mtxWord),
                                                &outtime)) {
          std::cout << "start timeout." << std::endl;
          std::cout << "current request task_id:" << request->getTaskId()
                    << std::endl;
          timedwait_flag = true;
          pthread_mutex_unlock(&(cbParam->mtxWord));
          request->cancel();
          run_cancel++;
          const char* request_info = request->dumpAllInfo();
          if (request_info) {
            std::cout << "  all info: " << request_info << std::endl;
          }
          AlibabaNls::NlsClient::getInstance()
              ->releaseDashParaformerTranscriberRequest(request);
          if (global_run) {
            continue;
          } else {
            break;
          }
        }
        pthread_mutex_unlock(&(cbParam->mtxWord));
        if (!simplifyLog) {
          std::cout << "current request task_id:" << request->getTaskId()
                    << std::endl;
        }
      } else {
        /*
         * 4.2.
         * g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用start()
         *      返回值0即表示启动成功。
         */
      }
    }

    uint64_t sendAudio_us = 0;
    uint32_t sendAudio_cnt = 0;

    /*
     * 5. 从文件取音频数据循环发送音频
     */
    gettimeofday(&(cbParam->startAudioTv), NULL);
    while (!fs.eof()) {
      uint8_t data[frame_size];
      memset(data, 0, frame_size);

      fs.read((char*)data, sizeof(uint8_t) * frame_size);
      size_t nlen = fs.gcount();
      if (nlen == 0) {
        std::cout << "fs empty..." << std::endl;
        if (g_loop_file_flag) {
          fs.clear();
          fs.seekg(0, std::ios::beg);
        }
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
        FILE* audio_stream = fopen(file_name, "a+");
        if (audio_stream) {
          fwrite(data, nlen, 1, audio_stream);
          fclose(audio_stream);
        }
      }

      struct timeval tv0, tv1;
      gettimeofday(&tv0, NULL);
      /*
       * 5.1. 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败,
       * 需要停止发送; 返回大于0 为成功. 若希望用省流量的opus格式上传音频数据,
       * 则第三参数传入ENCODER_OPU/ENCODER_OPUS
       *
       * ENCODER_OPU/ENCODER_OPUS模式时, 会占用一定的CPU进行音频压缩
       */
      ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      if (ret < 0) {
        // 发送失败, 退出循环数据发送
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
      } else {
        // std::cout << "send data " << nlen << "bytes, return " << ret << "
        // bytes." << std::endl;
        tst->audioFileDataUsedBytes += nlen;
      }

      /*
       * 运行过程中如果需要改参数, 可以调用control接口.
       * 以如下max_sentence_silence为例, 传入json字符串
       * 目前仅支持设置 max_sentence_silence和vocabulary_id
       */
      // request->control("{\"payload\":{\"max_sentence_silence\":2000}}");

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
         * 此处是用语音数据来自文件的方式进行模拟,
         * 故发送时需要控制速率来模拟真实录音场景.
         */
        // 根据发送数据大小，采样率，数据压缩比 来获取sleep时间
        sleepMs = getSendAudioSleepTime(ret, sample_rate, 1);

        /*
         * 语音数据发送延时控制, 实际使用中无需sleep.
         */
        if (sleepMs * 1000 > tmp_us) {
          if (sleepMultiple > 1) {
            usleep((sleepMs * 1000 - tmp_us) / sleepMultiple);
          } else {
            usleep(sleepMs * 1000 - tmp_us);
          }
        }
      }

      if (g_loop_file_flag && fs.eof()) {
        fs.clear();
        fs.seekg(0, std::ios::beg);
        tst->audioFileDataUsedBytes = 0;
      }
      if (!global_run) {
        break;
      }
    }  // while - sendAudio

    tst->sendConsumed += sendAudio_cnt;
    tst->sendTotalValue += sendAudio_us;
    if (sendAudio_cnt > 0) {
      if (!simplifyLog) {
        std::cout << "sendAudio ave: " << (sendAudio_us / sendAudio_cnt) << "us"
                  << std::endl;
      }
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
    if (!simplifyLog) {
      std::cout << "stop ->" << std::endl;
    }
    // stop()后会收到所有回调，若想立即停止则调用cancel()取消所有回调
    uint64_t stopApiBeginUs = getTimestampUs(NULL);
    std::string ts = timestampStr(NULL, NULL);
    ret = request->stop();
    cbParam->tParam->stopApiTimeDiffUs +=
        (getTimestampUs(NULL) - stopApiBeginUs);
    cbParam->tParam->stopApiConsumed++;
    if (!simplifyLog) {
      std::cout << "stop done. ret " << ret << "\n" << std::endl;
    }
    if (ret < 0) {
      std::cout << "stop failed(" << ret << ")." << std::endl;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 6.1.
         * g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *      需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         *
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        // 等待closed事件后再进行释放, 否则会出现崩溃
        // 若调用了setSyncCallTimeout()启动了同步调用模式,
        // 则可以不等待closed事件。
        if (!simplifyLog) {
          std::cout << "wait closed callback." << std::endl;
        }

        /*
         * 语音服务器存在来不及处理当前请求, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 错误信息为:
         * "Gateway:IDLE_TIMEOUT:Websocket session is idle for too long time,
         * the last directive is 'StopRecognition'!" 所以需要设置一个超时机制.
         */
        std::string ts0 = timestampStr(NULL, NULL);
        gettimeofday(&now, NULL);
        if (sleepMultiple > 1) {
          outtime.tv_sec =
              now.tv_sec + OPERATION_TIMEOUT_S * sleepMultiple * 10;
        } else {
          outtime.tv_sec = now.tv_sec + OPERATION_TIMEOUT_S;
        }
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord),
                                                &(cbParam->mtxWord),
                                                &outtime)) {
          std::string ts1 = timestampStr(NULL, NULL);
          std::cout << "stop timeout " << ts << "->" << ts0 << "->" << ts1
                    << std::endl;
          timedwait_flag = true;
          pthread_mutex_unlock(&(cbParam->mtxWord));
          const char* request_info = request->dumpAllInfo();
          if (request_info) {
            std::cout << "  all info: " << request_info << std::endl;
          }
          AlibabaNls::NlsClient::getInstance()
              ->releaseDashParaformerTranscriberRequest(request);
          if (global_run) {
            continue;
          } else {
            break;
          }
        }
        pthread_mutex_unlock(&(cbParam->mtxWord));
      } else {
        /*
         * 6.2.
         * g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用stop()
         *      返回值0即表示启动成功。
         */
      }
    }

    /*
     * 7. 完成所有工作后释放当前请求。
     *    请在closed事件(确定完成所有工作)后再释放, 否则容易破坏内部状态机,
     * 会强制卸载正在运行的请求。
     */
    const char* request_info = request->dumpAllInfo();
    if (request_info && strlen(request_info) > 0) {
      std::cout << "  final all info: " << request_info << std::endl;
    }
    AlibabaNls::NlsClient::getInstance()
        ->releaseDashParaformerTranscriberRequest(request);

    std::cout << "run " << testCount << "/" << loop_count << std::endl;
    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    } else {
      if (g_break_time_each_round_s > 0) {
        std::srand(now.tv_usec);
        int sleepS = rand() % g_break_time_each_round_s + 1;
        while (global_run && sleepS-- > 0) {
          sleep(1);
        }
      }
    }
  } while (global_run);

  std::cout << "finish this pthreadFunc " << pthread_self() << std::endl;

  // 关闭音频文件
  fs.close();

  pthread_mutex_destroy(&(tst->mtx));

  if (timedwait_flag) {
    /*
     * stop超时的情况下, 会在10s后返回TaskFailed和Closed回调.
     * 若在回调前delete cbParam, 会导致回调中对cbParam的操作变成野指针操作，
     * 故若存在cbParam, 则在这里等一会
     */
    sleep(10);
  }

  if (cbParam) {
    delete cbParam;
    cbParam = NULL;
  }

  std::cout << "this pthreadFunc " << pthread_self() << " exit." << std::endl;
  g_lived_threads--;

  return NULL;
}

/**
 * @brief 长链接模式下工作线程
 *           createDashParaformerTranscriberRequest
 *                          |
 *        然后以    request->start() <----------|
 *                          |                   |
 *                  request->sendAudio()        |
 *                          |                   |
 *                  request->stop()             |
 *                          |                   |
 *                  收到onChannelClosed回调  ---|
 *        进行循环          |
 *           releaseDashParaformerTranscriberRequest(request)
 */
void* pthreadLongConnectionFunction(void* arg) {
  int sleepMs = 0;  // 根据发送音频数据帧长度计算sleep时间，用于模拟真实录音情景
  int testCount = 0;  // 运行次数计数，用于超过设置的loop次数后退出
  ParamCallBack* cbParam = NULL;
  bool timedwait_flag = false;

  // 从自定义线程参数中获取token, 配置文件等参数.
  ParamStruct* tst = static_cast<ParamStruct*>(arg);
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
  memset(cbParam->userInfo, 0, 8);
  strcpy(cbParam->userInfo, "User.");

  pthread_mutex_init(&(tst->mtx), NULL);

  /*
   * 1. 创建实时音频流识别DashParaformerTranscriberRequest对象
   */
  AlibabaNls::DashParaformerTranscriberRequest* request =
      AlibabaNls::NlsClient::getInstance()
          ->createDashParaformerTranscriberRequest("cpp", longConnection);
  if (request == NULL) {
    std::cout << "DashParaformerTranscriberRequest failed." << std::endl;
    delete cbParam;
    cbParam = NULL;
    return NULL;
  }

  /*
   * 2. 设置用于接收结果的回调
   */
  // 设置识别启动回调函数
  request->setOnTranscriptionStarted(onTranscriptionStarted, cbParam);
  // 设置识别结果变化回调函数
  request->setOnTranscriptionResultChanged(onTranscriptionResultChanged,
                                           cbParam);
  // 设置语音转写结束回调函数
  request->setOnTranscriptionCompleted(onTranscriptionCompleted, cbParam);
  // 设置一句话开始回调函数
  request->setOnSentenceBegin(onSentenceBegin, cbParam);
  // 设置一句话结束回调函数
  request->setOnSentenceEnd(onSentenceEnd, cbParam);
  // 设置异常识别回调函数
  request->setOnTaskFailed(onTaskFailed, cbParam);
  // 设置识别通道关闭回调函数
  request->setOnChannelClosed(onChannelClosed, cbParam);
  if (g_on_message_flag) {
    // 设置所有服务端返回信息回调函数
    request->setOnMessage(onMessage, cbParam);
    // 开启所有服务端返回信息回调函数, 其他回调(除了OnBinaryDataRecved)失效
    request->setEnableOnMessage(true);
  }
  // 是否开启重连机制
  request->setEnableContinued(g_continued_flag);

  /*
   * 3. 设置request的相关参数
   */
  request->setModel(g_model.c_str());
  if (strlen(tst->url) > 0) {
    request->setUrl(tst->url);
  }
  // 获取返回文本的编码格式
  // const char* output_format = request->getOutputFormat();
  // std::cout << "text format: " << output_format << std::endl;

  // 参数设置, 如指定声学模型
  // request->setPayloadParam("{\"model\":\"test-regression-model\"}");

  // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm, 推荐opus
  if (encoder_type == ENCODER_OPUS) {
    request->setFormat("opus");
  } else if (encoder_type == ENCODER_OPU) {
    request->setFormat("opu");
  } else {
    if (strcmp(audio_format.c_str(), "wav") == 0) {
      request->setFormat("wav");
    } else {
      request->setFormat("pcm");
    }
  }
  // 设置音频数据采样率, 可选参数，目前支持16000, 8000. 默认是16000
  request->setSampleRate(sample_rate);

  // 语音断句检测阈值，一句话之后静音长度超过该值，即本句结束，
  // 合法参数范围200～2000(ms)，默认值800ms
  if (max_sentence_silence > 0) {
    if (max_sentence_silence > 2000 || max_sentence_silence < 200) {
      std::cout << "max sentence silence: " << max_sentence_silence
                << " is invalid" << std::endl;
    } else {
      request->setMaxSentenceSilence(max_sentence_silence);
    }
  }

  struct timeval now;
  if (g_tokenExpirationS > 0) {
    gettimeofday(&now, NULL);
    uint64_t expirationMs =
        now.tv_sec * 1000 + now.tv_usec / 1000 + g_tokenExpirationS * 1000;
    request->setTokenExpirationTime(expirationMs);
  }

  /*
   * 4. 循环读音频文件，将音频数据送给request，以模拟真实录音场景。
   */
  do {
    // 打开音频文件, 获取数据
    std::string cur_file_name = getWavFile(g_wav_files);
    if (cur_file_name.empty()) {
      cur_file_name = tst->fileName;
    }
    std::ifstream fs(cur_file_name.c_str());
    if (!fs.is_open()) {
      std::cout << cur_file_name << " isn't exist.." << std::endl;
      break;
    } else {
      fs.seekg(0, std::ios::end);
      int len = fs.tellg();
      tst->audioFileTimeLen = getAudioFileTimeMs(len, sample_rate, 1);
      fs.seekg(0, std::ios::beg);
    }

    cbParam->tParam->requestConsumed++;

    // update token
    tryToGetToken();
    if (!g_token.empty()) {
      request->setAPIKey(g_token.c_str());
    } else {
      if (strlen(tst->apikey) > 0) {
        request->setAPIKey(tst->apikey);
      }
    }

    /*
     * 4.1.
     * start()为同步/异步两种操作，默认异步。由于异步模式通过回调判断request是否成功运行有修改门槛，且部分旧版本为同步接口。
     *      为了能较为平滑的更新升级SDK，提供了同步/异步两种调用方式。
     *      异步情况：默认未调用setSyncCallTimeout()的时候，start()调用立即返回，
     *              且返回值并不代表request成功开始工作，需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
     *      同步情况：调用setSyncCallTimeout()设置同步接口的超时时间，并启动同步模式。start()调用后不会立即返回，
     *              直到内部得到成功(同时也会触发started事件回调)或失败(同时也会触发TaskFailed事件回调)后返回。
     *              此方法方便旧版本SDK
     */
    if (!simplifyLog) {
      std::cout << "start ->" << std::endl;
    }
    struct timespec outtime;
    struct timeval now;
    uint64_t startApiBeginUs = getTimestampUs(&(cbParam->startTv));
    int ret = request->start();
    cbParam->tParam->startApiTimeDiffUs +=
        (getTimestampUs(NULL) - startApiBeginUs);
    run_cnt++;
    testCount++;
    if (ret < 0) {
      run_start_failed++;
      std::cout << "start() failed: " << ret << std::endl;
      break;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 4.1.1.
         * g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *        需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         *
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        if (!simplifyLog) {
          std::cout << "wait started callback." << std::endl;
        }
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + OPERATION_TIMEOUT_S;
        outtime.tv_nsec = now.tv_usec * 1000;
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord),
                                                &(cbParam->mtxWord),
                                                &outtime)) {
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
         * 4.1.2.
         * g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用start()
         *        返回值0即表示启动成功。
         */
      }
    }

    uint64_t sendAudio_us = 0;
    uint32_t sendAudio_cnt = 0;

    /*
     * 4.2 从文件取音频数据循环发送音频
     */
    gettimeofday(&(cbParam->startAudioTv), NULL);
    while (!fs.eof()) {
      uint8_t data[frame_size];
      memset(data, 0, frame_size);

      fs.read((char*)data, sizeof(uint8_t) * frame_size);
      size_t nlen = fs.gcount();
      if (nlen == 0) {
        std::cout << "fs empty..." << std::endl;
        if (g_loop_file_flag) {
          fs.clear();
          fs.seekg(0, std::ios::beg);
        }
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
        FILE* audio_stream = fopen(file_name, "a+");
        if (audio_stream) {
          fwrite(data, nlen, 1, audio_stream);
          fclose(audio_stream);
        }
      }

      struct timeval tv0, tv1;
      gettimeofday(&tv0, NULL);
      /*
       * 4.2.1. 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败,
       * 需要停止发送; 返回大于0 为成功. 若希望用省流量的opus格式上传音频数据,
       * 则第三参数传入ENCODER_OPU/ENCODER_OPUS
       *
       * ENCODER_OPU/ENCODER_OPUS模式时, 会占用一定的CPU进行音频压缩
       */
      ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      if (ret < 0) {
        // 发送失败, 退出循环数据发送
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
      } else {
        // std::cout << "send data " << nlen << "bytes, return " << ret << "
        // bytes." << std::endl;
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
         * 此处是用语音数据来自文件的方式进行模拟,
         * 故发送时需要控制速率来模拟真实录音场景.
         */
        // 根据发送数据大小，采样率，数据压缩比 来获取sleep时间
        sleepMs = getSendAudioSleepTime(ret, sample_rate, 1);

        /*
         * 语音数据发送延时控制, 实际使用中无需sleep.
         */
        if (sleepMs * 1000 > tmp_us) {
          if (sleepMultiple > 1) {
            usleep((sleepMs * 1000 - tmp_us) / sleepMultiple);
          } else {
            usleep(sleepMs * 1000 - tmp_us);
          }
        }
      }

      if (g_loop_file_flag && fs.eof()) {
        fs.clear();
        fs.seekg(0, std::ios::beg);
      }
    }  // while - sendAudio

    // 关闭音频文件
    fs.clear();
    fs.close();

    tst->sendConsumed += sendAudio_cnt;
    tst->sendTotalValue += sendAudio_us;
    if (sendAudio_cnt > 0) {
      if (!simplifyLog) {
        std::cout << "sendAudio ave: " << (sendAudio_us / sendAudio_cnt) << "us"
                  << std::endl;
      }
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
    if (!simplifyLog) {
      std::cout << "stop ->" << std::endl;
    }
    // stop()后会收到所有回调，若想立即停止则调用cancel()取消所有回调
    uint64_t stopApiBeginUs = getTimestampUs(NULL);
    ret = request->stop();
    cbParam->tParam->stopApiTimeDiffUs +=
        (getTimestampUs(NULL) - stopApiBeginUs);
    cbParam->tParam->stopApiConsumed++;
    if (!simplifyLog) {
      std::cout << "stop done. ret " << ret << "\n" << std::endl;
    }
    if (ret < 0) {
      std::cout << "stop failed(" << ret << ")." << std::endl;
    } else {
      if (g_sync_timeout == 0) {
        /*
         * 4.3.1.
         * g_sync_timeout等于0，即默认未调用setSyncCallTimeout()，异步方式调用start()
         *        需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
         *
         * 等待started事件返回表示start()成功, 然后再发送音频数据。
         * 语音服务器存在来不及处理当前请求的情况, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 所以需要设置一个超时机制。
         */
        // 等待closed事件后再进行释放, 否则会出现崩溃
        // 若调用了setSyncCallTimeout()启动了同步调用模式,
        // 则可以不等待closed事件。
        if (!simplifyLog) {
          std::cout << "wait closed callback." << std::endl;
        }
        /*
         * 语音服务器存在来不及处理当前请求, 10s内不返回任何回调的问题,
         * 然后在10s后返回一个TaskFailed回调, 错误信息为:
         * "Gateway:IDLE_TIMEOUT:Websocket session is idle for too long time,
         * the last directive is 'StopRecognition'!" 所以需要设置一个超时机制.
         */
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + OPERATION_TIMEOUT_S;
        outtime.tv_nsec = now.tv_usec * 1000;
        // 等待closed事件后再进行释放, 否则会出现崩溃
        pthread_mutex_lock(&(cbParam->mtxWord));
        if (ETIMEDOUT == pthread_cond_timedwait(&(cbParam->cvWord),
                                                &(cbParam->mtxWord),
                                                &outtime)) {
          std::cout << "stop timeout" << std::endl;
          pthread_mutex_unlock(&(cbParam->mtxWord));
          break;
        }
        pthread_mutex_unlock(&(cbParam->mtxWord));
      } else {
        /*
         * 4.3.2.
         * g_sync_timeout大于0，即调用了setSyncCallTimeout()，同步方式调用stop()
         *        返回值0即表示启动成功。
         */
      }
    }

    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    }
  } while (global_run);

  /*
   * 5. 完成所有工作后释放当前请求。
   *    请在closed事件(确定完成所有工作)后再释放, 否则容易破坏内部状态机,
   * 会强制卸载正在运行的请求。
   */
  const char* request_info = request->dumpAllInfo();
  if (request_info && strlen(request_info) > 0) {
    std::cout << "  final all info: " << request_info << std::endl;
  }
  AlibabaNls::NlsClient::getInstance()->releaseDashParaformerTranscriberRequest(
      request);
  request = NULL;

  pthread_mutex_destroy(&(tst->mtx));

  if (timedwait_flag) {
    /*
     * stop超时的情况下, 会在10s后返回TaskFailed和Closed回调.
     * 若在回调前delete cbParam, 会导致回调中对cbParam的操作变成野指针操作，
     * 故若存在cbParam, 则在这里等一会
     */
    sleep(10);
  }

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
#define AUDIO_FILE_NUMS 4
#define AUDIO_FILE_NAME_LENGTH 32
int speechTranscriberMultFile(int threads) {
#ifdef SELF_TESTING_TRIGGER
  if (loop_count == 0) {
    pthread_t p_id;
    pthread_create(&p_id, NULL, &autoCloseFunc, NULL);
    pthread_detach(p_id);
  }
#endif

  char audioFileNames[AUDIO_FILE_NUMS][AUDIO_FILE_NAME_LENGTH] = {
      "test0.wav", "test1.wav", "test2.wav", "test3.wav"};
  ParamStruct pa[threads];

  // init ParamStruct
  for (int i = 0; i < threads; i++) {
    memset(pa[i].fileName, 0, DEFAULT_STRING_LEN);
    if (g_audio_path.empty()) {
      int num = i % AUDIO_FILE_NUMS;
      strncpy(pa[i].fileName, audioFileNames[num], strlen(audioFileNames[num]));
    } else {
      strncpy(pa[i].fileName, g_audio_path.c_str(), DEFAULT_STRING_LEN);
    }

    memset(pa[i].apikey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].apikey, g_apikey.c_str(), g_apikey.length());

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

    pa[i].startApiTimeDiffUs = 0;
    pa[i].stopApiTimeDiffUs = 0;
    pa[i].stopApiConsumed = 0;
    pa[i].sendTotalValue = 0;

    pa[i].audioFileTimeLen = 0;

    pa[i].s50Value = 0;
    pa[i].s100Value = 0;
    pa[i].s200Value = 0;
    pa[i].s500Value = 0;
    pa[i].s1000Value = 0;
    pa[i].s1500Value = 0;
    pa[i].s2000Value = 0;
    pa[i].sToValue = 0;

    pa[i].connectWithSSL = 0;
    pa[i].connectWithDirectIP = 0;
    pa[i].connectWithIpCache = 0;
    pa[i].connectWithLongConnect = 0;
    pa[i].connectWithPrestartedPool = 0;
    pa[i].connectWithPreconnectedPool = 0;
  }

  global_run = true;
  std::vector<pthread_t> pthreadId(threads);
  // 启动threads个工作线程, 同时识别threads个音频文件
  for (int j = 0; j < threads; j++) {
    if (longConnection) {
      pthread_create(&pthreadId[j], NULL, &pthreadLongConnectionFunction,
                     (void*)&(pa[j]));
      g_lived_threads++;
    } else {
      if (g_start_gradually_ms > 0) {
        struct timeval now;
        gettimeofday(&now, NULL);
        std::srand(now.tv_usec);
        int sleepMs = rand() % g_start_gradually_ms + 1;
        usleep(sleepMs * 1000);
      }
      pthread_create(&pthreadId[j], NULL, &pthreadFunction, (void*)&(pa[j]));
      g_lived_threads++;
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
  unsigned long long s1500Count = 0;
  unsigned long long s2000Count = 0;
  unsigned long long sToCount = 0;

  unsigned long long eMaxTime = 0;
  unsigned long long eMinTime = 0;
  unsigned long long eAveTime = 0;

  unsigned long long cMaxTime = 0;
  unsigned long long cMinTime = 0;
  unsigned long long cAveTime = 0;

  unsigned long long sendTotalCount = 0;
  unsigned long long sendTotalTime = 0;
  unsigned long long sendAveTime = 0;

  unsigned long long startApiTotalTime = 0;
  unsigned long long stopApiTotalTime = 0;
  unsigned long long stopApiTotalCount = 0;

  unsigned long long audioFileAveTimeLen = 0;

  uint32_t connectWithSSL = 0;
  uint32_t connectWithDirectIP = 0;
  uint32_t connectWithIpCache = 0;
  uint32_t connectWithLongConnect = 0;
  uint32_t connectWithPrestartedPool = 0;
  uint32_t connectWithPreconnectedPool = 0;

  for (int i = 0; i < threads; i++) {
    sTotalCount += pa[i].startedConsumed;
    iTotalCount += pa[i].firstConsumed;
    eTotalCount += pa[i].completedConsumed;
    fTotalCount += pa[i].failedConsumed;
    cTotalCount += pa[i].closeConsumed;
    rTotalCount += pa[i].requestConsumed;
    sendTotalCount += pa[i].sendConsumed;
    sendTotalTime += pa[i].sendTotalValue;  // us, 所有线程sendAudio耗时总和
    startApiTotalTime += pa[i].startApiTimeDiffUs;
    stopApiTotalTime += pa[i].stopApiTimeDiffUs;
    stopApiTotalCount += pa[i].stopApiConsumed;
    audioFileAveTimeLen += pa[i].audioFileTimeLen;

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
    s1500Count += pa[i].s1500Value;
    s2000Count += pa[i].s2000Value;
    sToCount += pa[i].sToValue;

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

    connectWithSSL += pa[i].connectWithSSL;
    connectWithDirectIP += pa[i].connectWithDirectIP;
    connectWithIpCache += pa[i].connectWithIpCache;
    connectWithLongConnect += pa[i].connectWithLongConnect;
    connectWithPrestartedPool += pa[i].connectWithPrestartedPool;
    connectWithPreconnectedPool += pa[i].connectWithPreconnectedPool;
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
    PROFILE_INFO* cur_info = &(g_sys_info[cur]);
    cur_info->eAveTime = eAveTime;
  }

  if (sendTotalCount > 0) {
    sendAveTime = sendTotalTime / sendTotalCount;
  }

  for (int i = 0; i < threads; i++) {
    std::cout << "-----" << std::endl;
    std::cout << "No." << i << " Max started time: " << pa[i].startMaxValue
              << " ms" << std::endl;
    std::cout << "No." << i << " Min started time: " << pa[i].startMinValue
              << " ms" << std::endl;
    std::cout << "No." << i << " Ave started time: " << pa[i].startAveValue
              << " ms" << std::endl;

    std::cout << "No." << i
              << " Max first package time: " << pa[i].firstMaxValue << " ms"
              << std::endl;
    std::cout << "No." << i
              << " Min first package time: " << pa[i].firstMinValue << " ms"
              << std::endl;
    std::cout << "No." << i
              << " Ave first package time: " << pa[i].firstAveValue << " ms"
              << std::endl;

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

    std::cout << "No." << i
              << " Audio File duration: " << pa[i].audioFileTimeLen << " ms"
              << std::endl;
  }

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Total: " << std::endl;
  std::cout << "      Request: " << rTotalCount << std::endl;
  std::cout << "      Started: " << sTotalCount << std::endl;
  std::cout << "      Completed: " << eTotalCount << std::endl;
  std::cout << "      Failed: " << fTotalCount << std::endl;
  std::cout << "      Closed: " << cTotalCount << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max started time: " << sMaxTime << " ms" << std::endl;
  std::cout << "Min started time: " << sMinTime << " ms" << std::endl;
  std::cout << "Ave started time: " << sAveTime << " ms" << std::endl;

  std::cout << "  Started time <=   50 ms: " << s50Count << std::endl;
  std::cout << "               <=  100 ms: " << s100Count << std::endl;
  std::cout << "               <=  200 ms: " << s200Count << std::endl;
  std::cout << "               <=  500 ms: " << s500Count << std::endl;
  std::cout << "               <= 1000 ms: " << s1000Count << std::endl;
  std::cout << "               <= 1500 ms: " << s1500Count << std::endl;
  std::cout << "               <= 2000 ms: " << s2000Count << std::endl;
  std::cout << "                > 2000 ms: " << sToCount << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max first package time: " << fMaxTime << " ms" << std::endl;
  std::cout << "Min first package time: " << fMinTime << " ms" << std::endl;
  std::cout << "Ave first package time: " << fAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Max completed time: " << eMaxTime << " ms" << std::endl;
  std::cout << "Final Min completed time: " << eMinTime << " ms" << std::endl;
  std::cout << "Final Ave completed time: " << eAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Ave sendAudio() time: " << sendAveTime << " us" << std::endl;
  if (run_cnt > 0) {
    std::cout << "Ave start() time: " << startApiTotalTime / run_cnt << " us"
              << std::endl;
  }
  if (stopApiTotalCount > 0) {
    std::cout << "Ave stop() time: " << stopApiTotalTime / stopApiTotalCount
              << " us" << std::endl;
  }

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Max closed time: " << cMaxTime << " ms" << std::endl;
  std::cout << "Min closed time: " << cMinTime << " ms" << std::endl;
  std::cout << "Ave closed time: " << cAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Ave audio file duration: " << audioFileAveTimeLen << " ms"
            << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Connect type:" << std::endl;
  std::cout << "  SSL:                        " << connectWithSSL << std::endl;
  std::cout << "  Direct IP:                  " << connectWithDirectIP
            << std::endl;
  std::cout << "  IP_from_cache:              " << connectWithIpCache
            << std::endl;
  std::cout << "  Long connection:            " << connectWithLongConnect
            << std::endl;
  std::cout << "  SSL_from_PrestartedtedPool: " << connectWithPrestartedPool
            << std::endl;
  std::cout << "  SSL_from_PreconnectedPool:  " << connectWithPreconnectedPool
            << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "speechTranscribeMultFile exit..." << std::endl;
  return 0;
}

int invalid_argv(int index, int argc) {
  if (index >= argc) {
    std::cout << "invalid params..." << std::endl;
    return 1;
  }
  return 0;
}

int parse_argv(int argc, char* argv[]) {
  int index = 1;
  while (index < argc) {
    if (!strcmp(argv[index], "--apikey")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_apikey = argv[index];
    } else if (!strcmp(argv[index], "--url")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_url = argv[index];
    } else if (!strcmp(argv[index], "--model")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_model = argv[index];
    } else if (!strcmp(argv[index], "--threads")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_threads = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--cpu")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_cpu = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--time")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      loop_timeout = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--loop")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      loop_count = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--type")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (strcmp(argv[index], "pcm") == 0) {
        encoder_type = ENCODER_NONE;
        audio_format = "pcm";
      } else if (strcmp(argv[index], "wav") == 0) {
        encoder_type = ENCODER_NONE;
        audio_format = "wav";
      } else if (strcmp(argv[index], "opu") == 0) {
        encoder_type = ENCODER_OPU;
        audio_format = "opu";
      } else if (strcmp(argv[index], "opus") == 0) {
        encoder_type = ENCODER_OPUS;
        audio_format = "opus";
      } else {
        return 1;
      }
    } else if (!strcmp(argv[index], "--log")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      logLevel = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--sampleRate")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      sample_rate = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--frameSize")) {
      index++;
      frame_size = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--save")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        g_save_audio = true;
      } else {
        g_save_audio = false;
      }
    } else if (!strcmp(argv[index], "--message")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        g_on_message_flag = true;
      } else {
        g_on_message_flag = false;
      }
    } else if (!strcmp(argv[index], "--continued")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        g_continued_flag = true;
      } else {
        g_continued_flag = false;
      }
    } else if (!strcmp(argv[index], "--NlsScan")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      profile_scan = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--long")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        longConnection = true;
      } else {
        longConnection = false;
      }
    } else if (!strcmp(argv[index], "--sys")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        sysAddrinfo = true;
      } else {
        sysAddrinfo = false;
      }
    } else if (!strcmp(argv[index], "--noSleep")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        noSleepFlag = true;
      } else {
        noSleepFlag = false;
      }
    } else if (!strcmp(argv[index], "--sleepMultiple")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      sleepMultiple = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--audioFile")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_audio_path = argv[index];
    } else if (!strcmp(argv[index], "--audioDir")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_audio_dir = argv[index];
    } else if (!strcmp(argv[index], "--dumpTranscription")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        enableTranscription = true;
      } else {
        enableTranscription = false;
      }
    } else if (!strcmp(argv[index], "--dumpTranscriptionFile")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_transcription_path = argv[index];
    } else if (!strcmp(argv[index], "--loopAudioFile")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        g_loop_file_flag = true;
      } else {
        g_loop_file_flag = false;
      }
    } else if (!strcmp(argv[index], "--maxSilence")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      max_sentence_silence = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--sync_timeout")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_sync_timeout = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--intermedia")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        enableIntermediateResult = true;
      } else {
        enableIntermediateResult = false;
      }
    } else if (!strcmp(argv[index], "--setrlimit")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_setrlimit = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--logFile")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_log_file = argv[index];
    } else if (!strcmp(argv[index], "--logFileCount")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_log_count = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--simplifyLog")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        simplifyLog = true;
      } else {
        simplifyLog = false;
      }
    } else if (!strcmp(argv[index], "--startGradually")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_start_gradually_ms = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--breakTimeEachRound")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_break_time_each_round_s = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--preconnectedPool")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        preconnectedPool = true;
      } else {
        preconnectedPool = false;
      }
    } else if (!strcmp(argv[index], "--tokenExpiration")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_tokenExpirationS = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--requestTimeout")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_requestTimeoutMs = atoi(argv[index]);
    }
    index++;
  }

  if (g_apikey.empty() && getenv("DASHSCOPE_APIKEY")) {
    g_apikey.assign(getenv("DASHSCOPE_APIKEY"));
  }
  if (g_apikey.empty()) {
    std::cout << "short of params..." << std::endl;
    std::cout << "if apikey is empty, please setenv DASHSCOPE_APIKEY"
              << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (parse_argv(argc, argv)) {
    std::cout
        << "params is not valid.\n"
        << "Usage:\n"
        << "  --apikey <apikey>\n"
        << "  --tokenExpiration <Manually set the token expiration time, and "
           "the current time plus the set value is the expiration time, in "
           "seconds.>\n"
        << "  --url <Url>\n"
        << "      public(default): "
           "wss://dashscope.aliyuncs.com/api-ws/v1/inference\n"
        << "  --threads <The number of requests running at the same time, "
           "default 1>\n"
        << "  --time <The time of the test run, in seconds>\n"
        << "  --loop <The number of requests run>\n"
        << "  --type <The audio format that is transmitted to the server, "
           "which can be opus and pcm, the default opus>\n"
        << "  --log <logLevel, default LogDebug = 4, closeLog = 0>\n"
        << "  --sampleRate <sample rate, 16000 or 8000, default is 16000.>\n"
        << "  --long <long connection: 1, short connection: 0, default 0>\n"
        << "  --sys <use system getaddrinfo(): 1, evdns_getaddrinfo(): 0>\n"
        << "  --noSleep <Use sleep after sendAudio(), default 0>\n"
        << "  --audioFile <The absolute path of the audio file used for the "
           "audio input for the test>\n"
        << "  --audioDir <>\n"
        << "  --logFile <log file>\n"
        << "  --logFileCount <The count of log file>\n"
        << "  --loopAudioFile <Loop reading data from audio files simulates "
           "long-lasting real-time speech recognition, default 0.>\n"
        << "  --frameSize <The number of bytes that are fed into the SDK each "
           "time, 640 ~ 16384bytes>\n"
        << "  --save <The audio data sent to the SDK is saved, default 0>\n"
        << "  --message <Use onMessage for callbacks, default 0>\n"
        << "  --continued <Enable the reconnection mechanism within the SDK, "
           "default 0>\n"
        << "  --intermedia <Turn on intermedia recognition results, default "
           "0>\n"
        << "  --maxSilence <Maximum silence time, in milliseconds, "
           "200~6000(ms), default is 800ms>\n"
        << "  --NlsScan <Profile scan number of CPUs>\n"
        << "  --sync_timeout <Use sync invoke, set timeout_ms, default 0, "
           "invoke is async.>\n"
        << "  --setrlimit <Set the limits of the file descriptor>\n"
        << "  --preconnectedPool <Whether to enable the pre-connected pooling "
           "feature. Default is 0.>\n"
        << "  --startGradually <Start multithreading step by step, set the "
           "value to the maximum random value of the start thread delay, in "
           "milliseconds. Default is 0.>\n"
        << "  --breakTimeEachRound <Set the interval time for each round, and "
           "set the value to the maximum interval time, in seconds. Default is "
           "0.>\n"
        << "eg:\n"
        << "  ./paraformerTranscriberDemo --apikey xxxxxx\n"
        << "  ./paraformerTranscriberDemo --apikey xxxxxx --threads 4 --time "
           "3600\n"
        << "  ./paraformerTranscriberDemo --apikey xxxxxx --threads 4 --time "
           "3600 "
           "--log 4 --type pcm\n"
        << "  ./paraformerTranscriberDemo --apikey xxxxxx --threads 1 --loop 1 "
           "--log 4 --type wav --audioFile /home/xxx/test0.wav \n"
        << std::endl;
    return -1;
  }

  signal(SIGINT, signal_handler_int);
  signal(SIGQUIT, signal_handler_quit);

  std::cout << " API Key: " << g_apikey << std::endl;
  std::cout << " model: " << g_model << std::endl;
  std::cout << " threads: " << g_threads << std::endl;
  if (!g_audio_path.empty()) {
    std::cout << " audio files path: " << g_audio_path << std::endl;
    size_t last_slash = g_audio_path.find_last_of("/\\");
    size_t last_dot = g_audio_path.find_last_of('.');
    g_audio_basename =
        g_audio_path.substr(last_slash + 1, last_dot - last_slash - 1);

    std::cout << " audio file basename: " << g_audio_basename << std::endl;
  }
  std::cout << " loop timeout: " << loop_timeout << std::endl;
  std::cout << " loop count: " << loop_count << std::endl;

  if (enableTranscription && !g_transcription_path.empty()) {
    size_t last_slash = g_transcription_path.find_last_of("/\\");
    size_t last_dot = g_transcription_path.find_last_of('.');
    std::string dir = (last_slash == std::string::npos)
                          ? "."
                          : g_transcription_path.substr(0, last_slash + 1);
    std::string filename = (last_slash == std::string::npos)
                               ? g_transcription_path
                               : g_transcription_path.substr(last_slash + 1);
    // Replace .text with _tmp.text
    if (filename.length() >= 5 &&
        filename.substr(filename.length() - 5) == ".text") {
      g_transcription_tmp_path =
          dir + filename.substr(0, filename.length() - 5) + "_tmp.text";
    } else {
      g_transcription_tmp_path = g_transcription_path + "_tmp.text";
    }

    std::remove(g_transcription_tmp_path.c_str());
    std::cout << " transcription path: " << g_transcription_path << std::endl;
    std::cout << " transcription tmp path: " << g_transcription_tmp_path
              << std::endl;
  }

  std::cout << "\n" << std::endl;

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

  if (!g_audio_dir.empty()) {
    findWavFiles(g_audio_dir, g_wav_files);
  }

  for (cur_profile_scan = -1; cur_profile_scan < profile_scan;
       cur_profile_scan++) {
    if (cur_profile_scan == 0) continue;

    // 根据需要设置SDK输出日志, 可选.
    // 此处表示SDK日志输出至log-funasr-Transcriber.txt，
    // LogDebug表示输出所有级别日志 需要最早调用
    if (logLevel > 0) {
      int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
          g_log_file.c_str(), (AlibabaNls::LogLevel)logLevel, 100, g_log_count);
      if (ret < 0) {
        std::cout << "set log failed." << std::endl;
        return -1;
      }
    }

    // 存在部分设备在设置了dns后仍然无法通过SDK的dns获取可用的IP,
    // 可调用此接口主动启用系统的getaddrinfo来解决这个问题.
    if (sysAddrinfo) {
      AlibabaNls::NlsClient::getInstance()->setUseSysGetAddrInfo(true);
    }

    // g_sync_timeout等于0，即默认未调用setSyncCallTimeout()
    // 异步方式调用
    //   start():
    //   需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
    //   stop(): 需要等待返回closed事件则表示完成此次交互。
    // 同步方式调用
    //   start()/stop() 调用返回即表示交互启动/结束。
    if (g_sync_timeout > 0) {
      AlibabaNls::NlsClient::getInstance()->setSyncCallTimeout(g_sync_timeout);
    }

    if (preconnectedPool) {
      AlibabaNls::NlsClient::getInstance()->setPreconnectedPool(
          g_threads, 15000, g_requestTimeoutMs);
    }

    if (g_setrlimit) {
      struct rlimit lim;
      // 设置软限制
      lim.rlim_cur = g_setrlimit;      // 设置软限制
      lim.rlim_max = g_setrlimit * 2;  // 设置硬限制
      if (setrlimit(RLIMIT_NOFILE, &lim) == -1) {
        perror("setrlimit");
        std::cout << "setrlimit failed ... " << std::endl;
        return 1;
      }
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
    int ret = speechTranscriberMultFile(g_threads);
    if (ret) {
      std::cout << "speechTranscriberMultFile failed." << std::endl;
      AlibabaNls::NlsClient::releaseInstance();
      break;
    }

    // 所有工作完成，进程退出前，释放nlsClient.
    // 请注意, releaseInstance()非线程安全.
    AlibabaNls::NlsClient::releaseInstance();

    std::cout << "\n" << std::endl;
    std::cout << "Threads count:" << g_threads << ", Requests count:" << run_cnt
              << std::endl;
    std::cout << " time:" << loop_timeout << " count:" << loop_count
              << std::endl;
    std::cout << "    success:" << run_success << " cancel:" << run_cancel
              << " fail:" << run_fail << " start failed:" << run_start_failed
              << std::endl;

    sleep(1);

    run_cnt = 0;
    run_start_failed = 0;
    run_success = 0;
    run_fail = 0;

    std::cout << "===============================" << std::endl;
  }  // for

  if (g_sys_info) {
    int k = 0;
    for (k = 0; k < profile_scan + 1; k++) {
      PROFILE_INFO* cur_info = &(g_sys_info[k]);
      if (k == 0) {
        std::cout << "WorkThread: " << k - 1 << " USER: " << cur_info->usr_name
                  << " CPU: " << cur_info->ave_cpu_percent << "% "
                  << " MEM: " << cur_info->ave_mem_percent << "% "
                  << " Average Time: " << cur_info->eAveTime << "ms"
                  << std::endl;
      } else {
        std::cout << "WorkThread: " << k << " USER: " << cur_info->usr_name
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
    std::cout << "      CPU: " << g_min_percent.ave_cpu_percent << " %"
              << std::endl;
    std::cout << "      MEM: " << g_min_percent.ave_mem_percent << " %"
              << std::endl;
    std::cout << "    Max: " << std::endl;
    std::cout << "      CPU: " << g_max_percent.ave_cpu_percent << " %"
              << std::endl;
    std::cout << "      MEM: " << g_max_percent.ave_mem_percent << " %"
              << std::endl;
    std::cout << "    Average: " << std::endl;
    std::cout << "      CPU: " << g_ave_percent.ave_cpu_percent << " %"
              << std::endl;
    std::cout << "      MEM: " << g_ave_percent.ave_mem_percent << " %"
              << std::endl;
    std::cout << "===============================" << std::endl;
  }

  return 0;
}
