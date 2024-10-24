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
#include "speechTranscriberRequest.h"

#define SELF_TESTING_TRIGGER
#define FRAME_16K_20MS 640
#define FRAME_16K_100MS 3200
#define FRAME_8K_20MS 320
#define SAMPLE_RATE_8K 8000
#define SAMPLE_RATE_16K 16000

#define OPERATION_TIMEOUT_S 5
#define LOOP_TIMEOUT 60
#define DEFAULT_STRING_LEN 128

/**
 * 全局维护一个服务鉴权token和其对应的有效期时间戳，
 * 每次调用服务之前，首先判断token是否已经过期，
 * 如果已经过期，则根据AccessKey ID和AccessKey Secret重新生成一个token，
 * 并更新这个全局的token和其有效期时间戳。
 *
 * 注意：不要每次调用服务之前都重新生成新token，
 * 只需在token即将过期时重新生成即可。所有的服务并发可共用一个token。
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
  char fileName[DEFAULT_STRING_LEN];
  char token[DEFAULT_STRING_LEN];
  char appkey[DEFAULT_STRING_LEN];
  char url[DEFAULT_STRING_LEN];

  pthread_mutex_t mtx;

  int status;
};

struct SentenceParamStruct {
  std::string text;
  struct timeval endTv;
};

// 自定义事件回调参数
class ParamCallBack {
 public:
  explicit ParamCallBack(ParamStruct* param) {
    userId = 0;
    memset(userInfo, 0, 8);
    tParam = param;
  };
  ~ParamCallBack() { tParam = NULL; };

  unsigned long userId;
  char userInfo[8];

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

std::string g_appkey = "";
std::string g_akId = "";
std::string g_akSecret = "";
std::string g_token = "";
std::string g_domain = "";
std::string g_api_version = "";
std::string g_url = "";
std::string g_audio_path = "";
std::string g_audio_dir = "";
std::vector<std::string> g_wav_files;
std::string g_log_file = "log-transcriberMT";
int g_log_count = 20;
int g_threads = 1;
int g_cpu = 1;
static int loop_timeout = LOOP_TIMEOUT; /*循环运行的时间, 单位s*/
static int loop_count = 0; /*循环测试某音频文件的次数, 设置后loop_timeout无效*/

long g_expireTime = -1;
volatile static bool global_run = false;
static pthread_mutex_t params_mtx; /*全局统计参数g_statistics的操作锁*/
static int sample_rate = SAMPLE_RATE_16K;
static int frame_size = FRAME_16K_20MS; /*每次推送音频字节数.*/
static int encoder_type = ENCODER_OPUS;
static int logLevel = AlibabaNls::LogDebug; /* 0:为关闭log */
static int max_sentence_silence = 0; /*最大静音断句时间, 单位ms. 默认不设置.*/
static int run_cnt = 0;
static int run_start_failed = 0;
static int run_cancel = 0;
static int run_success = 0;
static int run_fail = 0;
static int max_msleep = 500;
static int special_type = 0;

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
           ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour,
           ltm.tm_min, ltm.tm_sec, tv.tv_usec);
  buf[63] = '\0';
  std::string tmp = buf;
  return tmp;
}

void findWavFiles(const std::string& directory,
                  std::vector<std::string>& wavFiles) {
  DIR* dir;
  struct dirent* ent;

  // 打开目录
  if ((dir = opendir(directory.c_str())) != NULL) {
    // 读取目录中的每个文件
    while ((ent = readdir(dir)) != NULL) {
      std::string fileName = ent->d_name;
      // 检查文件名是否以 .wav 结尾
      if (fileName.size() >= 4 &&
          fileName.substr(fileName.size() - 4) == ".wav") {
        wavFiles.push_back(directory + "/" + fileName);
      }
    }
    closedir(dir);
  } else {
    std::cerr << "无法打开目录: " << directory << std::endl;
  }
}

std::string getWavFile(std::vector<std::string>& wavFiles) {
  if (!wavFiles.empty()) {
    std::srand(std::time(0));
    int randomIndex = std::rand() % wavFiles.size();  // 随机索引
    return wavFiles[randomIndex];
  }
  return "";
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
 * @brief 获取sendAudio发送延时时间
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
unsigned int getSendAudioSleepTime(const int dataSize, const int sampleRate,
                                   const int compressRate) {
  int sleepMs = getAudioFileTimeMs(dataSize, sampleRate, compressRate);
  // std::cout << "data size: " << dataSize << "bytes, sleep: " << sleepMs <<
  // "ms." << std::endl;
  return sleepMs;
}

/**
 * @brief 调用start(), 成功与云端建立连接, sdk内部线程上报started事件
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void onTranscriptionStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "onTranscriptionStarted: All response:"
            << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;

    if (tmpParam->tParam->status != RequestStart &&
        tmpParam->tParam->status != RequestStop) {
      std::cout << "  onTranscriptionStarted invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestStarted;
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
  std::cout << "onSentenceBegin: All response:" << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  if (tmpParam->tParam->status != RequestStarted &&
      tmpParam->tParam->status != RequestRunning &&
      tmpParam->tParam->status != RequestStop) {
    std::cout << "  onSentenceBegin invalid request status:"
              << tmpParam->tParam->status << std::endl;
    // abort();
  }
  if (tmpParam->tParam->status == RequestStarted) {
    tmpParam->tParam->status = RequestRunning;
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
  std::cout << "onSentenceEnd: All response:" << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  if (tmpParam->tParam->status != RequestStarted &&
      tmpParam->tParam->status != RequestRunning &&
      tmpParam->tParam->status != RequestStop) {
    std::cout << "  onSentenceEnd invalid request status:"
              << tmpParam->tParam->status << std::endl;
    // abort();
  }
  if (tmpParam->tParam->status == RequestStarted) {
    tmpParam->tParam->status = RequestRunning;
  }
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

    std::cout << "onTranscriptionResultChanged: All response:"
              << cbEvent->getAllResponse()
              << std::endl;  // 获取服务端返回的全部信息

    if (tmpParam->tParam->status != RequestStarted &&
        tmpParam->tParam->status != RequestRunning &&
        tmpParam->tParam->status != RequestStop) {
      std::cout << "  onTranscriptionResultChanged invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    if (tmpParam->tParam->status == RequestStarted) {
      tmpParam->tParam->status = RequestRunning;
    }
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
      << " task id: " << cbEvent->getTaskId() << ", "
      << "status code: "
      << cbEvent
             ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << std::endl;
  std::cout << "onTranscriptionCompleted: All response:"
            << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息
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
    std::string ts = timestamp_str();
    char outbuf[1024] = {0};
    snprintf(outbuf, sizeof(outbuf),
             "%s status code:%d task id:%s error mesg:%s\n", ts.c_str(),
             cbEvent->getStatusCode(), cbEvent->getTaskId(),
             cbEvent->getErrorMessage());
    fwrite(outbuf, strlen(outbuf), 1, failed_stream);
    fclose(failed_stream);
  }

  std::cout << "onTaskFailed: All response:" << cbEvent->getAllResponse()
            << std::endl;  // 获取服务端返回的全部信息

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) return;

    if (tmpParam->tParam->status != RequestStarted &&
        tmpParam->tParam->status != RequestRunning &&
        tmpParam->tParam->status != RequestStop) {
      std::cout << "  onTaskFailed invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestFailed;
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

  // 这里需要解析json
  int result = cbEvent->parseJsonMsg(true);
  if (result) {
    std::cout << "onMessage: parseJsonMsg failed:" << result << std::endl;
  } else {
    switch (cbEvent->getMsgType()) {
      case AlibabaNls::NlsEvent::TaskFailed:
        break;
      case AlibabaNls::NlsEvent::TranscriptionStarted:
        break;
      case AlibabaNls::NlsEvent::Close:
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
  std::cout << "OnChannelClosed: All response: " << cbEvent->getAllResponse()
            << std::endl;  // getResponse() 可以通道关闭信息

  if (cbParam) {
    ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
    if (!tmpParam->tParam) {
      std::cout << "OnChannelClosed tParam is nullptr" << std::endl;
      return;
    }

    if (tmpParam->tParam->status != RequestStarted &&
        tmpParam->tParam->status != RequestFailed &&
        tmpParam->tParam->status != RequestRunning &&
        tmpParam->tParam->status != RequestStop) {
      std::cout << "onChannelClosed invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestClosed;
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
  std::cout << "autoCloseFunc exit..." << pthread_self() << std::endl;
  return NULL;
}

/**
 * @brief 短链接模式下工作线程
 *        以 createTranscriberRequest           <----|
 *                   |                               |
 *           request->start()                        |
 *                   |                               |
 *           request->sendAudio()                    |
 *                   |                               |
 *           request->stop()                         |
 *                   |                               |
 *           收到onChannelClosed回调                  |
 *                   |                               |
 *           releaseTranscriberRequest(request)  ----|
 *        进行循环。
 */
void* pthreadFunction(void* arg) {
  int testCount = 0;
  uint64_t sendAudio_us = 0;
  uint32_t sendAudio_cnt = 0;
  uint32_t sendAudio_cnt_max = 0;  // 读取音频文件调用sendAudio()发送完的次数
  uint32_t sendAudio_cnt_limit =
      0;  // 读取音频文件调用sendAudio()发送次数, 然后退出
  int audioFileTimeLen = 0;

  ParamStruct* tst = static_cast<ParamStruct*>(arg);
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  pthread_mutex_init(&(tst->mtx), NULL);

  /* 打开音频文件, 获取数据 */
  std::string cur_file_name = getWavFile(g_wav_files);
  if (cur_file_name.empty()) {
    cur_file_name = tst->fileName;
  }
  std::ifstream fs(cur_file_name.c_str());
  fs.open(tst->fileName, std::ios::binary | std::ios::in);
  if (!fs.is_open()) {
    std::cout << cur_file_name << " isn't exist.." << std::endl;
    return NULL;
  } else {
    fs.seekg(0, std::ios::end);
    int len = fs.tellg();
    audioFileTimeLen = getAudioFileTimeMs(len, sample_rate, 1);
  }

  uint32_t frameTimeLen = getAudioFileTimeMs(frame_size, sample_rate, 1);
  sendAudio_cnt_max = audioFileTimeLen / frameTimeLen;

  // 退出线程前释放
  ParamCallBack* cbParam = NULL;
  cbParam = new ParamCallBack(tst);
  if (!cbParam) {
    return NULL;
  }
  cbParam->userId = pthread_self();
  strcpy(cbParam->userInfo, "User.");

  do {
    if (cbParam->tParam->status != RequestReleased &&
        cbParam->tParam->status != RequestInvalid) {
      std::cout << "pthreadFunc invalid request status:"
                << cbParam->tParam->status << std::endl;
      // abort();
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    longConnection = (rand() % 2) ? true : false;

    /*
     * 创建实时音频流识别SpeechTranscriberRequest对象
     */
    AlibabaNls::SpeechTranscriberRequest* request =
        AlibabaNls::NlsClient::getInstance()->createTranscriberRequest(
            "cpp", longConnection);
    if (request == NULL) {
      std::cout << "createTranscriberRequest failed." << std::endl;
      if (cbParam) {
        delete cbParam;
        cbParam = NULL;
      }
      return NULL;
    } else {
      cbParam->tParam->status = RequestCreated;
    }

    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (rand() % 100 == 1) {
      std::cout << "Release after create() directly ..." << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseTranscriberRequest(request);
      cbParam->tParam->status = RequestReleased;
      continue;
    }

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
    // 设置所有服务端返回信息回调函数
    // request->setOnMessage(onMessage, cbParam);
    // 开启所有服务端返回信息回调函数, 其他回调(除了OnBinaryDataRecved)失效
    // request->setEnableOnMessage(true);

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
    // const char* output_format = request->getOutputFormat();
    // std::cout << "text format: " << output_format << std::endl;

    // 参数设置, 如指定声学模型
    // request->setPayloadParam("{\"model\":\"test-regression-model\"}");

    // 设置音频数据编码格式, 可选参数, 目前支持pcm,opus,opu. 默认是pcm
    if (encoder_type == ENCODER_OPUS) {
      request->setFormat("opus");
    } else if (encoder_type == ENCODER_OPU) {
      request->setFormat("opu");
    } else {
      request->setFormat("pcm");
    }
    // 设置音频数据采样率, 可选参数，目前支持16000, 8000. 默认是16000
    request->setSampleRate(sample_rate);
    // 设置是否返回中间识别结果, 可选参数. 默认false
    request->setIntermediateResult(true);
    // 设置是否在后处理中添加标点, 可选参数. 默认false
    request->setPunctuationPrediction(true);
    // 设置是否在后处理中执行数字转写, 可选参数. 默认false
    request->setInverseTextNormalization(true);

    // 语音断句检测阈值，一句话之后静音长度超过该值，即本句结束，合法参数范围200～2000(ms)，默认值800ms
    if (max_sentence_silence > 0) {
      if (max_sentence_silence > 2000 || max_sentence_silence < 200) {
        std::cout << "max sentence silence: " << max_sentence_silence
                  << " is invalid" << std::endl;
      } else {
        request->setMaxSentenceSilence(max_sentence_silence);
      }
    }

    // 语义断句，启动此功能语音断句检测功能不会生效。此功能必须开启中间识别结果。
    // request->setSemanticSentenceDetection(true);

    // request->setCustomizationId("TestId_123"); //定制模型id, 可选.
    // request->setVocabularyId("TestId_456"); //定制泛热词id, 可选.

    // 设置链接超时时间
    // request->setTimeout(5000);
    // 设置发送超时时间
    // request->setSendTimeout(5000);
    // 设置是否开启接收超时
    // request->setEnableRecvTimeout(false);

    fs.clear();
    fs.seekg(0, std::ios::beg);

    gettimeofday(&(cbParam->startTv), NULL);
    int ret = request->start();
    run_cnt++;
    testCount++;
    if (ret < 0) {
      std::cout << "start() failed: " << ret << std::endl;
      run_start_failed++;
      // start()失败，释放request对象
      AlibabaNls::NlsClient::getInstance()->releaseTranscriberRequest(request);
      cbParam->tParam->status = RequestReleased;
      std::cout << "\n" << std::endl;
      continue;
    } else {
      cbParam->tParam->status = RequestStart;
      // std::cout << "start() success." << std::endl;
    }

    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (max_msleep <= 0) max_msleep = 1;
    int sleepMs = rand() % max_msleep;
    // std::cout << "sleep " << sleepMs << "ms." << std::endl;
    usleep(sleepMs * 1000);

    if (special_type == 1) {
      ret = request->cancel();
      cbParam->tParam->status = RequestCancelled;
      AlibabaNls::NlsClient::getInstance()->releaseTranscriberRequest(request);
      // std::cout << "release done." << std::endl;
      cbParam->tParam->status = RequestReleased;
      if (loop_count > 0 && testCount >= loop_count) {
        global_run = false;
      }
      continue;
    }

    sendAudio_us = 0;
    sendAudio_cnt = 0;
    gettimeofday(&(cbParam->startAudioTv), NULL);

    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (sendAudio_cnt_max <= 0) sendAudio_cnt_max = 1;
    sendAudio_cnt_limit = rand() % sendAudio_cnt_max;
    // std::cout << " sendAudio max count:" << sendAudio_cnt_max << std::endl;
    // std::cout << " sendAudio limited count:" << sendAudio_cnt_limit
    //           << std::endl;

    while (!fs.eof()) {
      uint8_t data[frame_size];
      memset(data, 0, frame_size);

      fs.read((char*)data, sizeof(uint8_t) * frame_size);
      size_t nlen = fs.gcount();
      if (nlen == 0) {
        // std::cout << "fs empty..." << std::endl;
        continue;
      }

      struct timeval tv0, tv1;
      gettimeofday(&tv0, NULL);
      /*
       * 发送音频数据: sendAudio为异步操作, 返回负值表示发送失败,
       * 需要停止发送; 返回0 为成功.
       * notice : 返回值非成功发送字节数.
       * 若希望用省流量的opus格式上传音频数据, 则第三参数传入ENCODER_OPU
       * ENCODER_OPU/ENCODER_OPUS模式时, nlen必须为640
       */
      ret = request->sendAudio(data, nlen, (ENCODER_TYPE)encoder_type);
      // std::cout << "send audio nlen:" << nlen << ", ret:" << ret <<
      // std::endl;
      if (ret < 0) {
        // 发送失败, 退出循环数据发送
        std::cout << "send data fail(" << ret << ")." << std::endl;
        break;
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
      if (sendAudio_cnt++ >= sendAudio_cnt_limit) {
        // std::cout << " break ..." << std::endl;
        break;
      }

      if (noSleepFlag) {
        /*
         * 不进行sleep, 用于测试性能.
         */
      } else {
        /*
         * 语音数据发送控制:
         * 语音数据是实时的, 不用sleep控制速率, 直接发送即可.
         * 语音数据来自文件, 发送时需要控制速率,
         * 使单位时间内发送的数据大小接近单位时间原始语音数据存储的大小.
         */
        // 根据发送数据大小，采样率，数据压缩比 来获取sleep时间
        sleepMs = getSendAudioSleepTime(ret, sample_rate, 1);

        /*
         * 语音数据发送延时控制
         */
        if (sleepMs * 1000 > tmp_us) {
          usleep(sleepMs * 1000 - tmp_us);
        }
      }
    }  // while

    /*
     * 数据发送结束，关闭识别连接通道.
     * stop()为异步操作.
     */
    if (sendAudio_cnt > 0) {
      // std::cout << "sendAudio ave: " << (sendAudio_us / sendAudio_cnt) <<
      // "us"
      //           << std::endl;
    }

    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (max_msleep <= 0) max_msleep = 1;
    sleepMs = rand() % max_msleep;
    usleep(sleepMs * 1000);

    int type = rand() % 3;
    if (type == 0) {
      std::cout << "stop ->" << std::endl;
      // stop()后会收到所有回调，若想立即停止则调用cancel()取消所有回调
      ret = request->stop();
      cbParam->tParam->status = RequestStop;
      // std::cout << "stop done. ret " << ret << "\n" << std::endl;
    } else if (type == 1) {
      std::cout << "cancel ->" << std::endl;
      ret = request->cancel();
      cbParam->tParam->status = RequestCancelled;
      // std::cout << "cancel done. ret " << ret << "\n" << std::endl;
    }

    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (max_msleep <= 0) max_msleep = 1;
    sleepMs = rand() % max_msleep;
    usleep(sleepMs * 1000);

    AlibabaNls::NlsClient::getInstance()->releaseTranscriberRequest(request);
    cbParam->tParam->status = RequestReleased;

    if (loop_count > 0 && testCount >= loop_count) {
      global_run = false;
    }
  } while (global_run);

  // 关闭音频文件
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
#define AUDIO_FILE_NUMS 4
#define AUDIO_FILE_NAME_LENGTH 32
int speechTranscriberMultFile(const char* appkey, int threads) {
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

    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());

    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));

    memset(pa[i].url, 0, DEFAULT_STRING_LEN);
    if (!g_url.empty()) {
      memcpy(pa[i].url, g_url.c_str(), g_url.length());
    }

    pa[i].status = RequestInvalid;
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

  std::cout << "speechTranscribeMultFile exit..." << std::endl;
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
    } else if (!strcmp(argv[index], "--sampleRate")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      sample_rate = atoi(argv[index]);
      if (sample_rate == SAMPLE_RATE_8K) {
        frame_size = FRAME_8K_20MS;
      } else if (sample_rate == SAMPLE_RATE_16K) {
        frame_size = FRAME_16K_20MS;
      }
    } else if (!strcmp(argv[index], "--frameSize")) {
      index++;
      frame_size = atoi(argv[index]);
      encoder_type = ENCODER_NONE;
    } else if (!strcmp(argv[index], "--msleep")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      max_msleep = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--special")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      special_type = atoi(argv[index]);
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
    } else if (!strcmp(argv[index], "--audioDir")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_audio_dir = argv[index];
    } else if (!strcmp(argv[index], "--maxSilence")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      max_sentence_silence = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--logFile")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_log_file = argv[index];
    } else if (!strcmp(argv[index], "--logFileCount")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_log_count = atoi(argv[index]);
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
        << "  --loop <The number of requests run>\n"
        << "  --type <The audio format that is transmitted to the server, "
           "which can be opus and pcm, the default opus>\n"
        << "  --log <logLevel, default LogDebug = 4, closeLog = 0>\n"
        << "  --sampleRate <sample rate, 16000 or 8000, default is 16000.>\n"
        << "  --long <long connection: 1, short connection: 0, default 0>\n"
        << "  --sys <use system getaddrinfo(): 1, evdns_getaddrinfo(): 0>\n"
        << "  --noSleep <use sleep after sendAudio(), default 0>\n"
        << "  --audioFile <The absolute path of the audio file used for the "
           "audio input for the test>\n"
        << "  --audioDir <>\n"
        << "  --maxSilence <Maximum silence time, in milliseconds, "
           "200~6000(ms), default is 800ms>\n"
        << "  --msleep <The maximum sleep time after start() to sendAudio(), "
           "or after sendAudio() to stop()/cancel()>\n"
        << "  --special <Special test content, default 0. 1: cancel()/stop() "
           "after start().>\n"
        << "  --NlsScan <Profile scan number of CPUs>\n"
        << "  --logFile <log file>\n"
        << "  --logFileCount <The count of log file>\n"
        << "eg:\n"
        << "  ./stMT --appkey xxxxxx --token xxxxxx\n"
        << "  ./stMT --appkey xxxxxx --token xxxxxx --threads 4 --time 3600\n"
        << "  ./stMT --appkey xxxxxx --token xxxxxx --threads 4 --time 3600 "
           "--log 4 --type pcm\n"
        << "  ./stMT --appkey xxxxxx --token xxxxxx --threads 1 --loop 1 --log "
           "4 --type pcm --audioFile /home/xxx/test0.wav \n"
        << "  ./stMT --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx --threads "
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
  if (!g_audio_path.empty()) {
    std::cout << " audio files path: " << g_audio_path << std::endl;
  }
  std::cout << " loop timeout: " << loop_timeout << std::endl;
  std::cout << " loop count: " << loop_count << std::endl;
  std::cout << "\n" << std::endl;

  pthread_mutex_init(&params_mtx, NULL);

  if (!g_audio_dir.empty()) {
    findWavFiles(g_audio_dir, g_wav_files);
  }

  // 根据需要设置SDK输出日志, 可选.
  // 此处表示SDK日志输出至log-Transcriber.txt， LogDebug表示输出所有级别日志
  // 需要最早调用
  if (logLevel > 0) {
    int ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
        g_log_file.c_str(), (AlibabaNls::LogLevel)logLevel, 100, g_log_count);
    if (ret < 0) {
      std::cout << "set log failed." << std::endl;
      return -1;
    }
  }

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
  AlibabaNls::NlsClient::getInstance()->startWorkThread(g_cpu);
  std::cout << "startWorkThread finish" << std::endl;

  // 识别多个音频数据
  speechTranscriberMultFile(g_appkey.c_str(), g_threads);

  // 所有工作完成，进程退出前，释放nlsClient.
  // 请注意, releaseInstance()非线程安全.
  std::cout << "releaseInstance -> " << std::endl;
  AlibabaNls::NlsClient::releaseInstance();
  std::cout << "releaseInstance done." << std::endl;

  pthread_mutex_destroy(&params_mtx);

  return 0;
}
