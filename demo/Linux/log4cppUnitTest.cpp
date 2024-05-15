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

#include "nlog.h"
#include "nlsClient.h"

using namespace AlibabaNls;

#define LOOP_TIMEOUT 60
#define SELF_TESTING_TRIGGER

std::string g_vocab_path = "";
int g_threads = 1;
static int loop_timeout = LOOP_TIMEOUT; /*循环运行的时间, 单位s*/
static int loop_count = 0; /*循环测试某音频文件的次数, 设置后loop_timeout无效*/
volatile static bool global_run = false;

struct ParamStruct {
  FILE* fp;
  int line;
};

void signal_handler_int(int signo) {
  std::cout << "\nget interrupt mesg\n" << std::endl;
  global_run = false;
}
void signal_handler_quit(int signo) {
  std::cout << "\nget quit mesg\n" << std::endl;
  global_run = false;
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

void* pthreadFunction(void* arg) {
  ParamStruct* tst = static_cast<ParamStruct*>(arg);
  if (tst == NULL) {
    std::cout << "arg is not valid." << std::endl;
    return NULL;
  }

  char text[2048];
  std::srand(std::time(NULL));

  while (global_run) {
    int logLevel = rand() % AlibabaNls::LogDebug + 1;
    int sleepMs = rand() % 50;
    char* ret = NULL;

    memset(text, 0, 2048);
    ret = fgets(text, 2048, tst->fp);
    if (ret == NULL) {
      fseek(tst->fp, 0, SEEK_SET);
      continue;
    }

    std::string text_str(text);
    switch (logLevel) {
      case AlibabaNls::LogDebug:
        LOG_DEBUG("Node:%p: %s", text, text_str.c_str());
        break;
      case AlibabaNls::LogInfo:
        LOG_INFO("Node:%p: %s", text, text_str.c_str());
        break;
      case AlibabaNls::LogWarning:
        LOG_WARN("Node:%p: %s", text, text_str.c_str());
        break;
      case AlibabaNls::LogError:
        LOG_ERROR("Node:%p: %s", text, text_str.c_str());
        break;
      default:
        LOG_INFO("Node:%p: %s", text, text_str.c_str());
        break;
    }

    usleep(sleepMs * 1000);
  }

  return NULL;
}

int logSystemMultThreads(int threads) {
#ifdef SELF_TESTING_TRIGGER
  if (loop_count == 0) {
    pthread_t p_id;
    pthread_create(&p_id, NULL, &autoCloseFunc, NULL);
    pthread_detach(p_id);
  }
#endif

  char tmp[2048] = {0};
  global_run = true;
  struct ParamStruct pa;
  pa.fp = NULL;
  pa.line = 0;
  std::vector<pthread_t> pthreadId(threads);

  pa.fp = fopen(g_vocab_path.c_str(), "r");
  if (pa.fp == NULL) {
    return -1;
  }
  while (NULL != fgets(tmp, 2048, pa.fp)) pa.line++;

  fseek(pa.fp, 0, SEEK_SET);

  for (int j = 0; j < threads; j++) {
    pthread_create(&pthreadId[j], NULL, &pthreadFunction, (void*)&pa);
  }

  for (int j = 0; j < threads; j++) {
    pthread_join(pthreadId[j], NULL);
  }

  usleep(2 * 1000 * 1000);

  fclose(pa.fp);

  std::cout << "logSystemMultThreads exit..." << std::endl;
  return 0;
}

void onExternalLogCallback(const char* timestamp, int level,
                           const char* message) {
  std::cout << "onExternalLogCallback timestamp:" << timestamp << std::endl;
  std::cout << "  log level:" << level << std::endl;
  std::cout << "  log message:" << message << std::endl;
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
    if (!strcmp(argv[index], "--threads")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_threads = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--time")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      loop_timeout = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--loop")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      loop_count = atoi(argv[index]);
    } else if (!strcmp(argv[index], "--vocabFile")) {
      index++;
      if (invalied_argv(index, argc)) return 1;
      g_vocab_path = argv[index];
    }
    index++;
  }
  if (g_vocab_path.length() < 1) {
    return -1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (parse_argv(argc, argv)) {
    std::cout << "params is not valid.\n"
              << "Usage:\n"
              << "  --threads <Thread Numbers, default 1>\n"
              << "  --time <Timeout secs, default 60 seconds>\n"
              << "  --vocabFile <the absolute path of vocab file>\n"
              << "  --loop <loop count>\n"
              << "eg:\n"
              << "  ./logUnitTest --threads 4 --vocabFile ddddddd.txt\n"
              << std::endl;
    return -1;
  }

  signal(SIGINT, signal_handler_int);
  signal(SIGQUIT, signal_handler_quit);

  std::cout << " threads: " << g_threads << std::endl;
  if (!g_vocab_path.empty()) {
    std::cout << " vocab files path: " << g_vocab_path << std::endl;
  }
  std::cout << " loop timeout: " << loop_timeout << std::endl;
  std::cout << "\n" << std::endl;

  AlibabaNls::utility::NlsLog::getInstance()->logConfig(
      "log4cppUnitTest", AlibabaNls::LogDebug, 400, 5, onExternalLogCallback);

  logSystemMultThreads(g_threads);

  return 0;
}
