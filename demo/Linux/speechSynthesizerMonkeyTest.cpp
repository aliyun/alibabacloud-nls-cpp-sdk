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

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstdlib>
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

#define SAMPLE_RATE_8K 8000
#define SAMPLE_RATE_16K 16000
#define SELF_TESTING_TRIGGER
#define LOOP_TIMEOUT 60
#define LOG_TRIGGER
//#define TTS_AUDIO_DUMP
#define DEFAULT_STRING_LEN 128
#define AUDIO_TEXT_LENGTH 1024

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey
 * Secret重新生成一个token，并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
 */

enum RequestStatus {
  RequestInvalid = 0,
  RequestCreated,
  RequestStart,
  RequestStarted,
  RequestFailed,
  RequestRunning = 5,
  RequestStop,
  RequestClosed,
  RequestCancelled,
  RequestReleased = 9
};

// 自定义线程参数
struct ParamStruct {
  char text[AUDIO_TEXT_LENGTH];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char audioFile[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  pthread_mutex_t mtx;

  int status;
};

// 自定义事件回调参数
struct ParamCallBack {
 public:
  explicit ParamCallBack(ParamStruct* param) {
    userId = 0;
    memset(userInfo, 0, 8);
    tParam = param;
  };
  ~ParamCallBack() { tParam = NULL; };

  unsigned long userId;  // 这里用线程号
  char userInfo[8];

  ParamStruct* tParam;
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
static int loop_timeout = LOOP_TIMEOUT; /*循环运行的时间, 单位s*/
static int loop_count = 0; /*循环测试某音频文件的次数, 设置后loop_timeout无效*/

long g_expireTime = -1;
volatile static bool global_run = false;
static int sample_rate = SAMPLE_RATE_16K;
static int run_cnt = 0;
static int run_cancel = 0;
static int run_success = 0;
static int run_fail = 0;
static int max_msleep = 500;
static int special_type = 0;
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
           ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour,
           ltm.tm_min, ltm.tm_sec, tv.tv_usec);
  buf[63] = '\0';
  std::string tmp = buf;
  return tmp;
}

/**
 * 根据AccessKey ID和AccessKey Secret重新生成一个token，并获取其有效期时间戳
 */
int generateToken(std::string akId, std::string akSecret, std::string* token,
                  long* expireTime) {
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
    std::cout << "Failed error code: " << retCode
              << "  error msg: " << nlsTokenRequest.getErrorMsg() << std::endl;
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
unsigned int getAudioFileTimeMs(const int dataSize, const int sampleRate,
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
  std::cout << "OnSynthesisCompleted: All response:"
            << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息
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

  FILE* failed_stream = fopen("synthesisTaskFailed.log", "a+");
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

  std::cout << "OnSynthesisTaskFailed: All response:"
            << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);

    if (tmpParam->tParam->status == RequestInvalid ||
        tmpParam->tParam->status == RequestCreated ||
        tmpParam->tParam->status == RequestCancelled ||
        tmpParam->tParam->status == RequestReleased) {
      std::cout << "OnSynthesisTaskFailed invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestFailed;
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
  std::cout << "OnSynthesisChannelClosed: "
            << ", All response: " << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  if (cbParam && tmpParam->tParam) {
    if (tmpParam->tParam->status == RequestInvalid ||
        tmpParam->tParam->status == RequestCreated ||
        tmpParam->tParam->status == RequestCancelled ||
        tmpParam->tParam->status == RequestReleased) {
      std::cout << "OnSynthesisChannelClosed invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestClosed;
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
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (tmpParam) {
#if 0
    std::vector<unsigned char> data =
        cbEvent->getBinaryData();  // getBinaryData() 获取文本合成的二进制音频数据
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
    snprintf(file_name, 256, "./tts_audio/%s.wav", cbEvent->getTaskId());
    FILE* tts_stream = fopen(file_name, "a+");
    if (tts_stream) {
      fwrite((char*)&data[0], data.size(), 1, tts_stream);
      fclose(tts_stream);
    }
#endif

    if (tmpParam->tParam) {
      if (tmpParam->tParam->status == RequestInvalid ||
          tmpParam->tParam->status == RequestCreated ||
          tmpParam->tParam->status == RequestCancelled ||
          tmpParam->tParam->status == RequestReleased) {
        std::cout << "OnBinaryDataRecved invalid request status:"
                  << tmpParam->tParam->status << std::endl;
        // abort();
      }
      tmpParam->tParam->status = RequestRunning;
    }
  }
}

/**
 * @brief 返回 tts 文本对应的日志信息，增量返回对应的字幕信息
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnMetaInfo(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (tmpParam && tmpParam->tParam) {
    if (tmpParam->tParam->status == RequestInvalid ||
        tmpParam->tParam->status == RequestCreated ||
        tmpParam->tParam->status == RequestCancelled ||
        tmpParam->tParam->status == RequestReleased) {
      std::cout << "OnMetaInfo invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestRunning;
  }
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
  std::cout << "onMessage: All response:" << cbEvent->getAllResponse()
            << std::endl;
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
  std::cout << "autoCloseFunc exit..." << pthread_self() << std::endl;

  return NULL;
}

void* pthreadFunc(void* arg) {
  int testCount = 0;

  /*
   * 从自定义线程参数中获取token, 配置文件等参数.
   */
  ParamStruct* tst = static_cast<ParamStruct*>(arg);
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
    if (tst->status != RequestReleased && tst->status != RequestInvalid) {
      std::cout << "pthreadFunc invalid request status:" << tst->status
                << std::endl;
      // abort();
    }

    /*
     * 创建语音识别SpeechSynthesizerRequest对象.
     * 默认为实时短文本语音合成请求, 支持一次性合成300字符以内的文字,
     * 其中1个汉字、1个英文字母或1个标点均算作1个字符,
     * 超过300个字符的内容将会报错(或者截断).
     * 一次性合成超过300字符可考虑长文本语音合成功能.
     *
     * 实时短文本语音合成文档详见:
     * https://help.aliyun.com/document_detail/84435.html
     * 长文本语音合成文档详见:
     * https://help.aliyun.com/document_detail/130509.html
     */
    int chars_cnt =
        AlibabaNls::NlsClient::getInstance()->calculateUtf8Chars(tst->text);
    // std::cout << "pid:" << pthread_self() << " this text contains " <<
    // chars_cnt
    //           << "chars" << std::endl;

    AlibabaNls::SpeechSynthesizerRequest* request =
        AlibabaNls::NlsClient::getInstance()->createSynthesizerRequest();
    if (request == NULL) {
      std::cout << "createSynthesizerRequest failed." << std::endl;
      break;
    } else {
      cbParam.tParam->status = RequestCreated;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (rand() % 100 == 1) {
      std::cout << "Release after create() directly ..." << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request);
      cbParam.tParam->status = RequestReleased;
      continue;
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
    // 设置所有服务端返回信息回调函数
    // request->setOnMessage(onMessage, &cbParam);
    // 开启所有服务端返回信息回调函数, 其他回调(除了OnBinaryDataRecved)失效
    // request->setEnableOnMessage(true);

    if (strlen(tst->appkey) > 0) {
      request->setAppKey(tst->appkey);
    }

    // 设置待合成文本, 必填参数. 文本内容必须为UTF-8编码
    // 一次性合成超过300字符可考虑长文本语音合成功能.
    // 长文本语音合成文档详见:
    // https://help.aliyun.com/document_detail/130509.html
    request->setText(tst->text);
    // 发音人, 包含"xiaoyun", "ruoxi", "xiaogang"等. 可选参数, 默认是xiaoyun
    request->setVoice("siqi");
    // 访问个性化音色，访问的Voice必须是个人定制音色
    // request->setPayloadParam("{\"enable_ptts\":true}");
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
    // 设置账号校验token, 必填参数
    if (strlen(tst->token) > 0) {
      request->setToken(tst->token);
    }
    if (strlen(tst->url) > 0) {
      request->setUrl(tst->url);
    }
    // 获取返回文本的编码格式
    // const char* output_format = request->getOutputFormat();
    // std::cout << "text format: " << output_format << std::endl;

    /*
     * start()为异步操作。成功则开始返回BinaryRecv事件。失败返回TaskFailed事件。
     */
    std::string ts = timestamp_str();
    // std::cout << "start -> pid " << pthread_self() << " " << ts.c_str()
    //           << std::endl;
    int ret = request->start();
    ts = timestamp_str();
    run_cnt++;
    testCount++;
    if (ret < 0) {
      std::cout << "start failed. pid:" << pthread_self() << ". ret:" << ret
                << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(
          request);  // start()失败，释放request对象
      cbParam.tParam->status = RequestReleased;
      std::cout << "\n" << std::endl;
      if (ret == -160) {
        // 常见于start指令发送太多导致内部处理不过来
        usleep(500 * 1000);
      }
      continue;
    } else {
      // std::cout << "start success. pid " << pthread_self() << " " <<
      // ts.c_str()
      //           << std::endl;
      cbParam.tParam->status = RequestStart;
    }

    struct timespec outtime;
    if (special_type == 1) {
      gettimeofday(&now, NULL);
      std::srand(now.tv_usec);
      if (max_msleep <= 0) max_msleep = 1;
      int sleepMs = rand() % max_msleep;
      // std::cout << "sleep " << sleepMs << "ms." << std::endl;
      usleep(sleepMs * 1000);

      ret = request->cancel();
      cbParam.tParam->status = RequestCancelled;
      AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request);
      // std::cout << "release done." << std::endl;
      cbParam.tParam->status = RequestReleased;
      if (loop_count > 0 && testCount >= loop_count) {
        global_run = false;
      }
      continue;
    }

    /*
     * 通知云端数据发送结束.
     * stop()为无意义接口，调用与否都会跑完全程.
     * cancel()立即停止工作, 且不会有回调返回, 失败返回TaskFailed事件。
     */
    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    int type = rand() % 3;
    if (type == 0) {
      ret = request->stop();  // always return 0
    } else if (type == 1) {
      ret = request->cancel();
    }

    /*
     * 识别结束, 释放request对象
     */
    if (ret == 0) {
      gettimeofday(&now, NULL);
      std::srand(now.tv_usec);
      if (max_msleep <= 0) max_msleep = 1;
      int sleepMs = rand() % max_msleep;
      usleep(sleepMs * 1000);
      // std::cout << "usleep " << sleepMs << "ms" << std::endl;
    } else {
      std::cout << "ret is " << ret << ", pid " << pthread_self() << std::endl;
    }
    gettimeofday(&now, NULL);
    // std::cout << "stop finished. pid " << pthread_self()
    //           << " tv: " << now.tv_sec << std::endl;

    AlibabaNls::NlsClient::getInstance()->releaseSynthesizerRequest(request);
    cbParam.tParam->status = RequestReleased;
    // std::cout << "release Synthesizer success. pid " << pthread_self() <<
    // "\n"
    //           << std::endl;

    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    }
  }  // while global_run

  pthread_mutex_destroy(&(tst->mtx));

  return NULL;
}

/**
 * 合成多个文本数据;
 * sdk多线程指一个文本数据对应一个线程, 非一个文本数据对应多个线程.
 * 示例代码为同时开启4个线程合成4个文件;
 * 免费用户并发连接不能超过2个;
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
      std::cout << "the token will be expired, please generate new token by "
                   "AccessKey-ID and AccessKey-Secret."
                << std::endl;
      if (generateToken(g_akId, g_akSecret, &g_token, &g_expireTime) < 0) {
        return -1;
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

  const char syAudioFiles[AUDIO_TEXT_NUMS][AUDIO_FILE_NAME_LENGTH] = {
      "syAudio0.wav", "syAudio1.wav", "syAudio2.wav", "syAudio3.wav"};
  /* 不要超过AUDIO_TEXT_LENGTH */
  const char texts[AUDIO_TEXT_NUMS][AUDIO_TEXT_LENGTH] = {
      "今日天气真不错，我想去操场踢足球.", "今日天气真不错，我想去操场踢足球.",
      "今日天气真不错，我想去操场踢足球.", "今日天气真不错，我想去操场踢足球."};
  ParamStruct pa[threads];

  for (int i = 0; i < threads; i++) {
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

    pa[i].status = RequestInvalid;
  }

  global_run = true;
  std::vector<pthread_t> pthreadId(threads);
  // 启动四个工作线程, 同时识别四个音频文件
  for (int j = 0; j < threads; j++) {
    pthread_create(&pthreadId[j], NULL, &pthreadFunc, (void*)&(pa[j]));
  }

  std::cout << "start pthread_join..." << std::endl;

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
    } else if (!strcmp(argv[index], "--msleep")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      max_msleep = atoi(argv[index]);
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
    } else if (!strcmp(argv[index], "--special")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      special_type = atoi(argv[index]);
    }
    index++;
  }

  if (g_akId.empty() || g_akSecret.empty()) {
    std::cout << "short of params ... akId or akSecret is empty" << std::endl;
    if (g_token.empty()) {
      std::cout << "short of params ... token is empty" << std::endl;
      return 1;
    }
  }
  if (g_appkey.empty()) {
    std::cout << "short of params ... appkey is empty" << std::endl;
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
        << "  --threads <The number of requests running at the same time, "
           "default 1>\n"
        << "  --time <The time of the test run, in seconds>\n"
        << "  --msleep <The maximum sleep time after start(), or after "
           "stop()/cancel() to release()>\n"
        << "  --log <logLevel, default LogDebug = 4, closeLog = 0>\n"
        << "  --sampleRate <sample rate, default 16000>\n"
        << "  --sys <use system getaddrinfo(): 1, evdns_getaddrinfo(): 0>\n"
        << "eg:\n"
        << "  ./syMT --appkey xxxxxx --token xxxxxx\n"
        << "  ./syMT --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx --threads "
           "4 --time 3600\n"
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

  // 根据需要设置SDK输出日志, 可选. 此处表示SDK日志输出至log-SynthesizerMT.txt,
  // LogDebug表示输出所有级别日志
  // 需要最早调用
#ifdef LOG_TRIGGER
  int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
      "log-synthesizerMT", AlibabaNls::LogDebug, 100, 20);
  if (ret < 0) {
    std::cout << "set log failed." << std::endl;
    return -1;
  }
#endif

  // 设置运行环境需要的套接口地址类型, 默认为AF_INET
  // 必须在startWorkThread()前调用
  // AlibabaNls::NlsClient::getInstance()->setAddrInFamily("AF_INET");

  // 私有云部署的情况下进行直连IP的设置
  // 必须在startWorkThread()前调用
  // AlibabaNls::NlsClient::getInstance()->setDirectHost("106.15.83.44");

  // 存在部分设备在设置了dns后仍然无法通过SDK的dns获取可用的IP,
  // 可调用此接口主动启用系统的getaddrinfo来解决这个问题.
  if (sysAddrinfo) {
    AlibabaNls::NlsClient::getInstance()->setUseSysGetAddrInfo(true);
  }

  std::cout << "startWorkThread begin... " << std::endl;

  // 启动工作线程, 在创建请求和启动前必须调用此函数
  // 入参为负时, 启动当前系统中可用的核数
  // 默认推荐为1
  AlibabaNls::NlsClient::getInstance()->startWorkThread(g_cpu);

  std::cout << "startWorkThread finish" << std::endl;

  // 合成多个文本
  speechSynthesizerMultFile(g_appkey.c_str(), g_threads);

  // 所有工作完成，进程退出前，释放nlsClient.
  // 请注意, releaseInstance()非线程安全.
  std::cout << "releaseInstance -> " << std::endl;
  AlibabaNls::NlsClient::releaseInstance();
  std::cout << "releaseInstance done." << std::endl;

  return 0;
}
