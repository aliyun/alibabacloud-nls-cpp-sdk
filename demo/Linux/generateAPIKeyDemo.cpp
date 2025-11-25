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

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "dashToken.h"
/* 若需要启动Log记录, 则需要此头文件 */
#include "nlsClient.h"

std::string g_apikey = "";
int g_tokenExpirationS = 1800;

void signal_handler_int(int signo) {
  std::cout << "\nget interrupt mesg\n" << std::endl;
}
void signal_handler_quit(int signo) {
  std::cout << "\nget quit mesg\n" << std::endl;
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
    } else if (!strcmp(argv[index], "--expiration")) {
      index++;
      if (invalid_argv(index, argc)) return 1;
      g_tokenExpirationS = atoi(argv[index]);
    }
    index++;
  }
  if (g_apikey.empty()) {
    std::cout << "short of params..." << std::endl;
    return 1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (parse_argv(argc, argv)) {
    std::cout << "params is not valid.\n"
              << "Usage:\n"
              << "  --apikey <API Key>\n"
              << "  --expiration < 1- 1800 seconds>\n"
              << "eg:\n"
              << "  ./gaDemo --apikey xxxxxxn --expiration 1800" << std::endl;
    return -1;
  }

  signal(SIGINT, signal_handler_int);
  signal(SIGQUIT, signal_handler_quit);

  std::cout << " apikey: " << g_apikey << std::endl;
  std::cout << " expiration: " << g_tokenExpirationS << std::endl;
  std::cout << "\n" << std::endl;

  /* 此为启动Log记录, 非必须 */
  AlibabaNls::NlsClient::getInstance()->setLogConfig(
      "log-generateDashToken", AlibabaNls::LogDebug, 100, 10);

  /**
   * 获取token
   */
  AlibabaNlsCommon::DashToken request;

  /*设置阿里云账号API Key*/
  request.setAPIKey(g_apikey);
  if (g_tokenExpirationS > 0) {
    request.setExpireInSeconds(g_tokenExpirationS);
  }

  /*获取token, 成功返回0, 失败返回负值*/
  int ret = request.applyDashToken();
  if (ret < 0) {
    std::cout << "generate API Key token failed error code: " << ret
              << "  error msg: " << request.getErrorMsg()
              << std::endl; /*获取失败原因*/
    return -1;
  } else {
    /*
     * 示例:
     * {"token":"st-96a051xxxxxxxa9fc09a4da73a5","expires_at":1763361275}
     */
    std::string token = request.getToken();
    unsigned int expireTime = request.getExpireTime();

    std::cout << "Token: " << token << std::endl;
    std::cout << "ExpireTime: " << expireTime << std::endl;
  }

  /* 若启动了Log记录, 须反初始化 */
  AlibabaNls::NlsClient::releaseInstance();

  return 0;
}
