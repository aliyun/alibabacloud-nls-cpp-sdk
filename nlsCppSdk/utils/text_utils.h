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

#ifndef NLS_SDK_TEXT_UTILS_H
#define NLS_SDK_TEXT_UTILS_H

#include <stdlib.h>
#include <sstream>
#include <string.h>
#include <vector>

namespace AlibabaNls {
namespace utility {

class TextUtils {
 public:
  static bool IsEmpty(const char *str);
  static void ByteArrayToShortArray(char *byte, int len_byte,
                                    short *short_array);
  static std::string GetTime();
  static std::string GetTimestamp();
  static const char *GetGitCommitInfo();
  static std::string GetVersion();
  static std::string GetProductName();
  static std::vector<std::string> split(const std::string &s, char delim);
  static int CharsCalculate(const char *text);

  /*
  static std::string ws_to_string(const std::wstring &str) {
    unsigned len = str.size() * 4;
    setlocale(LC_CTYPE, "");
    char *p = new char[len];
    wcstombs(p, str.c_str(), len);
    std::string str1(p);
    delete[] p;
    return str1;
  }
  static std::string &trim(std::string &s) {
    if (s.empty()) {
      return s;
    }
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
  }

  static std::wstring to_wstring(const std::string str) {  // string转wstring
    unsigned len = str.size() * 2;                         // 预留字节数
    setlocale(LC_CTYPE, "");        //必须调用此函数
    wchar_t *p = new wchar_t[len];  // 申请一段内存存放转换后的字符串
    mbstowcs(p, str.c_str(), len);  // 转换
    std::wstring str1(p);
    delete[] p;  // 释放申请的内存
    return str1;
  }
  */

  template <typename T>
  static std::string to_string(const T &n) {
    std::ostringstream stm;
    stm << n;
    return stm.str();
  }

 private:
  static int Utf8Size(char head);
};

class DataUtils {
 public:
  static void ByteArrayToShortArray(char *byte, int len_byte,
                                    short *short_array);
  static void ShortArrayToByteArray(void *buffer, int len_short);
};

}  // namespace utility
}  // namespace AlibabaNls

#endif
