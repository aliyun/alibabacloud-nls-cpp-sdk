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

#include <sys/time.h>
#include <unistd.h>
#include <ctime>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <signal.h>
#include <errno.h>
#include "FileTrans.h"
/* 若需要启动Log记录, 则需要此头文件 */
#include "nlsClient.h"

std::string g_appkey = "";
std::string g_akId = "";
std::string g_akSecret = "";
std::string g_stsToken = "";
std::string g_fileLinkUrl = "https://gw.alipayobjects.com/os/bmw-prod/0574ee2e-f494-45a5-820f-63aee583045a.wav";
bool g_sync = true;
int g_threads = 1;

void signal_handler_int(int signo) {
  std::cout << "\nget interrupt mesg\n" << std::endl;
}
void signal_handler_quit(int signo) {
  std::cout << "\nget quit mesg\n" << std::endl;
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
    } else if (!strcmp(argv[index], "--stsToken")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_stsToken = argv[index];
    } else if (!strcmp(argv[index], "--fileLinkUrl")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_fileLinkUrl = argv[index];
    } else if (!strcmp(argv[index], "--sync")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      if (atoi(argv[index])) {
        g_sync = true;
      } else {
        g_sync = false;
      }
    } else if (!strcmp(argv[index], "--threads")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_threads = atoi(argv[index]);
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

  if ((g_fileLinkUrl.empty() && (g_akId.empty() || g_akSecret.empty())) ||
      g_appkey.empty()) {
    std::cout << "short of params..." << std::endl;
    std::cout << "if ak/sk is empty, please setenv NLS_AK_ENV&NLS_SK_ENV&NLS_APPKEY_ENV" << std::endl;
    return 1;
  }
  return 0;
}

void fileTransferCallback(void* request, void* cbParam) {
  if (request) {
    AlibabaNlsCommon::FileTrans* result = (AlibabaNlsCommon::FileTrans*)request;
    std::map<std::string, AlibabaNlsCommon::FileTrans> *requests =
      (std::map<std::string, AlibabaNlsCommon::FileTrans> *)cbParam;

    std::string taskId = result->getTaskId();
    int event = result->getEvent();

    std::cout << "Callback taskId:" << taskId << std::endl;
    std::cout << "   event:" << event << std::endl;
    if (event == AlibabaNlsCommon::TaskFailed) {
      std::cout << "   errorMesg:" << result->getErrorMsg() << std::endl;
    } else {
      std::cout << "   result:" << result->getResult() << std::endl;
    }
    std::map<std::string, AlibabaNlsCommon::FileTrans>::iterator iter = requests->find(taskId);
    if (iter != requests->end()) {
      iter->second = *result; 
    } else {
      requests->insert(std::make_pair(taskId, *result));
    }
  } else {
    std::cout << "request is nullptr." << std::endl;
  }
  return;
}

void releaseAllRequests(std::map<std::string, AlibabaNlsCommon::FileTrans> *requests) {
  while (!requests->empty()) {
    requests->erase(requests->begin());
  }

  return;
}

void waitComplete(
    int threads, int cnt,
    std::map<std::string, AlibabaNlsCommon::FileTrans> *requests) {
  int completed = 0;
  int failed = 0;
  int requests_cnt = threads;
  int started_cnt = cnt;

  while ((completed + failed) != started_cnt) {
    failed = 0;
    completed = 0;

    std::map<std::string, AlibabaNlsCommon::FileTrans>::iterator iter;
    for (iter = requests->begin(); iter != requests->end(); iter++) {
      int event = iter->second.getEvent();
      if (event == AlibabaNlsCommon::TaskFailed) {
        failed++;
      } else if (event == AlibabaNlsCommon::TaskCompleted) {
        completed++;
      }
    }

    std::cout << "--- ---" << std::endl;
    std::cout << "  requests count: " << requests_cnt << std::endl;
    std::cout << "  requests started: " << started_cnt << std::endl;
    std::cout << "  requests completed: " << completed << std::endl;
    std::cout << "  requests failed: " << failed << std::endl;

    usleep(500 * 1000);
  }

  return;
}

int fileTransferMultThreads(
    int threads, std::map<std::string, AlibabaNlsCommon::FileTrans> *requests,
    AlibabaNlsCommon::FileTrans *request_array) {
  int cnt = 0;

  for (int i = 0; i < threads; i++) {
    /*设置文件识别事件回调*/
    request_array[i].setEventListener(fileTransferCallback, requests);
    /*设置音频文件url地址*/
    request_array[i].setFileLinkUrl(g_fileLinkUrl);

    /*设置阿里云AppKey*/
    request_array[i].setAppKey(g_appkey);

    /*设置STS临时访问凭证说明：
     *  此处账号AccessKey Id、AccessKey Secret、stsToken为临时账号信息
     *  如何获取请查看:
     *  https://help.aliyun.com/document_detail/466615.html#b1c9b9b3702ze
     */
    if (!g_stsToken.empty()) {
      //  设置sts AccessKey Id
      request_array[i].setAccessKeyId(g_akId);
      //  设置sts AccessKey Secret
      request_array[i].setKeySecret(g_akSecret);
      //  设置sts token
      request_array[i].setStsToken(g_stsToken);
    } else {
    /*设置原阿里云账号访问账号说明：
     *  此处阿里云账号AccessKey Id和AccessKey Secret
     *  此方案存在账号泄露的风险，推荐使用STS临时访问方案
     */
      //  设置阿里云账号AccessKey Id
      request_array[i].setAccessKeyId(g_akId);
      //  设置阿里云账号AccessKey Secret
      request_array[i].setKeySecret(g_akSecret);
    }


    /*设置闲时版的说明：
     *  录音文件识别闲时版是针对已经录制完成的录音文件，进行离线识别的服务。
     *  录音文件识别闲时版是非实时的，识别的文件需要提交基于HTTP可访问的URL地址，不支持提交本地文件。
     *  与录音文件识别区别在于返回时间不同，闲时版为24小时内返回结果。
     *  具体说明和设置参数请看https://help.aliyun.com/document_detail/397307.html
     */
    //  设置闲时版连接域名,这里默填写的是上海地域,如果要使用其它地域请参考各地域POP调用参数
    //request_array[i].setDomain("speechfiletranscriberlite.cn-shanghai.aliyuncs.com");
    //  设置闲时版连接版本
    //request_array[i].setServerVersion("2021-12-21");
    //  设置连接地域
    //request_array[i].setRegionId("cn-shanghai");


    /*开始文件识别, 成功返回0, 失败返回非0*/
    int ret = request_array[i].applyFileTrans(false);
    if (ret) {
      std::cout << "FileTrans (" << i << ") failed error code: "
        << ret << "  error msg: "
        << request_array[i].getErrorMsg() << std::endl; /*获取失败原因*/
    } else {
      requests->insert(
          std::make_pair(request_array[i].getTaskId(), request_array[i]));
      cnt++;
    }
  } // for ()

  return cnt;
}

int fileTransferSync() {
  /**
   * 录音文件识别
   */
  AlibabaNlsCommon::FileTrans request;

  /*设置阿里云账号AccessKey Secret*/
  request.setKeySecret(g_akSecret);
  /*设置阿里云AppKey*/
  request.setAppKey(g_appkey);
  /*设置音频文件url地址*/
  request.setFileLinkUrl(g_fileLinkUrl);

  /*设置阿里云AppKey*/
  request.setAppKey(g_appkey);

  /*设置STS临时访问凭证说明：
   *  此处账号AccessKey Id、AccessKey Secret、stsToken为临时账号信息
   *  如何获取请查看:
   *  https://help.aliyun.com/document_detail/466615.html#b1c9b9b3702ze
   */
  if (!g_stsToken.empty()) {
    //  设置sts AccessKey Id
    request.setAccessKeyId(g_akId);
    //  设置sts AccessKey Secret
    request.setKeySecret(g_akSecret);
    //  设置sts token
    request.setStsToken(g_stsToken);
  } else {
  /*设置原阿里云账号访问账号说明：
   *  此处阿里云账号AccessKey Id和AccessKey Secret
   *  此方案存在账号泄露的风险，推荐使用STS临时访问方案
   */
    //  设置阿里云账号AccessKey Id
    request.setAccessKeyId(g_akId);
    //  设置阿里云账号AccessKey Secret
    request.setKeySecret(g_akSecret);
  }


  /*设置闲时版的说明：
   *  录音文件识别闲时版是针对已经录制完成的录音文件，进行离线识别的服务。
   *  录音文件识别闲时版是非实时的，识别的文件需要提交基于HTTP可访问的URL地址，不支持提交本地文件。
   *  与录音文件识别区别在于返回时间不同，闲时版为24小时内返回结果。
   *  具体说明和设置参数请看https://help.aliyun.com/document_detail/397307.html
   */
  //  设置闲时版连接域名,这里默填写的是上海地域,如果要使用其它地域请参考各地域POP调用参数
  //request.setDomain("speechfiletranscriberlite.cn-shanghai.aliyuncs.com");
  //  设置闲时版连接版本
  //request.setServerVersion("2021-12-21");
  //  设置连接地域
  //request.setRegionId("cn-shanghai");


  /*开始文件识别, 成功返回0, 失败返回负值*/
  int ret = request.applyFileTrans();
  if (ret < 0) {
    std::cout << "FileTrans failed error code: "
      << ret << "  error msg: "
      << request.getErrorMsg() << std::endl; /*获取失败原因*/
    return -1;
  } else {
    std::string result = request.getResult();

    std::cout << "FileTrans successed: " << result << std::endl;
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
      << "  --fileLinkUrl <your file link url>\n"
      << "eg:\n"
      << "  ./ftDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx --fileLinkUrl xxxxxxxxx\n"
      << "  ./ftDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx --fileLinkUrl xxxxxxxxx --sync 0 --threads 50\n"
      << std::endl;
    return -1;
  }

  signal(SIGINT, signal_handler_int);
  signal(SIGQUIT, signal_handler_quit);

  std::cout << " appKey: " << g_appkey << std::endl;
  std::cout << " akId: " << g_akId << std::endl;
  std::cout << " akSecret: " << g_akSecret << std::endl;
  std::cout << " fileLinkUrl: " << g_fileLinkUrl << std::endl;
  std::cout << "\n" << std::endl;

  /* 此为启动Log记录, 非必须 */
  AlibabaNls::NlsClient::getInstance()->setLogConfig(
      "log-filetransfer", AlibabaNls::LogDebug, 100, 10);


  if (g_sync) {
    fileTransferSync();
  } else {
    std::map<std::string, AlibabaNlsCommon::FileTrans> requests;
    AlibabaNlsCommon::FileTrans request_array[g_threads];

    int cnt = fileTransferMultThreads(g_threads, &requests, request_array);
    std::cout << "multi-threads request finish." << std::endl;

    waitComplete(g_threads, cnt, &requests);

    releaseAllRequests(&requests);
  }

  /* 若启动了Log记录, 须反初始化 */
  AlibabaNls::NlsClient::releaseInstance();

  return 0;
}
