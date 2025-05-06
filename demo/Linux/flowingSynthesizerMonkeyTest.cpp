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
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
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

#include "flowingSynthesizerRequest.h"
#include "nlsClient.h"
#include "nlsEvent.h"
#include "nlsToken.h"

#define SELF_TESTING_TRIGGER
#define SAMPLE_RATE_16K 16000
#define LOOP_TIMEOUT 60
#define LOG_TRIGGER
#define DEFAULT_STRING_LEN 512
#define AUDIO_TEXT_LENGTH 2048

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
std::string g_voice = "xiaoyun";
std::string g_log_file = "log-flowingSynthesizerMT";
int g_log_count = 20;
int g_threads = 1;
int g_cpu = 1;
std::string g_text = "";
std::string g_text_file = "";
std::string g_format = "wav";
static int loop_timeout = LOOP_TIMEOUT; /*循环运行的时间, 单位s*/
static int loop_count = 0; /*循环测试某音频文件的次数, 设置后loop_timeout无效*/

long g_expireTime = -1;
volatile static bool global_run = false;
static int sample_rate = SAMPLE_RATE_16K;
static int max_msleep = 1500;
static int special_type = 0; /*1:随机时间后中止交互*/
static bool sendFlushFlag = false;
static bool enableLongSilence = false;

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

bool isNotEmptyAndNotSpace(const char* str) {
  if (str == NULL) {
    return false;
  }
  size_t length = strlen(str);
  if (length == 0) {
    return false;
  }
  for (size_t i = 0; i < length; ++i) {
    if (!std::isspace(static_cast<unsigned char>(str[i]))) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> splitString(
    const std::string& str, const std::vector<std::string>& delimiters) {
  std::vector<std::string> result;
  size_t startPos = 0;

  // 查找字符串中的每个分隔符
  while (startPos < str.length()) {
    size_t minPos = std::string::npos;
    size_t delimiterLength = 0;

    for (std::vector<std::string>::const_iterator it = delimiters.begin();
         it != delimiters.end(); ++it) {
      std::size_t position = str.find(*it, startPos);
      // 查找最近的分隔符
      if (position != std::string::npos &&
          (minPos == std::string::npos || position < minPos)) {
        minPos = position;
        delimiterLength = it->size();
      }
    }

    // 如果找到分隔符，则提取前面的字符串
    if (minPos != std::string::npos) {
      result.push_back(str.substr(startPos, minPos - startPos));
      startPos = minPos + delimiterLength;
    } else {
      // 没有更多分隔符，剩下的全部是一个部分
      result.push_back(str.substr(startPos));
      break;
    }
  }

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

void OnSynthesisStarted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  std::cout << "OnSynthesisStarted:"
            << "  status code: " << cbEvent->getStatusCode()
            << "  task id: " << cbEvent->getTaskId()
            << "  all response:" << cbEvent->getAllResponse() << std::endl;
}

/**
 * @brief sdk在接收到云端返回合成结束消息时, sdk内部线程上报Completed事件
 * @note 上报Completed事件之后，SDK内部会关闭识别连接通道.
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnSynthesisCompleted(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
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
  FILE* failed_stream = fopen("flowingSynthesisTaskFailed.log", "a+");
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
  std::string ts = timestamp_str();
  std::cout << "OnSynthesisChannelClosed: " << ts.c_str()
            << ", userId: " << tmpParam->userId
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
  std::string ts = timestamp_str();
  std::cout
      << "  OnBinaryDataRecved: " << ts.c_str() << ", "
      << "status code: "
      << cbEvent
             ->getStatusCode()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << ", userId: " << tmpParam->userId << ", taskId: "
      << cbEvent->getTaskId()  // 当前任务的task id，方便定位问题，建议输出
      << ", data size: " << data.size()  // 数据的大小
      << std::endl;
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

void OnSentenceBegin(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  if (tmpParam && tmpParam->tParam) {
    if (tmpParam->tParam->status == RequestInvalid ||
        tmpParam->tParam->status == RequestCreated ||
        tmpParam->tParam->status == RequestCancelled ||
        tmpParam->tParam->status == RequestReleased) {
      std::cout << "OnSentenceBegin invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestRunning;
  }
#if 0
  std::cout
      << "OnSentenceBegin "
      << "Response: "
      << cbEvent
             ->getAllResponse()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << std::endl;
#endif
}

void OnSentenceEnd(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  if (tmpParam && tmpParam->tParam) {
    if (tmpParam->tParam->status == RequestInvalid ||
        tmpParam->tParam->status == RequestCreated ||
        tmpParam->tParam->status == RequestCancelled ||
        tmpParam->tParam->status == RequestReleased) {
      std::cout << "OnSentenceEnd invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestRunning;
  }
#if 0
  std::cout
      << "OnSentenceEnd "
      << "Response: "
      << cbEvent
             ->getAllResponse()  // 获取消息的状态码，成功为0或者20000000，失败时对应失败的错误码
      << std::endl;
#endif
}

/**
 * @brief 返回 tts 文本对应的日志信息，增量返回对应的字幕信息
 * @param cbEvent 回调事件结构, 详见nlsEvent.h
 * @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
 * @return
 */
void OnSentenceSynthesis(AlibabaNls::NlsEvent* cbEvent, void* cbParam) {
  ParamCallBack* tmpParam = static_cast<ParamCallBack*>(cbParam);
  if (tmpParam && tmpParam->tParam) {
    if (tmpParam->tParam->status == RequestInvalid ||
        tmpParam->tParam->status == RequestCreated ||
        tmpParam->tParam->status == RequestCancelled ||
        tmpParam->tParam->status == RequestReleased) {
      std::cout << "OnSentenceSynthesis invalid request status:"
                << tmpParam->tParam->status << std::endl;
      // abort();
    }
    tmpParam->tParam->status = RequestRunning;
  }
#if 0
  ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
  std::cout << "OnSentenceSynthesis "
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
    sleep(1);
  }
  global_run = false;
  std::cout << "autoCloseFunc exit..." << pthread_self() << std::endl;

  return NULL;
}

void* pthreadFunc(void* arg) {
  int testCount = 0;  // 运行次数计数，用于超过设置的loop次数后退出
  bool timedwait_flag = false;

  // 从自定义线程参数中获取token, 配置文件等参数.
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

  int total_count = 0;
  if (!g_text_file.empty()) {
    std::ifstream file(g_text_file.c_str());
    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        total_count++;
      }
      file.close();
    }
  }

  while (global_run) {
    if (tst->status != RequestReleased && tst->status != RequestInvalid) {
      std::cout << "pthreadFunc invalid request status:" << tst->status
                << std::endl;
      // abort();
    }

    /*
     * 1. 创建流式文本语音合成FlowingSynthesizerRequest对象.
     *
     * 流式文本语音合成文档详见:
     * https://help.aliyun.com/zh/isi/developer-reference/streaming-text-to-speech-synthesis/
     */

    AlibabaNls::FlowingSynthesizerRequest* request =
        AlibabaNls::NlsClient::getInstance()->createFlowingSynthesizerRequest();
    if (request == NULL) {
      std::cout << "createFlowingSynthesizerRequest failed." << std::endl;
      break;
    } else {
      cbParam.tParam->status = RequestCreated;
    }

    // 随机在创建后直接释放
    struct timeval now;
    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (rand() % 100 == 1) {
      std::cout << "Release after create() directly ..." << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseFlowingSynthesizerRequest(
          request);
      cbParam.tParam->status = RequestReleased;
      continue;
    }

    /*
     * 2. 设置用于接收结果的回调
     */
    // 设置音频合成可以开始的回调函数
    request->setOnSynthesisStarted(OnSynthesisStarted, &cbParam);
    // 设置音频合成结束回调函数
    request->setOnSynthesisCompleted(OnSynthesisCompleted, &cbParam);
    // 设置音频合成通道关闭回调函数
    request->setOnChannelClosed(OnSynthesisChannelClosed, &cbParam);
    // 设置异常失败回调函数
    request->setOnTaskFailed(OnSynthesisTaskFailed, &cbParam);
    // 设置文本音频数据接收回调函数
    request->setOnBinaryDataReceived(OnBinaryDataRecved, &cbParam);
    // 设置字幕信息
    request->setOnSentenceSynthesis(OnSentenceSynthesis, &cbParam);
    // 一句话开始
    request->setOnSentenceBegin(OnSentenceBegin, &cbParam);
    // 一句话结束
    request->setOnSentenceEnd(OnSentenceEnd, &cbParam);
    // 设置所有服务端返回信息回调函数
    // request->setOnMessage(onMessage, &cbParam);
    // 开启所有服务端返回信息回调函数, 其他回调(除了OnBinaryDataRecved)失效
    // request->setEnableOnMessage(true);

    /*
     * 3. 设置request的相关参数
     */
    // 发音人, 包含"xiaoyun", "ruoxi", "xiaogang"等. 可选参数, 默认是xiaoyun
    request->setVoice(g_voice.c_str());
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
    // request->setTimeout(500);
    // 获取返回文本的编码格式
    // const char* output_format = request->getOutputFormat();
    // std::cout << "text format: " << output_format << std::endl;

    /*
     * 4.
     * start()为异步操作。成功则开始返回BinaryRecv事件。失败返回TaskFailed事件。
     */
    // std::string ts = timestamp_str();
    // std::cout << "start -> pid " << pthread_self() << " " << ts.c_str()
    //           << std::endl;
    int ret = request->start();
    // ts = timestamp_str();
    testCount++;
    if (ret < 0) {
      std::cout << "start failed. pid:" << pthread_self() << ". ret:" << ret
                << std::endl;
      AlibabaNls::NlsClient::getInstance()->releaseFlowingSynthesizerRequest(
          request);  // start()失败，释放request对象
      cbParam.tParam->status = RequestReleased;
      std::cout << "\n" << std::endl;
      if (ret == -160) {
        // 常见于start指令发送太多导致内部处理不过来
        usleep(500 * 1000);
      }
      continue;
    } else {
      cbParam.tParam->status = RequestStart;
    }

    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    if (max_msleep <= 0) max_msleep = 1;
    int sleepMs = rand() % max_msleep;
    usleep(sleepMs * 1000);

    if (special_type == 1) {
      gettimeofday(&now, NULL);
      std::srand(now.tv_usec);
      if (max_msleep <= 0) max_msleep = 1;
      int sleepMs = rand() % max_msleep;
      std::cout << "sleep " << sleepMs << "ms." << std::endl;
      usleep(sleepMs * 1000);

      ret = request->cancel();
      cbParam.tParam->status = RequestCancelled;
      AlibabaNls::NlsClient::getInstance()->releaseFlowingSynthesizerRequest(
          request);
      // std::cout << "release done." << std::endl;
      cbParam.tParam->status = RequestReleased;
      if (loop_count > 0 && testCount >= loop_count) {
        global_run = false;
      }
      continue;
    }

    /*
     * 5. 模拟大模型流式返回文本结果，进行逐个语音合成
     */
    std::string text_str(tst->text);
    if (!g_text_file.empty() && total_count > 0) {
      std::ifstream file(g_text_file.c_str());
      if (file.is_open()) {
        std::string line;
        std::srand(std::time(0));
        int randomIndex = std::rand() % total_count;
        while (std::getline(file, line)) {
          if (randomIndex-- <= 0) {
            text_str = line;
            break;
          }
        }  // while
        file.close();
      }
    }
    if (!text_str.empty()) {
      const char* delims[] = {"。", "！", "；", "？", "\n"};
      std::vector<std::string> delimiters(
          delims, delims + sizeof(delims) / sizeof(delims[0]));
      std::vector<std::string> sentences = splitString(text_str, delimiters);
      std::cout << "total text: " << text_str << "\n" << std::endl;
      for (std::vector<std::string>::const_iterator it = sentences.begin();
           it != sentences.end(); ++it) {
        if (isNotEmptyAndNotSpace(it->c_str())) {
          std::cout << "sendText: " << *it << "\n" << std::endl;
          ret = request->sendText(it->c_str());
          if (ret < 0) {
            break;
          }

          gettimeofday(&now, NULL);
          std::srand(now.tv_usec);
          sendFlushFlag = rand() % 3 == 1 ? true : false;
          gettimeofday(&now, NULL);
          std::srand(now.tv_usec + 99);
          enableLongSilence = rand() % 3 == 1 ? true : false;

          if (sendFlushFlag) {
            if (enableLongSilence) {
              request->sendFlush("{\"enable_long_silence\":true}");
            } else {
              request->sendFlush();
            }
          }

          gettimeofday(&now, NULL);
          std::srand(now.tv_usec);
          if (max_msleep <= 0) max_msleep = 1;
          int sleepMs = rand() % max_msleep;
          usleep(sleepMs * 1000);
        }

        if (!global_run) break;
      }  // for
      if (ret < 0) {
        std::cout << "sendText failed. pid:" << pthread_self() << " ret:" << ret
                  << std::endl;
        AlibabaNls::NlsClient::getInstance()->releaseFlowingSynthesizerRequest(
            request);  // start()失败，释放request对象
        cbParam.tParam->status = RequestReleased;
        if (loop_count > 0 && testCount >= loop_count) {
          global_run = false;
        }
        continue;
      }
    }

    /*
     * 6. start成功，开始等待接收完所有合成数据。
     *    stop()为无意义接口，调用与否都会跑完全程.
     *    cancel()立即停止工作, 且不会有回调返回, 失败返回TaskFailed事件。
     */
    gettimeofday(&now, NULL);
    std::srand(now.tv_usec);
    int type = rand() % 10;
    if (type == 0) {
      ret = request->cancel();
    } else {
      ret = request->stop();
    }

    /*
     * 开始等待接收完所有合成数据。
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
    // gettimeofday(&now, NULL);
    // std::cout << "current request task_id:" << request->getTaskId()
    //           << std::endl;
    // std::cout << "stop finished. pid " << pthread_self()
    //           << " tv: " << now.tv_sec << std::endl;

    /*
     * 7. 完成所有工作后释放当前请求。
     *    请在closed事件(确定完成所有工作)后再释放, 否则容易破坏内部状态机,
     * 会强制卸载正在运行的请求。
     */
    AlibabaNls::NlsClient::getInstance()->releaseFlowingSynthesizerRequest(
        request);
    cbParam.tParam->status = RequestReleased;
    // std::cout << "release Synthesizer success. pid " << pthread_self()
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
int flowingSynthesizerMultFile(const char* appkey, int threads) {
  /**
   * 获取当前系统时间戳，判断token是否过期
   */
  std::time_t curTime = std::time(0);
  if (g_token.empty()) {
    if (g_expireTime - curTime < 10) {
      std::cout << "the token will be expired, please generate new token by "
                   "AccessKey-ID and AccessKey-Secret."
                << std::endl;
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

  /* 不要超过AUDIO_TEXT_LENGTH */
  const char texts[AUDIO_TEXT_LENGTH] = {
      "唧唧复唧唧，木兰当户织。不闻机杼声，唯闻女叹息。问女何所思，问女何所忆。"
      "女亦无所思，女亦无所忆。昨夜见军帖，可汗大点兵。军书十二卷，卷卷有爷名。"
      "阿爷无大儿，木兰无长兄。愿为市鞍马，从此替爷征。东市买骏马，西市买鞍鞯，"
      "南市买辔头，北市买长鞭。旦辞爷娘去，暮宿黄河边。不闻爷娘唤女声，但闻黄河"
      "流水鸣溅溅。旦辞黄河去，暮至黑山头。不闻爷娘唤女声，但闻燕山胡骑鸣啾啾。"
      " 万里赴戎机，关山度若飞。朔气传金柝，寒光照铁衣。将军百战死，壮士十年归"
      "。 "
      "归来见天子，天子坐明堂。策勋十二转，赏赐百千强。可汗问所欲，木兰不用尚书"
      "郎，愿驰千里足，送儿还故乡。 "
      "爷娘闻女来，出郭相扶将；阿姊闻妹来，当户理红妆；小弟闻姊来，磨刀霍霍向猪"
      "羊。开我东阁门，坐我西阁床。脱我战时袍，著我旧时裳。当窗理云鬓，对镜帖花"
      "黄。出门看火伴，火伴皆惊忙：同行十二年，不知木兰是女郎。 "
      "雄兔脚扑朔，雌兔眼迷离；双兔傍地走，安能辨我是雄雌？"};
  ParamStruct pa[threads];

  for (int i = 0; i < threads; i++) {
    memset(pa[i].token, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].token, g_token.c_str(), g_token.length());

    memset(pa[i].appkey, 0, DEFAULT_STRING_LEN);
    memcpy(pa[i].appkey, appkey, strlen(appkey));

    memset(pa[i].text, 0, AUDIO_TEXT_LENGTH);
    if (g_text.empty()) {
      memcpy(pa[i].text, texts, strlen(texts));
    } else {
      memcpy(pa[i].text, g_text.data(), strlen(g_text.data()));
    }

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

  std::cout << "flowingSynthesizerMultFile exit..." << std::endl;
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
    if (!strcmp(argv[index], "--appkey")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_appkey = argv[index];
    } else if (!strcmp(argv[index], "--akId")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_akId = argv[index];
    } else if (!strcmp(argv[index], "--akSecret")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_akSecret = argv[index];
    } else if (!strcmp(argv[index], "--token")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_token = argv[index];
    } else if (!strcmp(argv[index], "--tokenDomain")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_domain = argv[index];
    } else if (!strcmp(argv[index], "--tokenApiVersion")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_api_version = argv[index];
    } else if (!strcmp(argv[index], "--url")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_url = argv[index];
    } else if (!strcmp(argv[index], "--threads")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_threads = atoi(argv[index]);
      if (g_threads < 1) {
        g_threads = 1;
      }
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
    } else if (!strcmp(argv[index], "--sampleRate")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      sample_rate = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--format")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_format = argv[index];
    } else if (!strcmp(argv[index], "--text")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_text = argv[index];
    } else if (!strcmp(argv[index], "--textFile")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_text_file = argv[index];
    } else if (!strcmp(argv[index], "--voice")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_voice = argv[index];
    } else if (!strcmp(argv[index], "--logFile")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_log_file = argv[index];
    } else if (!strcmp(argv[index], "--logFileCount")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_log_count = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--special")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      special_type = atoi(argv[index]);
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
    std::cout << "if ak/sk is empty, please setenv "
                 "NLS_AK_ENV&NLS_SK_ENV&NLS_APPKEY_ENV"
              << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (parse_argv(argc, argv)) {
    std::cout << "params is not valid.\n"
              << "Stream input synthesizer monkey test usage:\n"
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
              << "  --loop <How many rounds to run>\n"
              << "  --format <audio format pcm opu or opus, default wav>\n"
              << "  --voice <set voice, default xiaoyun>\n"
              << "  --text <set text for synthesizing>\n"
              << "  --textFile <set text file path for synthesizing>\n"
              << "  --log <logLevel, default LogDebug = 4, closeLog = 0>\n"
              << "  --sampleRate <sample rate, 16K or 8K>\n"
              << "  --logFile <log file>\n"
              << "  --logFileCount <The count of log file>\n"
              << "eg:\n"
              << "  ./fsMT --appkey xxxxxx --token xxxxxx\n"
              << "  ./fsMT --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
                 "--threads 4 --time 3600\n"
              << "  ./fsMT --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
                 "--threads 4 --time 3600 --format wav\n"
              << std::endl;
    return -1;
  }

  signal(SIGINT, signal_handler_int);
  signal(SIGQUIT, signal_handler_quit);

  std::cout << " appKey: " << g_appkey << std::endl;
  std::cout << " akId: " << g_akId << std::endl;
  std::cout << " akSecret: " << g_akSecret << std::endl;
  std::cout << " voice: " << g_voice << std::endl;
  std::cout << " domain for token: " << g_domain << std::endl;
  std::cout << " apiVersion for token: " << g_api_version << std::endl;
  std::cout << " threads: " << g_threads << std::endl;
  std::cout << " loop timeout: " << loop_timeout << std::endl;
  std::cout << " loop count: " << loop_count << std::endl;
  std::cout << " text: " << g_text << std::endl;
  std::cout << " text file: " << g_text_file << std::endl;
  std::cout << "\n" << std::endl;

  // 根据需要设置SDK输出日志, 可选.
  // 此处表示SDK日志输出至log-flowingSynthesizerMT.txt,
  // LogDebug表示输出所有级别日志
  // 需要最早调用
  int ret = 0;
#ifdef LOG_TRIGGER
  ret = AlibabaNls::NlsClient::getInstance()->setLogConfig(
      g_log_file.c_str(), AlibabaNls::LogDebug, 100, g_log_count, NULL);
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
  // if (sysAddrinfo) {
  //   AlibabaNls::NlsClient::getInstance()->setUseSysGetAddrInfo(true);
  // }

  // g_sync_timeout等于0，即默认未调用setSyncCallTimeout()
  // 异步方式调用
  //   start():
  //   需要等待返回started事件表示成功启动，或返回TaskFailed事件表示失败。
  //   stop(): 需要等待返回closed事件则表示完成此次交互。
  // 同步方式调用
  //   start()/stop() 调用返回即表示交互启动/结束。

  std::cout << "startWorkThread begin... " << std::endl;

  // 启动工作线程, 在创建请求和启动前必须调用此函数
  // 入参为负时, 启动当前系统中可用的核数
  // 高并发的情况下推荐4, 单请求的情况推荐为1
  AlibabaNls::NlsClient::getInstance()->startWorkThread(g_cpu);

  std::cout << "startWorkThread finish" << std::endl;

  // 合成多个文本
  ret = flowingSynthesizerMultFile(g_appkey.c_str(), g_threads);
  if (ret) {
    std::cout << "flowingSynthesizerMultFile failed." << std::endl;
    AlibabaNls::NlsClient::releaseInstance();
  } else {
    // 所有工作完成，进程退出前，释放nlsClient.
    // 请注意, releaseInstance()非线程安全.
    std::cout << "releaseInstance -> " << std::endl;
    AlibabaNls::NlsClient::releaseInstance();
    std::cout << "releaseInstance done." << std::endl;
  }

  return 0;
}
