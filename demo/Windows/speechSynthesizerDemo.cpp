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
#include "speechSynthesizerRequest.h"

#define SAMPLE_RATE 16000
#define SELF_TESTING_TRIGGER
#define LOOP_TIMEOUT 60
#define LOG_TRIGGER
//#define TTS_AUDIO_DUMP
#define DEFAULT_STRING_LEN 128

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey
 * Secret重新生成一个token，并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
 */
/* 自定义线程参数 */
struct ParamStruct {
  char text[DEFAULT_STRING_LEN];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char audioFile[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  pthread_mutex_t mtx;
};

/* 自定义事件回调参数 */
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
 * 根据AccessKey ID和AccessKey Secret重新生成一个token，并获取其有效期时间戳
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
 * @brief sdk在接收到云端返回合成结束消息时, sdk内部线程上报Completed事件
 * @note 上报Completed事件之后，SDK内部会关闭识别连接通道.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnSynthesisCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_success++;
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (tmpParam) {
    std::cout
        << "OnSynthesisCompleted: "
        << "userId: " << tmpParam->userId.x << ", status code: "
        << cbEvent
               ->getStatusCode() /* 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
                                  */
        << ", task id: "
        << cbEvent->getTaskId() /* 当前任务的task id，方便定位问题，建议输出 */
        << std::endl;
    // std::cout << "OnSynthesisCompleted: All response:" <<
    // cbEvent->getAllResponse() << std::endl; /* 获取服务端返回的全部信息 */

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
 * @brief 合成过程发生异常时, sdk内部线程上报TaskFailed事件
 * @note 上报TaskFailed事件之后，SDK内部会关闭识别连接通道.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnSynthesisTaskFailed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  run_fail++;

  FILE* failed_stream = fopen("synthesisTaskFailed.log", "ab");
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

  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (tmpParam) {
    std::cout
        << "OnSynthesisTaskFailed userId: " << tmpParam->userId.x
        << "status code: "
        << cbEvent
               ->getStatusCode() /* 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
                                  */
        << ", task id: "
        << cbEvent->getTaskId() /* 当前任务的task id，方便定位问题，建议输出 */
        << ", error message: " << cbEvent->getErrorMessage() << std::endl;
    std::cout << "OnSynthesisTaskFailed: All response:"
              << cbEvent->getAllResponse()
              << std::endl; /* 获取服务端返回的全部信息 */

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
void OnSynthesisChannelClosed(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (tmpParam) {
    std::cout << "OnSynthesisChannelClosed userId: " << tmpParam->userId.x
              << ", All response: " << cbEvent->getAllResponse()
              << std::endl; /* 获取服务端返回的全部信息 */

    //  tmpParam->audioFile.close();
    //  delete tmpParam; /* 识别流程结束,释放回调参数 */

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

    /* 通知发送线程, 最终识别结果已经返回, 可以调用stop() */
    pthread_mutex_lock(&(tmpParam->mtxWord));
    pthread_cond_signal(&(tmpParam->cvWord));
    pthread_mutex_unlock(&(tmpParam->mtxWord));
  }
}

/**
 * @brief 文本上报服务端之后, 收到服务端返回的二进制音频数据,
 * SDK内部线程通过BinaryDataRecved事件上报给用户
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 * @notice 此处切记不可做block操作,只可做音频数据转存. 若在此回调中做过多操作,
 *         会阻塞后续的数据回调和completed事件回调.
 */
void OnBinaryDataRecved(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  // ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  unsigned char* data =
      cbEvent->getBinaryDataInChar(); /* 获取文本合成的二进制音频数据 */
  unsigned int data_size = cbEvent->getBinaryDataSize();
#if 1
  std::cout
      << "OnBinaryDataRecved: "
      << "status code: "
      << cbEvent
             ->getStatusCode() /* 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
                                */
      << ", taskId: "
      << cbEvent->getTaskId() /* 当前任务的task id，方便定位问题，建议输出 */
      << ", data size: " << data_size /* 数据的大小 */
      << std::endl;
#endif
#ifdef TTS_AUDIO_DUMP
  /* 以追加形式将二进制音频数据写入文件 */
  char file_name[256] = {0};
  snprintf(file_name, 256, "%s.pcm", cbEvent->getTaskId());
  FILE* tts_stream = fopen(file_name, "ab");
  if (tts_stream) {
    int ret = fwrite((char*)data, data_size, 1, tts_stream);
    fclose(tts_stream);
    std::cout << "fwrite ok " << data_size << "  ret:" << ret << std::endl;
  } else {
    std::cout << "file name:" << file_name << "cannot open" << std::endl;
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
#if 0
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnMetaInfo "
    << "Response: " << cbEvent->getAllResponse()  /* 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码 */
    << std::endl;
#endif
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
void* pthreadFunc(void* arg) {
  /*
   * 0: 从自定义线程参数中获取token, 配置文件等参数.
   */
  ParamStruct* tst = static_cast<ParamStruct*>(arg);
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  pthread_mutex_init(&(tst->mtx), NULL);

  /* 初始化自定义回调参数 */
  ParamCallBack* cbParam = new ParamCallBack(tst);
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  strcpy(cbParam->userInfo, "User.");

  while (global_run) {
    /*
     * 2: 创建语音识别SpeechSynthesizerRequest对象
     */
    AlibabaNls::SpeechSynthesizerRequest* request =
        AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest();
    if (request == NULL) {
      std::cout << "createSynthesizerRequest failed." << std::endl;
      // cbParam->audioFile.close();
      break;
    }

    /* 设置音频合成结束回调函数 */
    request->setOnSynthesisCompleted(OnSynthesisCompleted, cbParam);
    /* 设置音频合成通道关闭回调函数 */
    request->setOnChannelClosed(OnSynthesisChannelClosed, cbParam);
    /* 设置异常失败回调函数 */
    request->setOnTaskFailed(OnSynthesisTaskFailed, cbParam);
    /* 设置文本音频数据接收回调函数 */
    request->setOnBinaryDataReceived(OnBinaryDataRecved, cbParam);
    /* 设置字幕信息 */
    request->setOnMetaInfo(OnMetaInfo, cbParam);

    if (strlen(tst->appkey) > 0) {
      request->setAppKey(tst->appkey);
    }
    /* 设置待合成文本, 必填参数. 文本内容必须为UTF-8编码 */
    request->setText(tst->text);
    /* 发音人, 包含"xiaoyun", "ruoxi", "xiaogang"等. 可选参数, 默认是xiaoyun */
    request->setVoice("siqi");
    /* 音量, 范围是0~100, 可选参数, 默认50 */
    request->setVolume(50);
    /* 音频编码格式, 可选参数, 默认是wav. 支持的格式pcm, wav, mp3 */
    request->setFormat("wav");
    /* 音频采样率, 包含8000, 16000. 可选参数, 默认是16000 */
    request->setSampleRate(SAMPLE_RATE);
    /* 语速, 范围是-500~500, 可选参数, 默认是0 */
    request->setSpeechRate(0);
    /* 语调, 范围是-500~500, 可选参数, 默认是0 */
    request->setPitchRate(0);
    /* 开启字幕 */
    request->setEnableSubtitle(true);
    /* 设置账号校验token, 必填参数 */
    if (strlen(tst->token) > 0) {
      request->setToken(tst->token);
    }
    if (strlen(tst->url) > 0) {
      request->setUrl(tst->url);
    }

    /*
     * 2: start()为异步操作。成功返回BinaryRecv事件。失败返回TaskFailed事件。
     */
    gettimeofday(&(cbParam->startTv), NULL);
    int ret = request->start();
    run_cnt++;
    if (ret < 0) {
      std::cout << "start() failed." << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(
          request); /* start()失败，释放request对象 */
      // cbParam->audioFile.close();
      break;
    } else {
      std::cout << "start success. pid " << pthread_self().x << std::endl;
      struct ParamStatistics params;
      params.running = true;
      params.success_flag = false;
      params.audio_ms = 0;
      vectorSetParams(pthread_self().x, true, params);
    }

    /*
     * 6: 通知云端数据发送结束.
     * stop()为无意义接口，调用与否都会跑完全程.
     * cancel()立即停止工作, 且不会有回调返回, 失败返回TaskFailed事件。
     */
    //    ret = request->cancel();
    ret = request->stop();

    /*
     * 7: 识别结束, 释放request对象
     */
    if (ret == 0) {
      std::cout << "wait closed callback." << std::endl;
      struct timeval now;
      struct timespec outtime;
      gettimeofday(&now, NULL);
      outtime.tv_sec = now.tv_sec + 30;
      outtime.tv_nsec = now.tv_usec * 1000;
      /* 等待closed事件后再进行释放, 否则会出现崩溃 */
      pthread_mutex_lock(&(cbParam->mtxWord));
      pthread_cond_timedwait(&(cbParam->cvWord), &(cbParam->mtxWord), &outtime);
      pthread_mutex_unlock(&(cbParam->mtxWord));
    } else {
      std::cout << "ret is " << ret << std::endl;
    }
    std::cout << "stop finished" << std::endl;
    AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request);
    std::cout << "release Synthesizer success. pid " << pthread_self().x
              << std::endl;

    //    if (vectorGetFailed(cbParam->userId)) break;
  }  // while global_run

  Sleep(5 * 1000);

  pthread_mutex_destroy(&(tst->mtx));

  if (cbParam) {
    delete cbParam;
    cbParam = NULL;
  }

  return NULL;
}

/**
 * 合成多个文本数据;
 * sdk多线程指一个文本数据对应一个线程, 非一个文本数据对应多个线程.
 * 示例代码为同时开启4个线程合成4个文件;
 * 免费用户并发连接不能超过2个;
 */
#define AUDIO_TEXT_NUMS        4
#define AUDIO_TEXT_LENGTH      640
#define AUDIO_FILE_NAME_LENGTH 32
int speechSynthesizerMultFile(const char* appkey, int threads) {
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

  const char syAudioFiles[AUDIO_TEXT_NUMS][AUDIO_FILE_NAME_LENGTH] = {
      "syAudio0.wav", "syAudio1.wav", "syAudio2.wav", "syAudio3.wav"};
  const char texts[AUDIO_TEXT_NUMS][AUDIO_TEXT_LENGTH] = {
      "今日天气真不错，我想去操场踢足球.", "今日天气真不错，我想去操场踢足球.",
      "今日天气真不错，我想去操场踢足球.", "今日天气真不错，我想去操场踢足球."};
  const int c_threads = 2;
  ParamStruct pa[c_threads] = {0};

  for (int i = 0; i < threads; i++) {
    int num = i % AUDIO_TEXT_NUMS;

    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());

    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));

    memset(pa[i].text, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].text, texts[num], strlen(texts[num]));

    memset(pa[i].audioFile, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].audioFile, syAudioFiles[num], strlen(syAudioFiles[num]));

    if (!g_url.empty()) {
      memset(pa[i].url, 0, DEFAULT_STRING_LEN);
      memcpy(pa[i].url, g_url.c_str(), g_url.length());
    }
  }

  global_run = true;
  std::vector<pthread_t> pthreadId(threads);
  /* 启动四个工作线程, 同时识别四个音频文件 */
  for (int j = 0; j < threads; j++) {
    pthread_create(&pthreadId[j], NULL, &pthreadFunc, (void*)&(pa[j]));
  }

  for (int j = 0; j < threads; j++) {
    pthread_join(pthreadId[j], NULL);
  }

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
              << "eg:\n"
              << "  ./syDemo --appkey xxxxxx --token xxxxxx\n"
              << "  ./syDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
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

  /* 根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-Synthesizer.txt，
   * LogDebug表示输出所有级别日志 */
#ifdef LOG_TRIGGER
  int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
      "log-synthesizer", AlibabaNls::LogLevel::LogDebug, 400, 50);
  if (ret < 0) {
    std::cout << "set log failed." << std::endl;
    return -1;
  }
#endif

  /*
   * 设置运行环境需要的套接口地址类型, 默认为AF_INET
   */
  // AlibabaNls::NlsClient::getInstance()->setAddrInFamily("AF_INET");

  /*
   * 启动工作线程, 在创建请求和启动前必须调用此函数
   * 入参为负时, 启动当前系统中可用的核数
   */
  AlibabaNls::NlsClient::getInstance()->startWorkThread(1);

  /* 合成多个文本 */
  speechSynthesizerMultFile(g_appkey.c_str(), g_threads);

  /* 所有工作完成，进程退出前，释放nlsClient. 请注意,
   * releaseInstance()非线程安全. */
  std::cout << "releaseInstance -> " << std::endl;
  AlibabaNls::NlsClient::releaseInstance();
  std::cout << "releaseInstance done." << std::endl;

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
