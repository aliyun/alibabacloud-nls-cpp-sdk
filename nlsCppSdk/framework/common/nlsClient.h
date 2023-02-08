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

#ifndef NLS_SDK_CLIENT_H
#define NLS_SDK_CLIENT_H

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <pthread.h>
#endif
#include "nlsGlobal.h"

namespace AlibabaNls {

class INlsRequest;
class SpeechRecognizerCallback;
class SpeechRecognizerRequest;
class SpeechRecognizerSyncRequest;
class SpeechTranscriberCallback;
class SpeechTranscriberRequest;
class SpeechTranscriberSyncRequest;
class SpeechSynthesizerCallback;
class SpeechSynthesizerRequest;
class DialogAssistantCallback;
class DialogAssistantRequest;
class NlsEventNetWork;

enum LogLevel {
  LogError = 1,
  LogWarning,
  LogInfo,
  LogDebug
};

enum TtsVersion {
  ShortTts = 0,
  LongTts
};

enum DaVersion {
  DaV1 = 0,
  DaV2
};

class NLS_SDK_CLIENT_EXPORT NlsClient {

 public:

  /*
   * @brief 设置日志文件与存储路径
   * @param logOutputFile 日志文件，绝对路径或者相对路径均可
                          若填写"log-transcriber"，
                          则会在程序当前目录生成log-transcriber.log；
                          若填写"/home/XXX/NlsCppSdk/log-transcriber"，
                          则会在/home/XXX/NlsCppSdk/下生成log-transcriber.log
   * @param logLevel  日志级别，默认1
   *                   （LogError : 1, LogWarning : 2, LogInfo : 3, LogDebug : 4）
   * @param logFileSize 日志文件的大小，以MB为单位，默认为10MB；
   *                    如果日志文件内容的大小超过这个值，
   *                    SDK会自动备份当前的日志文件，超过后会循环覆盖已有文件
   * @param logFileNum  日志文件循环存储最大数，默认10个文件
   * @return 成功则返回0; 失败返回负值, 详见NlsRetCode
   */
  int setLogConfig(const char* logOutputFile, const LogLevel logLevel,
                   unsigned int logFileSize = 10, unsigned int logFileNum = 10);

  /*
   * @brief 创建一句话识别对象
   * @param onResultReceivedEvent  事件回调接口
   * @param sdkName SDK的命名, 涉及到运行平台和代码语言
   * @return 成功返回speechRecognizerRequest对象，否则返回NULL
   */
  SpeechRecognizerRequest* createRecognizerRequest(
      const char* sdkName = "cpp", bool isLongConnection = false);

  /*
   * @brief 销毁一句话识别对象
   * @param request  createRecognizerRequest所建立的request对象
   * @return
   */
  void releaseRecognizerRequest(SpeechRecognizerRequest* request);

  /*
   * @brief 创建实时音频流识别对象
   * @param onResultReceivedEvent  事件回调接口
   * @param sdkName SDK的命名, 涉及到运行平台和代码语言
   * @return 成功返回SpeechTranscriberRequest对象，否则返回NULL
   */
  SpeechTranscriberRequest* createTranscriberRequest(
      const char* sdkName = "cpp", bool isLongConnection = false);

  /*
   * @brief 销毁实时音频流识别对象
   * @param request  createTranscriberRequest所建立的request对象
   * @return
   */
  void releaseTranscriberRequest(SpeechTranscriberRequest* request);

  /*
   * @brief 创建语音合成对象
   * @param type tts类型
   * @param sdkName SDK的命名, 涉及到运行平台和代码语言
   * @return 成功则SpeechSynthesizerRequest对象，否则返回NULL
   */
  SpeechSynthesizerRequest* createSynthesizerRequest(
      TtsVersion version = ShortTts,
      const char* sdkName = "cpp",
      bool isLongConnection = false);

  /*
   * @brief 销毁语音合成对象
   * @param request  createSynthesizerRequest所建立的request对象
   * @return
   */
  void releaseSynthesizerRequest(SpeechSynthesizerRequest* request);

  /*
   * @brief 创建语音助手对象
   * @param version  dialogAssistant类型
   * @param sdkName SDK的命名, 涉及到运行平台和代码语言
   * @return 成功则DialogAssistantRequest对象，否则返回NULL
   */
  DialogAssistantRequest* createDialogAssistantRequest(
      DaVersion version = DaV1,
      const char* sdkName = "cpp",
      bool isLongConnection = false);

  /*
   * @brief 销毁语音助手对象
   * @param request  createDialogAssistantRequest所建立的request对象
   * @return
   */
  void releaseDialogAssistantRequest(DialogAssistantRequest* request);

  /*
   * @brief 当前版本信息
   * @return
   */
  const char* getVersion();

  /*
   * @brief 设置套接口地址结构的类型，若调用则需要在startWorkThread之前
   * @param aiFamily 套接口地址结构类型 AF_INET/AF_INET6/AF_UNSPEC
   *                 AF_INET:  仅返回IPV4相关的地址信息
   *                 AF_INET6: 仅返回IPV6相关的地址信息
   *                 AF_UNSPEC:返回适合任何协议族的地址
   * @return
   */
  void setAddrInFamily(const char* aiFamily = "AF_INET");

  /*
   * @brief 跳过dns域名解析直接设置服务器ip地址，若调用则需要在startWorkThread之前
   * @param ipv4的ip地址 比如106.15.83.44
   * @return
   */
  void setDirectHost(const char* ip);

  /*
   * @brief 是否使用系统的getaddrinfo接口, 替代libevent的域名解析, 默认false
   *        若调用则需要在startWorkThread之前.
   *        存在部分设备在设置了dns后仍然无法通过SDK的dns获取可用的IP,
   *        可调用此接口启用系统的getaddrinfo来解决这个问题.
   * @param
   * @return
   */
  void setUseSysGetAddrInfo(bool enable);

  /*
   * @brief 待合成音频文本内容字符数
   * @note 必选参数，需要传入UTF-8编码的文本内容
   *       短文本语音合成模式下(默认), 支持一次性合成300字符以内的文字,
   *       其中1个汉字、1个英文字母或1个标点均算作1个字符,
   *       超过300个字符的内容将会报错(或者截断).
   *       超过300个字符可考虑长文本语音合成, 详见官方文档.
   * @param value	待合成文本字符串
   * @return 返回字符数
   */
  int calculateUtf8Chars(const char* value);

  /*
   * @brief 启动工作线程数量
   * @param threadsNumber 启动工作线程数量，默认设置值为1
   * @return
   */
  void startWorkThread(int threadsNumber = 1);

  /*
   * @brief NlsClient对象实例
   * @param sslInitial 是否初始化openssl 线程安全，默认为true
   * @return NlsClient对象
   */
  static NlsClient* getInstance(bool sslInitial = true);

  /*
   * @brief 销毁NlsClient对象实例
   * @note 进程退出时调用, 销毁NlsClient.
   * @return
   */
  static void releaseInstance();

  void* getNodeManger();

 private:
  NlsClient();
  ~NlsClient();

  void releaseRequest(INlsRequest*);

#ifdef _MSC_VER
  static HANDLE _mtx;
#else
  static pthread_mutex_t _mtx;
#endif
  static bool _isInitializeSSL;
  static bool _isInitializeThread;
  static NlsClient* _instance;
  static char _aiFamily[16];
  static char _direct_host_ip[64];
  static bool _enableSysGetAddr;

  void* _node_manager;

}; // class NLS_SDK_CLIENT_EXPORT NlsClient

}  // namespace AlibabaNls

#endif // NLS_SDK_CLIENT_H
