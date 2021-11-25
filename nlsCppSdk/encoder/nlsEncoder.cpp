/*
 * Copyright 2015 Alibaba Group Holding Limited
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>

#include "opus/opus.h"
#include "opus/opus_defines.h"

#include "nlsGlobal.h"
#include "nlsEncoder.h"

#define DEFAULT_CHANNELS 1
//#define OPUS_DEBUG

void* createNlsEncoder(ENCODER_TYPE type, int channels,
                       const int sampleRate, int *errorCode) {
  int tmpCode = 0;
  int channel_num = channels;
  
  if (channel_num < 0) {
    channel_num = DEFAULT_CHANNELS;
  }

  void *pEncoder = NULL;

  if (type == ENCODER_OPUS || type == ENCODER_OPU) {
    pEncoder = opus_encoder_create(sampleRate, channel_num,
                                   OPUS_APPLICATION_VOIP, &tmpCode);
    if (pEncoder) {
      opus_encoder_ctl(
          (OpusEncoder*)pEncoder,
          OPUS_SET_VBR(1)); //动态码率:OPUS_SET_VBR(1), 固定码率:OPUS_SET_VBR(0)
      opus_encoder_ctl(
          (OpusEncoder*)pEncoder,
          OPUS_SET_BITRATE(27800)); //指定opus编码码率,比特率从 6kb/s 到 510 kb/s,想要压缩比大一些就设置码率小一点
      opus_encoder_ctl(
          (OpusEncoder*)pEncoder,
          OPUS_SET_COMPLEXITY(8)); //计算复杂度
      opus_encoder_ctl(
          (OpusEncoder*)pEncoder,
          OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE)); //设置针对语音优化
    } else {
      pEncoder = NULL;
    }

    *errorCode = tmpCode;
  }

  return (void *)pEncoder;
}

int nlsEncoding(void* encoder, ENCODER_TYPE type,
                const uint8_t* frameBuff, const int frameLen,
                unsigned char* outputBuffer, int outputSize) {
//  int16_t interBuffer[DEFAULT_FRAME_INTER_SIZE] = {0};
  int16_t *interBuffer = (int16_t *)malloc(frameLen);
  if (interBuffer == NULL) {
    return 0;
  }

  if (!encoder || !frameBuff || frameLen <= 0 ||
      !outputBuffer || outputSize <= 0) {
    free(interBuffer);
    return 0;
  }

  //uint8 to int16
  int i = 0;
//  for (i = 0; i < DEFAULT_FRAME_NORMAL_SIZE; i += 2) {
  for (i = 0; i < frameLen; i += 2) {
    interBuffer[i / 2] =
        (int16_t) ((frameBuff[i + 1] << 8 & 0xff00) |
        (frameBuff[i] & 0xff));
  }

  unsigned char* tmp = NULL;
  tmp = (unsigned char*)malloc(outputSize);
  if (!tmp) {
    free(interBuffer);
    return 0;
  }
  memset(tmp, 0x0, outputSize);

  int encoderSize = -1;
  if (type == ENCODER_OPUS || type == ENCODER_OPU) {
    encoderSize = opus_encode((OpusEncoder*)encoder,
                              interBuffer,
//                              DEFAULT_FRAME_INTER_SIZE,
                              frameLen / 2,
                              tmp,
                              outputSize);
//    printf("frameLen:%d, outputSize:%d, encoderSize:%d\n",
//        frameLen, outputSize, encoderSize);
  }
  if (encoderSize < 0) {
    free(interBuffer);
    free(tmp);
    return encoderSize;
  }

  if (type == ENCODER_OPU) {
    *(outputBuffer + 0) = (unsigned char) encoderSize;
    memcpy((outputBuffer + 1), tmp, encoderSize);
    encoderSize += 1;
  } else if (type == ENCODER_OPU) {
    memcpy(outputBuffer, tmp, encoderSize);
  }

#ifdef OPUS_DEBUG
  std::ofstream ofs;
  ofs.open("./mid_out.opu", std::ios::out | std::ios::app | std::ios::binary);
  if (ofs.is_open()) {
    ofs.write((const char*)outputBuffer, encoderSize);
    ofs.flush();
    ofs.close();
  }
#endif

  if (tmp) free(tmp);
  tmp = NULL;

  if (interBuffer) free(interBuffer);
  interBuffer = NULL;
  return encoderSize;
}

void destroyNlsEncoder(void* encoder, ENCODER_TYPE type) {
  if (!encoder) {
    return;
  }

  if (type == ENCODER_OPUS || type == ENCODER_OPU) {
    opus_encoder_destroy((OpusEncoder*)encoder);
  }
}

