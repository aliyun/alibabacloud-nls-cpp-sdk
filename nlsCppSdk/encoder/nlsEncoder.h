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

#ifndef ALIBABA_NLS_ENCODER_H
#define ALIBABA_NLS_ENCODER_H

#include <stdint.h>
#ifdef ENABLE_OGGOPUS
#include "thread_data.h"
#endif

#define DEFAULT_FRAME_NORMAL_SIZE 640
#define DEFAULT_FRAME_INTER_SIZE 320

namespace AlibabaNls {

class NlsEncoder {
 public:
  /*
   * @brief 建立编码器
   * @param _event sampleRate 采样率
   * @param errorCode 错误代码
   * @return 成功返回0，失败返回负值，查看errorCode
   */
  int createNlsEncoder(ENCODER_TYPE type, int channels,
                       const int sampleRate, int *errorCode);

  /*
   * @brief 对数据进行编码
   * @param frameBuff 原始PCM音频数据
   * @param frameLen 原始PCM音频数据长度。目前仅支持640字节。
   * @param outputBuffer 装载编码后音频数据的数组
   * @param outputSize outputBuffer长度
   * @return 成功返回opu编码后的数据长度，失败返回opus错误代码
   */
  int nlsEncoding(const uint8_t* frameBuff, const int frameLen,
                  unsigned char* outputBuffer, int outputSize);

  int nlsEncoderSoftRestart();

  /*
   * @brief 释放编码器
   * @return 成功返回0，失败返回负值
   */
  int destroyNlsEncoder();

#ifdef ENABLE_OGGOPUS
  int pushbackEncodedData(const uint8_t *encoded_data, int data_len);
#endif

 private:
  void* nlsEncoder_;
  ENCODER_TYPE encoder_type_;
#ifdef ENABLE_OGGOPUS
  DataBase<uint8_t> encoded_data_;
#endif
};

}

#endif // ALIBABA_NLS_ENCODER_H
