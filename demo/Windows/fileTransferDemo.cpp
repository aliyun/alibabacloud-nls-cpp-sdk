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

#include <errno.h>
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

#include "FileTrans.h"
/* 若需要启动Log记录, 则需要此头文件 */
#include "nlsClient.h"

std::string g_appkey = "";
std::string g_akId = "";
std::string g_akSecret = "";
std::string g_fileLinkUrl =
    "https://gw.alipayobjects.com/os/bmw-prod/"
    "0574ee2e-f494-45a5-820f-63aee583045a.wav";

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
    } else if (!strcmp(argv[index], "--fileLinkUrl")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_fileLinkUrl = argv[index];
    }
    index++;
  }
  if ((g_fileLinkUrl.empty() && (g_akId.empty() || g_akSecret.empty())) ||
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
              << "  --fileLinkUrl <your file link url>\n"
              << "eg:\n"
              << "  ./ftDemo --appkey xxxxxx --akId xxxxxx --akSecret xxxxxx "
                 "--fileLinkUrl xxxxxxxxx\n"
              << std::endl;
    return -1;
  }

  std::cout << " appKey: " << g_appkey << std::endl;
  std::cout << " akId: " << g_akId << std::endl;
  std::cout << " akSecret: " << g_akSecret << std::endl;
  std::cout << " fileLinkUrl: " << g_fileLinkUrl << std::endl;
  std::cout << "\n" << std::endl;

  /* 此为启动Log记录, 非必须 */
  AlibabaNls::NlsClient::getInstance()->setLogConfig(
      "log-filetransfer", AlibabaNls::LogDebug, 100, 10);

  /**
   * 录音文件识别
   */
  AlibabaNlsCommon::FileTrans request;

  /*设置阿里云账号AccessKey Id*/
  request.setAccessKeyId(g_akId);
  /*设置阿里云账号AccessKey Secret*/
  request.setKeySecret(g_akSecret);
  /*设置阿里云AppKey*/
  request.setAppKey(g_appkey);
  /*设置音频文件url地址*/
  request.setFileLinkUrl(g_fileLinkUrl);

  /*开始文件识别, 成功返回0, 失败返回-1*/
  int ret = request.applyFileTrans();
  if (ret < 0) {
    std::cout << "FileTrans failed error code: " << ret
              << "  error msg: " << request.getErrorMsg()
              << std::endl; /*获取失败原因*/
    return -1;
  } else {
    std::string result = request.getResult();

    std::cout << "FileTrans successed: " << result << std::endl;
  }

  /* 若启动了Log记录, 须反初始化 */
  AlibabaNls::NlsClient::releaseInstance();

  return 0;
}
