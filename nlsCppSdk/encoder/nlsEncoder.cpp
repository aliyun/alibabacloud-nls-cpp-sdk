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

#include "nlog.h"
#include "nlsGlobal.h"
#include "nlsEncoder.h"
#ifdef ENABLE_OGGOPUS
#include "nlsEncoderCommon.h"
#include "oggopusEncoder.h"
#endif


#define DEFAULT_CHANNELS 1
//#define OPU_DEBUG

namespace AlibabaNls {

#ifdef ENABLE_OGGOPUS
static size_t oggopusEncodedData(const uint8_t *encoded_data, int len,
                                 void *user_data) {
  NlsEncoder *encoder = reinterpret_cast<NlsEncoder *>(user_data);
  if (encoder && encoded_data) {
    encoder->pushbackEncodedData(encoded_data, len);
  }
  return len;
}

int NlsEncoder::pushbackEncodedData(
    const uint8_t *encoded_data, int data_len) {
  encoded_data_.Pushback(encoded_data, data_len);
  return kNlsOk;
}
#endif

int NlsEncoder::createNlsEncoder(ENCODER_TYPE type, int channels,
                                 const int sampleRate, int *errorCode) {
  int ret = Success;
  int tmpCode = 0;
  int channel_num = channels;
  
  if (channel_num < 0) {
    channel_num = DEFAULT_CHANNELS;
  }
  if (nlsEncoder_ != NULL) {
    LOG_WARN("nlsEncoder_ is existent, pls destroy first");
    return -(EncoderExistent);
  }

  if (type == ENCODER_OPU) {
    nlsEncoder_ = opus_encoder_create(sampleRate, channel_num,
                                      OPUS_APPLICATION_VOIP, &tmpCode);
    if (nlsEncoder_) {
      opus_encoder_ctl(
          (OpusEncoder*)nlsEncoder_,
          OPUS_SET_VBR(1)); /* 动态码率:OPUS_SET_VBR(1), 固定码率 : OPUS_SET_VBR(0) */
      opus_encoder_ctl(
          (OpusEncoder*)nlsEncoder_,
          OPUS_SET_BITRATE(27800)); /* 指定opus编码码率, 比特率从 6kb / s 到 510 kb / s, 想要压缩比大一些就设置码率小一点 */
      opus_encoder_ctl(
          (OpusEncoder*)nlsEncoder_,
          OPUS_SET_COMPLEXITY(8)); /* 计算复杂度 */
      opus_encoder_ctl(
          (OpusEncoder*)nlsEncoder_,
          OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE)); /* 设置针对语音优化 */

      encoder_type_ = type;
      ret = Success;

      LOG_DEBUG("opus_encoder_create for OPU mode success");
    } else {
      LOG_ERROR("encoder create failed");
      nlsEncoder_ = NULL;
      ret = -(OpusEncoderCreateFailed);
    }

    *errorCode = tmpCode;
  } else if (type == ENCODER_OPUS) {
#ifdef ENABLE_OGGOPUS
    nlsEncoder_ = new OggOpusDataEncoder();
    if (nlsEncoder_ == NULL) {
      LOG_ERROR("nlsEncoder_ new OggOpusDataEncoder failed");
      return -(OggOpusEncoderCreateFailed);
    }
    ret = ((OggOpusDataEncoder *)nlsEncoder_)->OggopusEncoderCreate(
        oggopusEncodedData, this, sampleRate);
    if (ret == kNlsOk) {
      /* 这里暂时未开放编码码率和计算复杂度的设置 */
      ((OggOpusDataEncoder *)nlsEncoder_)->SetBitrate(27800);
      ((OggOpusDataEncoder *)nlsEncoder_)->SetSampleRate(sampleRate);
      ((OggOpusDataEncoder *)nlsEncoder_)->SetComplexity(8);

      LOG_DEBUG("OggopusEncoderCreate for OPUS mode success");
    } else {
      LOG_ERROR("OggopusEncoderCreate failed, errorcode:%d", ret);
    }
#endif
    encoder_type_ = type;
  }

  return ret;
}

int NlsEncoder::nlsEncoding(const uint8_t* frameBuff, const int frameLen,
                            unsigned char* outputBuffer, int outputSize) {
  if (!frameBuff || frameLen <= 0 ||
      !outputBuffer || outputSize <= 0) {
    LOG_ERROR("invalid params");
    return 0;
  }

  unsigned char* outputTmp = NULL;
  outputTmp = (unsigned char*)malloc(outputSize);
  if (!outputTmp) {
    return 0;
  }
  memset(outputTmp, 0x0, outputSize);

  int encoderSize = -1;
  /* 1. 灌入数据开始编码 */
  if (encoder_type_ == ENCODER_OPU) {
    //uint8 to int16
    int16_t *interBuffer = (int16_t *)malloc(frameLen);
    if (interBuffer == NULL) {
      LOG_ERROR("interBuffer malloc failed");
      free(outputTmp);
      return 0;
    }
    int i = 0;
    for (i = 0; i < frameLen; i += 2) {
      interBuffer[i / 2] =
          (int16_t) ((frameBuff[i + 1] << 8 & 0xff00) |
          (frameBuff[i] & 0xff));
    }

    encoderSize = opus_encode((OpusEncoder*)nlsEncoder_,
                              interBuffer,
                              frameLen / 2,
                              outputTmp,
                              outputSize);
//    LOG_DEBUG("frameLen:%d, outputSize:%d, encoderSize:%d\n",
//        frameLen, outputSize, encoderSize);

    if (encoderSize < 0) {
      free(interBuffer);
      free(outputTmp);
      return encoderSize;
    }

    if (interBuffer) free(interBuffer);
    interBuffer = NULL;
  } else if (encoder_type_ == ENCODER_OPUS) {
#ifdef ENABLE_OGGOPUS
    encoderSize = ((OggOpusDataEncoder *)nlsEncoder_)->OggopusEncode(
        (const char *)frameBuff, frameLen);
    if (encoderSize != kNlsOk) {
      LOG_ERROR("OggopusEncode failed, ret %d", encoderSize);
      free(outputTmp);
      return 0;
    }
#endif
  }

  /* 2. 取出编码后数据 */
  if (encoder_type_ == ENCODER_OPU) {
    *(outputBuffer + 0) = (unsigned char) encoderSize;
    memcpy((outputBuffer + 1), outputTmp, encoderSize);
    encoderSize += 1;
  } else if (encoder_type_ == ENCODER_OPUS) {
#ifdef ENABLE_OGGOPUS
    encoderSize = 0;
    int data_len = 0;
    data_len = encoded_data_.ElementNum();
    if (data_len > 0) {
      int array_idx = 0;
      int element_idx = 0;
      data_len = (data_len > outputSize) ? outputSize : data_len;
      encoderSize = encoded_data_.Get(
              outputTmp, data_len,
              &array_idx, &element_idx, true);
      if (encoderSize > 0) {
//        LOG_DEBUG("opus encoded %dbytes", encoderSize);
        memcpy(outputBuffer, outputTmp, encoderSize);
      }
    }
#endif
  }

#ifdef OPU_DEBUG
  if (encoderSize > 0) {
    std::ofstream ofs;
    if (encoder_type_ == ENCODER_OPU) {
      ofs.open("./mid_out.opu", std::ios::out | std::ios::app | std::ios::binary);
    } else if (encoder_type_ == ENCODER_OPUS) {
      ofs.open("./mid_out.ogg", std::ios::out | std::ios::app | std::ios::binary);
    }
    if (ofs.is_open()) {
      ofs.write((const char*)outputBuffer, encoderSize);
      ofs.flush();
      ofs.close();
    }
  }
#endif

  if (outputTmp) free(outputTmp);
  outputTmp = NULL;

  return encoderSize;
}

int NlsEncoder::nlsEncoderSoftRestart() {
  if (!nlsEncoder_) {
    LOG_WARN("nlsEncoder is inexistent");
    return -(EncoderInexistent);
  }

  if (encoder_type_ == ENCODER_OPUS) {
#ifdef ENABLE_OGGOPUS
    ((OggOpusDataEncoder *)nlsEncoder_)->OggopusSoftRestart();
#endif
  }
}

int NlsEncoder::destroyNlsEncoder() {
  if (!nlsEncoder_) {
    LOG_WARN("nlsEncoder is inexistent");
    return -(EncoderInexistent);
  }

  if (encoder_type_ == ENCODER_OPU) {
    opus_encoder_destroy((OpusEncoder*)nlsEncoder_);
    nlsEncoder_ = NULL;
  } else if (encoder_type_ == ENCODER_OPUS) {
#ifdef ENABLE_OGGOPUS
    ((OggOpusDataEncoder *)nlsEncoder_)->OggopusDestroy();
    encoded_data_.Clear();
    delete (OggOpusDataEncoder *)nlsEncoder_;
    nlsEncoder_ = NULL;
#endif
  }

  encoder_type_ = ENCODER_NONE;

  return Success;
}

}

