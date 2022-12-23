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
#include <pthread.h>
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
#include "nlsToken.h"
/* 若需要启动Log记录, 则需要此头文件 */
#include "nlsClient.h"

std::string g_akId = "";
std::string g_akSecret = "";
std::string g_domain = "";
std::string g_api_version = "";

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
    if (!strcmp(argv[index], "--akId")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_akId = argv[index];
    } else if (!strcmp(argv[index], "--akSecret")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_akSecret = argv[index];
    } else if (!strcmp(argv[index], "--domain")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_domain = argv[index];
    } else if (!strcmp(argv[index], "--apiVersion")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_api_version = argv[index];
    }
    index++;
  }
  if (g_akId.empty() || g_akSecret.empty()) {
    std::cout << "short of params..." << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (parse_argv(argc, argv)) {
    std::cout << "params is not valid.\n"
      << "Usage:\n"
      << "  --akId <AccessKey ID>\n"
      << "  --akSecret <AccessKey Secret>\n"
      << "  --domain <url>\n"
      << "      mcos:  mcos.cn-shanghai.aliyuncs.com\n"
      << "  --apiVersion <API VERSION>\n"
      << "      mcos:  2022-08-11\n"
      << "eg:\n"
      << "  ./gtDemo --akId xxxxxx --akSecret xxxxxx --domain xxxxx\n"
      << std::endl;
    return -1;
  }

  signal(SIGINT, signal_handler_int);
  signal(SIGQUIT, signal_handler_quit);

  std::cout << " akId: " << g_akId << std::endl;
  std::cout << " akSecret: " << g_akSecret << std::endl;
  std::cout << " domain: " << g_domain << std::endl;
  std::cout << " api_version: " << g_api_version << std::endl;
  std::cout << "\n" << std::endl;

  /* 此为启动Log记录, 非必须 */
  AlibabaNls::NlsClient::getInstance()->setLogConfig(
      "log-generatetoken", AlibabaNls::LogDebug, 100, 10);

  /**
   * 获取token
   */
  AlibabaNlsCommon::NlsToken request;

  /*设置阿里云账号AccessKey Id*/
  request.setAccessKeyId(g_akId);
  /*设置阿里云账号AccessKey Secret*/
  request.setKeySecret(g_akSecret);
  /*设置Domain*/
  if (!g_domain.empty()) {
    request.setDomain(g_domain);
  }
  /*设置接口版本*/
  if (!g_api_version.empty()) {
    request.setServerVersion(g_api_version);
  }

  /*获取token, 成功返回0, 失败返回负值*/
  int ret = request.applyNlsToken();
  if (ret < 0) {
    std::cout << "generate token failed error code: "
      << ret << "  error msg: "
      << request.getErrorMsg() << std::endl; /*获取失败原因*/
    return -1;
  } else {
    std::string token = request.getToken();
    unsigned int expireTime = request.getExpireTime();

    std::cout << "Token: " << token << std::endl;
    std::cout << "ExpireTime: " << expireTime << std::endl;
  }

  /* 若启动了Log记录, 须反初始化 */
  AlibabaNls::NlsClient::releaseInstance();

  return 0;
}
