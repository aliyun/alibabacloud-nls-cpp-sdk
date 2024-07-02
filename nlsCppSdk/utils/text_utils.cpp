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
#include <Rpc.h>
#else
#include <sys/time.h>
#include <unistd.h>
#ifndef __ANDRIOD__
#include <iconv.h>
#endif
#include "uuid/uuid.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "nlog.h"
#include "nlsGlobal.h"
#include "text_utils.h"
#include "utility.h"

namespace AlibabaNls {
namespace utility {

#ifdef _MSC_VER
#define _ssnprintf _snprintf
#endif

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
           ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour,
           ltm.tm_min, ltm.tm_sec, tv.tv_usec);
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

struct timeval *TextUtils::GetTimevalFromMs(struct timeval *tv, time_t ms) {
  if (tv == NULL) {
    return NULL;
  }
  time_t timeout_ms = ms;
  tv->tv_sec = timeout_ms / 1000;
  tv->tv_usec = (timeout_ms - tv->tv_sec * 1000) * 1000;
  return tv;
}

struct timespec *TextUtils::GetTimespecFromMs(struct timespec *ts, time_t ms) {
  if (ts == NULL) {
    return NULL;
  }
  time_t timeout_ms = ms;
  ts->tv_sec = timeout_ms / 1000;
  ts->tv_nsec = (timeout_ms % 1000) * 1000000;
  return ts;
}

uint64_t TextUtils::GetTimestampMs() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}

std::string TextUtils::GetTimeFromMs(uint64_t ms) {
  char buf[64];
  struct timeval tv;
  tv.tv_sec = ms / 1000;
  uint64_t tv_msec = ms % 1000;
  time_t tt = tv.tv_sec;
  struct tm *ptm = localtime(&tt);

  snprintf(buf, sizeof(buf), "%04d-%02d-%02d_%02d:%02d:%02d.%03ld",
           ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour,
           ptm->tm_min, ptm->tm_sec, tv_msec);
  std::string s(buf);
  return s;
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
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
  };

  std::string dateStr = __DATE__;
  int year = atoi(dateStr.substr(dateStr.length() - 4).c_str());
  int month = 0;
  for (int i = 0; i < 12; i++) {
    if (dateStr.find(monthes[i]) != std::string::npos) {
      month = i + 1;
      break;
    }
  }

  std::string dayStr = dateStr.substr(4, 2);
  int day = atoi(dayStr.c_str());
  // std::string timeStr = __TIME__;
  // std::string hourStr = timeStr.substr(0, 2);
  // int hour = atoi(hourStr.c_str());

  std::string sdk_v = to_string(SDK_VERSION);

  int len_sdk_v = sdk_v.length();
  int total_len = len_sdk_v + 20;

  char *version = new char[total_len];
  sprintf(version, "V%s-%04d%02d%02d", sdk_v.c_str(), year, month, day);
  std::string ret(version);
  delete[] version;
  return ret;
}  // namespace utility

const char *TextUtils::GetGitCommitInfo() { return GIT_SHA1; }

#ifdef _MSC_VER
bool TextUtils::IsEmpty(const char *str) {
#else
__attribute__((visibility("default"))) bool TextUtils::IsEmpty(
    const char *str) {
#endif
  if (str == NULL) return true;
  return str[0] == 0;
}  // namespace AlibabaNls

std::vector<std::string> TextUtils::split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

#if defined(__ANDROID__) || defined(__linux__)
int TextUtils::codeConvert(char *from_charset, char *to_charset, char *inbuf,
                           size_t inlen, char *outbuf, size_t outlen) {
#if defined(__ANDRIOD__)
  outbuf = inbuf;
#else
  iconv_t cd;
  char **pin = &inbuf;
  char **pout = &outbuf;

  cd = iconv_open(to_charset, from_charset);
  if (cd == 0) {
    return -(IconvOpenFailed);
  }

  memset(outbuf, 0, outlen);
  if (iconv(cd, pin, &inlen, pout, &outlen) == (size_t)-1) {
    return -(IconvFailed);
  }

  iconv_close(cd);
#endif
  return Success;
}
#endif

/**
 * @brief 字符串UTF8转GBK
 * @param strUTF8 UTF8格式字符串
 * @return 转换成GBK后字符串
 */
std::string TextUtils::utf8ToGbk(const std::string &strUTF8) {
#if defined(__ANDROID__) || defined(__linux__)
  const char *msg = strUTF8.c_str();
  size_t inputLen = strUTF8.length();
  size_t outputLen = inputLen * 20;

  char *outbuf = new char[outputLen + 1];
  memset(outbuf, 0x0, outputLen + 1);

  char *inbuf = new char[inputLen + 1];
  memset(inbuf, 0x0, inputLen + 1);
  strncpy(inbuf, msg, inputLen);

  int res = codeConvert((char *)"UTF-8", (char *)"GBK", inbuf, inputLen, outbuf,
                        outputLen);
  if (res < 0) {
    LOG_ERROR("ENCODE: convert to utf8 error :%d .",
              utility::getLastErrorCode());
    return NULL;
  }

  std::string strTemp(outbuf);

  delete[] outbuf;
  outbuf = NULL;
  delete[] inbuf;
  inbuf = NULL;

  return strTemp;

#elif defined(_MSC_VER)

  int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
  unsigned short *wszGBK = new unsigned short[len + 1];
  memset(wszGBK, 0, len * 2 + 2);

  MultiByteToWideChar(CP_UTF8, 0, (char *)strUTF8.c_str(), -1,
                      (wchar_t *)wszGBK, len);

  len = WideCharToMultiByte(CP_ACP, 0, (wchar_t *)wszGBK, -1, NULL, 0, NULL,
                            NULL);

  char *szGBK = new char[len + 1];
  memset(szGBK, 0, len + 1);
  WideCharToMultiByte(CP_ACP, 0, (wchar_t *)wszGBK, -1, szGBK, len, NULL, NULL);

  std::string strTemp(szGBK);
  delete[] szGBK;
  delete[] wszGBK;

  return strTemp;

#else

  return strUTF8;

#endif
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

/**
 * @brief: 对完整发送字符串中敏感信息进行遮掩,
 * 防止日志中显示敏感信息导致账号泄露
 * @param buf_in 需要调整的日志buffer
 * @param buf_str 调整后的日志
 * @param key 日志buffer中需要调整的key
 * @param step 调整key的value中的step个字符
 * @param c 调整字符为c
 * @return:
 */
const char *TextUtils::securityDisposalForLog(char *buf_in,
                                              std::string *buf_str,
                                              std::string key,
                                              unsigned int step, char c) {
  unsigned int buf_in_size = strlen(buf_in);
  if (buf_in_size > 0) {
    char *buf_out = new char[buf_in_size + 1];
    if (buf_out) {
      std::string tmp_str(buf_in);
      std::string find_key = key; /* Sec-WebSocket-Key: or X-NLS-Token: */
      int pos2 = tmp_str.find(find_key);
      memset(buf_out, 0, buf_in_size + 1);
      strncpy(buf_out, buf_in, buf_in_size);

      if (pos2 >= 0) {
        int pos1 = 0;
        int begin = pos2 + find_key.length() + 1;
        for (pos1 = begin; pos1 < begin + step; pos1++) {
          buf_out[pos1] = c;
        }
      }

      buf_str->assign(buf_out);
      delete[] buf_out;
    }
  }
  return buf_str->c_str();
}

/**
 * @brief: 生成UUID
 * @return:
 */
std::string TextUtils::getRandomUuid() {
  char uuidBuff[48] = {0};
#ifdef _MSC_VER
  char *data = NULL;
  UUID uuidhandle;
  RPC_STATUS ret_val = UuidCreate(&uuidhandle);
  if (ret_val != RPC_S_OK) {
    LOG_ERROR("UuidCreate failed");
    return uuidBuff;
  }
  UuidToString(&uuidhandle, (RPC_CSTR *)&data);
  if (data == NULL) {
    LOG_ERROR("UuidToString data is nullptr");
    return uuidBuff;
  }
  int len = strnlen(data, 36);
  int i = 0, j = 0;
  for (i = 0; i < len; i++) {
    if (data[i] != '-') {
      uuidBuff[j++] = data[i];
    }
  }
  RpcStringFree((RPC_CSTR *)&data);
#else
  char tmp[48] = {0};
  uuid_t uuid;
  uuid_generate(uuid);
  uuid_unparse(uuid, tmp);
  int i = 0, j = 0;
  while (tmp[i]) {
    if (tmp[i] != '-') {
      uuidBuff[j++] = tmp[i];
    }
    i++;
  }
#endif
  return uuidBuff;
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

void DataUtils::ByteArrayToShortArray(char *byte, int len_byte,
                                      short *short_array) {
  int count = len_byte >> 1;
  for (int i = 0; i < count; i++) {
    short_array[i] = (short)((byte[2 * i + 1] << 8) | (byte[2 * i] & 0xff));
  }
}

void DataUtils::ShortArrayToByteArray(void *buffer, int len_short) {
  int count = len_short;
  char *tmp_char = (char *)buffer;
  short *tmp_short = (short *)buffer;
  for (int i = 0; i < count; i++) {
    char low_byte = tmp_short[i] & 0xF;
    char high_byte = (tmp_short[i] >> 8) & 0xF;
    tmp_char[i * 2] = low_byte;
    tmp_char[i * 2 + 1] = high_byte;
  }
}

}  // namespace utility
}  // namespace AlibabaNls
