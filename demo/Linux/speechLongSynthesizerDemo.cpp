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
#include <pthread.h>
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
#include "nlsClient.h"
#include "nlsEvent.h"
#include "nlsToken.h"
#include "speechSynthesizerRequest.h"
#include "profile_scan.h"

#define SAMPLE_RATE 16000
#define SELF_TESTING_TRIGGER
#define LOOP_TIMEOUT 60
#define LOG_TRIGGER
#define TTS_AUDIO_DUMP
#define DEFAULT_STRING_LEN 128
#define TEXT_STRING_LEN  8192

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey Secret重新生成一个token，并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
 */
// 自定义线程参数
struct ParamStruct {
  char text[TEXT_STRING_LEN];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  uint64_t startedConsumed;   /*started事件完成次数*/
  uint64_t completedConsumed; /*completed事件次数*/
  uint64_t closeConsumed;     /*closed事件次数*/

  uint64_t failedConsumed;    /*failed事件次数*/
  uint64_t requestConsumed;   /*发起请求次数*/

  uint64_t startTotalValue;   /*所有started完成时间总和*/
  uint64_t startAveValue;     /*started完成平均时间*/
  uint64_t startMaxValue;     /*调用start()到收到started事件最大用时*/
  uint64_t startMinValue;     /*调用start()到收到started事件最小用时*/

  uint64_t endTotalValue;     /*start()到completed事件的总用时*/
  uint64_t endAveValue;       /*start()到completed事件的平均用时*/
  uint64_t endMaxValue;       /*start()到completed事件的最大用时*/
  uint64_t endMinValue;       /*start()到completed事件的最小用时*/

  uint64_t closeTotalValue;   /*start()到closed事件的总用时*/
  uint64_t closeAveValue;     /*start()到closed事件的平均用时*/
  uint64_t closeMaxValue;     /*start()到closed事件的最大用时*/
  uint64_t closeMinValue;     /*start()到closed事件的最小用时*/

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
std::string g_url = "";
int g_threads = 1;
static int loop_timeout = LOOP_TIMEOUT;

long g_expireTime = -1;
volatile static bool global_run = false;
static std::map<unsigned long, struct ParamStatistics *> g_statistics;
static pthread_mutex_t params_mtx;
static int run_cnt = 0;
static int run_success = 0;
static int run_fail = 0;

static int profile_scan = -1;
static int cur_profile_scan = -1;
static PROFILE_INFO * g_sys_info = NULL;

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
  // std::cout << "OnSynthesisCompleted: All response:" << cbEvent->getAllResponse() << std::endl; // 获取服务端返回的全部信息

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
#if 1
  std::string ts = timestamp_str();
  std::cout << "OnBinaryDataRecved: " << ts.c_str() << ", "
    << "status code: " << cbEvent->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
    << ", userId: " << tmpParam->userId
    << ", taskId: " << cbEvent->getTaskId()        // 当前任务的task id，方便定位问题，建议输出
    << ", data size: " << data.size()              // 数据的大小
    << std::endl;
#endif
#ifdef TTS_AUDIO_DUMP
  // 以追加形式将二进制音频数据写入文件
  char file_name[256] = {0};
  snprintf(file_name, 256,
      "./tts_audio/%s.wav",
      cbEvent->getTaskId()
      );
  FILE *tts_stream = fopen(file_name, "a+");
  if (tts_stream) {
    fwrite((char*)&data[0], data.size(), 1, tts_stream);
    fclose(tts_stream);
  }
#endif
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
  }
  global_run = false;
  std::cout << "autoCloseFunc exit..." << pthread_self() << std::endl;
  return NULL;
}

// 工作线程
void* pthreadFunc(void* arg) {
  /*
   * 0: 从自定义线程参数中获取token, 配置文件等参数.
   */
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
     * 2: 创建语音识别SpeechSynthesizerRequest对象.
     * 默认为实时短文本语音合成请求, 支持一次性合成300字符以内的文字,
     * 其中1个汉字、1个英文字母或1个标点均算作1个字符,
     * 超过300个字符的内容将会报错(或者截断).
     * 一次性合成超过300字符可考虑长文本语音合成功能.
     *
     * 实时短文本语音合成文档详见: https://help.aliyun.com/document_detail/84435.html
     * 长文本语音合成文档详见: https://help.aliyun.com/document_detail/130509.html
     */
    AlibabaNls::SpeechSynthesizerRequest* request =
        AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest(AlibabaNls::LongTts);
    if (request == NULL) {
      std::cout << "createSynthesizerRequest failed." << std::endl;
      //cbParam.audioFile.close();
      break;
    }

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

    if (strlen(tst->appkey) > 0) {
      request->setAppKey(tst->appkey);
    }

    // 设置待合成文本, 必填参数. 文本内容必须为UTF-8编码
    // 默认为实时短文本语音合成请求, 支持一次性合成300字符以内的文字,
    // 其中1个汉字、1个英文字母或1个标点均算作1个字符,
    // 超过300个字符的内容将会报错(或者截断).
    // 注意: 传入的字符串或字符数组的长度, 错误使用字符串会导致崩溃.
    request->setText(tst->text);
    // 发音人, 包含"xiaoyun", "ruoxi", "xiaogang"等. 可选参数, 默认是xiaoyun
    request->setVoice("siqi");
    // 音量, 范围是0~100, 可选参数, 默认50
    request->setVolume(50);
    // 音频编码格式, 可选参数, 默认是wav. 支持的格式pcm, wav, mp3
    request->setFormat("wav");
    // 音频采样率, 包含8000, 16000. 可选参数, 默认是16000
    request->setSampleRate(SAMPLE_RATE);
    // 语速, 范围是-500~500, 可选参数, 默认是0
    request->setSpeechRate(0);
    // 语调, 范围是-500~500, 可选参数, 默认是0
    request->setPitchRate(0);
    //开启字幕
    request->setEnableSubtitle(true);
    // 设置账号校验token, 必填参数
    if (strlen(tst->token) > 0) {
      request->setToken(tst->token);
    }
    if (strlen(tst->url) > 0) {
      request->setUrl(tst->url);
    }

    /*
     * 2: start()为异步操作。成功则开始返回BinaryRecv事件。失败返回TaskFailed事件。
     */
    pthread_mutex_lock(&(cbParam.mtxWord));
    gettimeofday(&(cbParam.startTv), NULL);
    std::string ts = timestamp_str();
    std::cout << "start -> pid " << pthread_self() << " " << ts.c_str() << std::endl;
    int ret = request->start();
    ts = timestamp_str();
    run_cnt++;
    if (ret < 0) {
      pthread_mutex_unlock(&(cbParam.mtxWord));
      std::cout << "start failed. pid:" << pthread_self() << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request); // start()失败，释放request对象
      //cbParam.audioFile.close();
      break;
    } else {
      std::cout << "start success. pid " << pthread_self() << " " << ts.c_str() << std::endl;
      cbParam.tParam->startedConsumed++;
      struct ParamStatistics params;
      params.running = true;
      params.success_flag = false;
      params.audio_ms = 0;
      vectorSetParams(pthread_self(), true, params);
    }

    /*
     * 6: 通知云端数据发送结束.
     * stop()为无意义接口，调用与否都会跑完全程.
     * cancel()立即停止工作, 且不会有回调返回, 失败返回TaskFailed事件。
     */
//    ret = request->cancel();
    ret = request->stop();  // always return 0

    /*
     * 7: 识别结束, 释放request对象
     */
    struct timeval now;
    struct timespec outtime;
    if (ret == 0) {
      gettimeofday(&now, NULL);
      std::cout << "wait closed callback. pid " << pthread_self()
        << " tv: " << now.tv_sec << std::endl;
      gettimeofday(&now, NULL);
      outtime.tv_sec = now.tv_sec + 60;
      outtime.tv_nsec = now.tv_usec * 1000;
      // 等待closed事件后再进行释放, 否则会出现崩溃
      pthread_cond_timedwait(&(cbParam.cvWord), &(cbParam.mtxWord), &outtime);
      pthread_mutex_unlock(&(cbParam.mtxWord));
    } else {
      pthread_mutex_unlock(&(cbParam.mtxWord));
      std::cout << "ret is " << ret << ", pid " << pthread_self() << std::endl;
    }
    gettimeofday(&now, NULL);
    std::cout << "stop finished. pid " << pthread_self()
      << " tv: " << now.tv_sec << std::endl;
    AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request);
    std::cout << "release Synthesizer success. pid "
        << pthread_self() << std::endl;

//    if (vectorGetFailed(cbParam.userId)) break;
  }  // while global_run

  usleep(1 * 1000 * 1000);

  pthread_mutex_destroy(&(tst->mtx));

  return NULL;
}

/**
 * 合成多个文本数据;
 * sdk多线程指一个文本数据对应一个线程, 非一个文本数据对应多个线程.
 * 示例代码为同时开启4个线程合成4个文件;
 * 免费用户并发连接不能超过10个;
 */
#define AUDIO_TEXT_LENGTH 64
#define AUDIO_FILE_NAME_LENGTH 32
int speechSynthesizerMultFile(const char* appkey, int threads) {
  /**
   * 获取当前系统时间戳，判断token是否过期
   */
  std::time_t curTime = std::time(0);
  if (g_token.empty()) {
    if (g_expireTime - curTime < 10) {
      std::cout << "the token will be expired, please generate new token by AccessKey-ID and AccessKey-Secret." << std::endl;
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

  /*
   * 一次性合成最高10万字符（其中1个汉字、1个英文字母或1个标点均算作1个字符）
   */
  const char texts[TEXT_STRING_LEN] =
  {
    "西湖，位于浙江省杭州市西湖区龙井路1号，杭州市区西部，景区总面积49平方千米，汇水面积为21.22平方千米，湖面面积为6.38平方千米。西湖南、西、北三面环山，湖中白堤、苏堤、杨公堤、赵公堤将湖面分割成若干水面。西湖的湖体轮廓呈近椭圆形，湖底部较为平坦，湖泊平均水深为2.27米，最深约5米，最浅不到1米。湖泊天然地表水源是金沙涧、龙泓涧、赤山涧（慧因涧）、长桥溪四条溪流。西湖地处中国东南丘陵边缘和中亚热带北缘，年均太阳总辐射量在100—110千卡/平方厘米之间，日照时数1800—2100小时。西湖有100多处公园景点，有“西湖十景”、“新西湖十景”、“三评西湖十景”之说，有60多处国家、省、市级重点文物保护单位和20多座博物馆，有断桥、雷峰塔、钱王祠、净慈寺、苏小小墓等景点。2007年，杭州市西湖风景名胜区被评为“国家AAAAA级旅游景区”。2011年6月24日，“杭州西湖文化景观”正式被列入《世界遗产名录》。位置境域，西湖位于浙江省杭州市西湖区龙井路1号，杭州市区西部，景区总面积49平方千米，湖面面积为6.38平方千米，中心地理坐标：北纬30°14′45″， 东经120°8′30″。地质地貌，西湖南、西、北三面环山，东邻城区，南部和钱塘江隔山相邻，湖中白堤、苏堤、杨公堤、赵公堤将湖面分割成若干水面，湖中有三岛，西湖群山以西湖为中心，由近及远可分为四个层次，海拔高度从50至400米依次抬升，形成“重重叠叠山”的地貌景观。西湖与群山的第一层次相连，与后三个层次的距离，分别为1650米、3450米和5600米，在湖中看山景，仰角在5°以内。水文特征，西湖汇水面积为21.22平方千米，流域内年径流量为1400万立方米，蓄水量近1400万立方米，水的自然交替为1次/年。西湖的湖体轮廓呈近椭圆形，湖底部较为平坦。湖泊天然地表水源是金沙涧、龙泓涧、赤山涧（慧因涧）、长桥溪四条溪流。湖泊水位保持在黄海标高7.15米，±0.05米，最高水位7.70米，最低水位6.92米，高低相差50厘米。库容量约1429.4万立方米。湖泊平均水深为2.27米，最深约5米，最浅不到1米。湖泊年均湖面降水量562.9万立方米。水系冲刷系数为1.49，当枯水季节闸门封闭时，流速等于0，即使是洪水时期，一般流速也只在0.05米/秒以下。西湖引钱塘江水，量约为1.2亿立方米/年。气候特点，西湖地处中国东南丘陵边缘和亚热带北缘，年均太阳总辐射量在100—110千卡/平方厘米之间，日照时数1800—2100小时，光照充足，年均气温16.2℃，年均无霜期245天。常年四季分明，晴雨相间，冬、夏季风交替显著，雨量充沛，年降水量约1500毫米，空气湿润 。西湖生态系统可分为三个子系统：森林生态系统、淡水生态系统及湿地生态系统，以森林生态系统为主。西湖景区内共有鸟类119种，哺乳类动物20余种。西湖水面就有水鸟38种。丁家山、乌龟山等是鹭鸟的栖息地，鹭鸟数目在两万只以上。西湖在石灰岩地区，发育着一些落叶阔叶树种。西湖周边群山森林生态系统主要以常绿阔叶落叶阔叶混交林为主，间有亚热带常绿阔叶林、落叶阔叶林、针阔叶混交林等。西湖景区内种子植物物种共有184科739属1369种，其中裸子植物7科19属28种，被子植物150科675属1273种，蕨类27科45属68种。有国家一级保护珍稀植物浙江楠、野大豆、短穗竹等21种，国家二级保护植物63种。列为国家级或省级保护珍稀植物预备清单有浙江润楠、堇叶紫金牛、线萼南蛇藤、脉叶翅棱芹、寒竹等5种。西湖十景：西湖十景形成于南宋时期，基本围绕西湖分布，或位于湖上。西湖十景各擅其胜，组合在一起又能代表古代西湖胜景精华。新西湖十景：1984年《杭州日报》社、杭州市园林文物管理局、浙江电视台、杭州市旅游总公司、《园林与名胜》杂志5单位联合发起举办新西湖十景评选活动，得到杭州炼油厂、杭州啤酒厂、杭州中药二厂、杭州橡胶厂、杭州电视机厂、杭州牙膏厂、杭州电风扇总厂、杭州洗衣机总厂、杭州利民制药厂等9家企业赞助，并邀请夏衍、吴冷西、王朝闻、刘开渠、常书鸿为景名评选委员会顾问。最后评选出10处景点。三评西湖十景：2007年2月，杭州市秉承先人对西湖美景品评的做法，举行以“和谐西湖、品质杭州”为主题的“我最喜爱的西湖新景点”评选活动（“西湖十景”、“新西湖十景”不再列入评选范围），旨在进一步打响西湖品牌，弘扬“生活品质之城”的杭州城市品牌，提升西湖和杭州城市的知名度、美誉度和吸引力。该评选活动分群众推举景点景名、景点投票、景名投票3个阶段，历时9个月，33.68万人次参与，从149个景点中选出10个景点，并确定10个景点名、1个总名称。"
  };
	ParamStruct pa[threads];

	for (int i = 0; i < threads; i ++) {

    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());

    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));

    memset(pa[i].text, 0, TEXT_STRING_LEN);
    memcpy(pa[i].text, texts, strlen(texts));

    memset(pa[i].url, 0, DEFAULT_STRING_LEN);
    if (!g_url.empty()) {
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
  // 启动四个工作线程, 同时识别四个音频文件
  for (int j = 0; j < threads; j++) {
    pthread_create(&pthreadId[j], NULL, &pthreadFunc, (void *)&(pa[j]));
  }

  std::cout << "start pthread_join..." << std::endl;

  for (int j = 0; j < threads; j++) {
    pthread_join(pthreadId[j], NULL);
  }

  unsigned long long sTotalCount = 0;
  unsigned long long eTotalCount = 0;
  unsigned long long fTotalCount = 0;
  unsigned long long cTotalCount = 0;
  unsigned long long rTotalCount = 0;

  unsigned long long eMaxTime = 0;
  unsigned long long eMinTime = 0;
  unsigned long long eAveTime = 0;

  unsigned long long cMaxTime = 0;
  unsigned long long cMinTime = 0;
  unsigned long long cAveTime = 0;

  for (int i = 0; i < threads; i ++) {
    sTotalCount += pa[i].startedConsumed;
    eTotalCount += pa[i].completedConsumed;
    fTotalCount += pa[i].failedConsumed;
    cTotalCount += pa[i].closeConsumed;
    rTotalCount += pa[i].requestConsumed;

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
  }

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Total. " << std::endl;
  std::cout << "Final Request: " << rTotalCount << std::endl;
  std::cout << "Final Started: " << sTotalCount << std::endl;
  std::cout << "Final Completed: " << eTotalCount << std::endl;
  std::cout << "Final Failed: " << fTotalCount << std::endl;
  std::cout << "Final Closed: " << cTotalCount << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Max completed time: " << eMaxTime << " ms" << std::endl;
  std::cout << "Final Min completed time: " << eMinTime << " ms" << std::endl;
  std::cout << "Final Ave completed time: " << eAveTime << " ms" << std::endl;

  std::cout << "\n ------------------- \n" << std::endl;

  std::cout << "Final Max closed time: " << cMaxTime << " ms" << std::endl;
  std::cout << "Final Min closed time: " << cMinTime << " ms" << std::endl;
  std::cout << "Final Ave closed time: " << cAveTime << " ms" << std::endl;

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
    } else if (!strcmp(argv[index], "--NlsScan")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      profile_scan = atoi(argv[index]);
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
      << "eg:\n"
      << "  ./syDemo --appkey xxxxxx --token xxxxxx\n"
      << "  ./syDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx --threads 4 --time 3600\n"
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

  if (profile_scan > 0) {
    g_sys_info = new PROFILE_INFO[profile_scan + 1];
    memset(g_sys_info, 0, sizeof(PROFILE_INFO) * (profile_scan + 1));
  } else {
    profile_scan = 0;
  }

  for (cur_profile_scan = -1;
       cur_profile_scan < profile_scan;
       cur_profile_scan++) {

    if (cur_profile_scan == 0) continue;

    // 根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-longSynthesizer.txt,
    // LogDebug表示输出所有级别日志
    // 需要最早调用
  #ifdef LOG_TRIGGER
    int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
        "log-longSynthesizer", AlibabaNls::LogDebug, 400, 50);
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
    //AlibabaNls::NlsClient::getInstance()->setUseSysGetAddrInfo(true);

    std::cout << "startWorkThread begin... " << std::endl;

    // 启动工作线程, 在创建请求和启动前必须调用此函数
    // 入参为负时, 启动当前系统中可用的核数
    if (cur_profile_scan == -1) {
      // 高并发的情况下推荐4, 单请求的情况推荐为1
      AlibabaNls::NlsClient::getInstance()->startWorkThread(4);
    } else {
      AlibabaNls::NlsClient::getInstance()->startWorkThread(cur_profile_scan);
    }

    std::cout << "startWorkThread finish" << std::endl;

    // 合成多个文本
    speechSynthesizerMultFile(g_appkey.c_str(), g_threads);

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

      std::cout << "threads run count:" << run_count
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

  pthread_mutex_destroy(&params_mtx);

  return 0;
}
