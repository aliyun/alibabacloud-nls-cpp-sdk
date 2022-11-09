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
#include <Windows.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <algorithm>

#include "text_utils.h"

namespace AlibabaNls {
namespace utility {

#ifdef _MSC_VER
#define _ssnprintf _snprintf
#endif

static std::string ret;

#ifdef _MSC_VER
static int gettimeofday(struct timeval *tp, void *tzp) {
  time_t clock;
  struct tm tm;
  SYSTEMTIME wtm;
  GetLocalTime(&wtm);
  tm.tm_year = wtm.wYear - 1900;
  tm.tm_mon = wtm.wMonth - 1;
  tm.tm_mday = wtm.wDay;
  tm.tm_hour = wtm.wHour;
  tm.tm_min = wtm.wMinute;
  tm.tm_sec = wtm.wSecond;
  tm.tm_isdst = -1;
  clock = mktime(&tm);
  tp->tv_sec = clock;
  tp->tv_usec = wtm.wMilliseconds * 1000;
  return (0);
}
#endif

std::string TextUtils::GetTime() {
  char buf[64];
  struct timeval tv;
  struct tm ltm;

#ifdef _MSC_VER
  time_t clock;
  SYSTEMTIME wtm;
  GetLocalTime(&wtm);
  ltm.tm_year = wtm.wYear - 1900;
  ltm.tm_mon = wtm.wMonth - 1;
  ltm.tm_mday = wtm.wDay;
  ltm.tm_hour = wtm.wHour;
  ltm.tm_min = wtm.wMinute;
  ltm.tm_sec = wtm.wSecond;
  ltm.tm_isdst = -1;
  clock = mktime(&ltm);
  tv.tv_sec = clock;
  tv.tv_usec = wtm.wMilliseconds * 1000;
#else
  gettimeofday(&tv, NULL);
  localtime_r(&tv.tv_sec, &ltm);
#endif

  snprintf(buf, sizeof(buf), "%04d-%02d-%02d_%02d:%02d:%02d.%06ld",
           ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
           ltm.tm_hour, ltm.tm_min, ltm.tm_sec,
           tv.tv_usec);
  std::string s(buf);
  return s;
}

std::string TextUtils::GetTimestamp() {
  std::string timestamp_s;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  long long timestamp =
    (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;

  std::stringstream timestamp_ss;
  timestamp_ss << timestamp;
  timestamp_s = timestamp_ss.str();
  return timestamp_s;
}

#ifndef GIT_SHA1
#define GIT_SHA1 "unknown" 
#endif
#ifndef SDK_VERSION
#define SDK_VERSION "unknown"
#endif
#ifndef PRODUCT_NAME
#define PRODUCT_NAME "undefined"
#endif

#ifdef _MSC_VER
std::string TextUtils::GetVersion() {
#else
__attribute__((visibility("default"))) std::string TextUtils::GetVersion() {
#endif
  std::string monthes[] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
  };

  std::string dateStr = __DATE__;
  int year = atoi(dateStr.substr(dateStr.length() - 4).c_str());
  int month = 0;
  for(int i = 0; i < 12; i++) {
    if(dateStr.find(monthes[i]) != std::string::npos) {
      month = i + 1;
      break;
    }
  }

  std::string dayStr = dateStr.substr(4, 2);
  int day = atoi(dayStr.c_str());
  std::string timeStr = __TIME__;
  std::string hourStr = timeStr.substr(0, 2);
  int hour = atoi(hourStr.c_str());

  std::string sdk_v = to_string(SDK_VERSION);

  int len_sdk_v = sdk_v.length();
  int total_len = len_sdk_v + 20;

  char *version = new char[total_len];
  sprintf(version, "V%s-%04d%02d%02d",
          sdk_v.c_str(), year, month, day);
  ret.assign(version);
  delete []version;
  return ret;
}

std::string TextUtils::GetProductName() {
  std::string product_name = to_string(PRODUCT_NAME);
  ret.assign(product_name);
  return ret;
}

const char *TextUtils::GetGitCommitInfo() {
  return GIT_SHA1;
}

#ifdef _MSC_VER
bool TextUtils::IsEmpty(const char* str) {
#else
__attribute__((visibility("default"))) bool TextUtils::IsEmpty(const char *str) {
#endif
  if (str == NULL)
    return true;
  return str[0] == 0;
}


std::vector<std::string> TextUtils::split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

int TextUtils::CharsCalculate(const char *text) {
  if (text == NULL) {
    return 0;
  }
  int count = strlen(text);
  int ret = 0;

  for (int i = 0; i < count; i++) {
    int len = Utf8Size(text[i]);
    if (len > 6 || len <= 0) {
      break;
    }
    i += len - 1;
    ret++;
  }

  return ret;
}

int TextUtils::Utf8Size(char head) {
  int len = 0;
  int one_mask = (head >> 7) & 0x1;
  int two_mask = (head >> 5) & 0x7;
  int three_mask = (head >> 4) & 0xf;
  int four_mask = (head >> 3) & 0x1f;
  int five_mask = (head >> 2) & 0x3f;
  int six_mask = (head >> 1) & 0x7f;

  if (one_mask == 0) {
    len = 1;
  } else if (two_mask == 0x6) {
    len = 2;
  } else if (three_mask == 0xe) {
    len = 3;
  } else if (four_mask == 0x1e) {
    len = 4;
  } else if (five_mask == 0x3e) {
    len = 5;
  } else if (six_mask == 0x7e) {
    len = 6;
  }

  return len;
}

void DataUtils::ByteArrayToShortArray(
    char *byte, int len_byte, short *short_array) {
  int count = len_byte >> 1;
  for (int i = 0; i < count; i++) {
    short_array[i] = (short) ((byte[2 * i + 1] << 8) | (byte[2 * i] & 0xff));
  }
}

void DataUtils::ShortArrayToByteArray(void *buffer, int len_short) {
  int count = len_short;
  char* tmp_char = (char*)buffer;
  short* tmp_short = (short*)buffer;
  char low_byte,high_byte;
  for (int i = 0; i < count; i++) {
    low_byte = tmp_short[i] & 0xF;
    high_byte = (tmp_short[i] >> 8) & 0xF;
    tmp_char[i*2] = low_byte;
    tmp_char[i*2 + 1] = high_byte;
  }
}


}  // namespace utility
}  // namespace AlibabaNls
