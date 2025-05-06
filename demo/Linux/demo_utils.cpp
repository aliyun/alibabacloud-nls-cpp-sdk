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

#include "demo_utils.h"

#include <dirent.h>
#include <sys/time.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>

std::string timestampStr(struct timeval* tv, uint64_t* tvUs) {
  char buf[64];
  struct tm ltm;

  if (tv == NULL) {
    struct timeval curTv;
    gettimeofday(&curTv, NULL);
    localtime_r(&curTv.tv_sec, &ltm);
    if (tvUs != NULL) {
      *tvUs = (uint64_t)curTv.tv_sec * 1000000 + (uint64_t)curTv.tv_usec;
    }
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
             ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour,
             ltm.tm_min, ltm.tm_sec, curTv.tv_usec);
  } else {
    gettimeofday(tv, NULL);
    localtime_r(&tv->tv_sec, &ltm);
    if (tvUs != NULL) {
      *tvUs = (uint64_t)tv->tv_sec * 1000000 + (uint64_t)tv->tv_usec;
    }
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
             ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour,
             ltm.tm_min, ltm.tm_sec, tv->tv_usec);
  }

  buf[63] = '\0';
  std::string tmp = buf;
  return tmp;
}

uint64_t getTimestampUs(struct timeval* tv) {
  if (tv == NULL) {
    struct timeval curTv;
    gettimeofday(&curTv, NULL);
    return (uint64_t)curTv.tv_sec * 1000000 + (uint64_t)curTv.tv_usec;
  } else {
    gettimeofday(tv, NULL);
    return (uint64_t)tv->tv_sec * 1000000 + (uint64_t)tv->tv_usec;
  }
}

uint64_t getTimestampMs(struct timeval* tv) {
  return getTimestampUs(tv) / 1000;
}

std::string findConnectType(const char* input) {
  const char* key = "\"connect_type\":\"";
  const char* start = strstr(input, key);
  if (start != NULL) {
    start += strlen(key);
    const char* end = strchr(start, '\"');
    if (end != NULL) {
      size_t length = end - start;
      char* value = new char[length + 1];
      strncpy(value, start, length);
      value[length] = '\0';
      std::string result(value);
      delete[] value;
      return result;
    }
  }
  return "";
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