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
#include "opus/opus_defines.h"
#include "nlsOpuCoder.h"

#define DEFAULT_CHANNELS 1

OpusEncoder* createOpuEncoder(const int sampleRate, int *errorCode) {

    int tmpCode = 0;

    OpusEncoder *pOpusEncoder = opus_encoder_create(sampleRate,
                                                    DEFAULT_CHANNELS,
                                                    OPUS_APPLICATION_VOIP,
                                                    &tmpCode);
    if (pOpusEncoder) {
        opus_encoder_ctl(pOpusEncoder, OPUS_SET_VBR(1)); //动态码率
        opus_encoder_ctl(pOpusEncoder, OPUS_SET_BITRATE(27800)); //指定opus编码码率
        opus_encoder_ctl(pOpusEncoder, OPUS_SET_COMPLEXITY(8)); //计算复杂度
        opus_encoder_ctl(pOpusEncoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE)); //设置针对语音优化
    } else {
        pOpusEncoder = NULL;
    }

    *errorCode = tmpCode;

    return pOpusEncoder;
}

//OpusDecoder* createOpuDecoder(const int sampleRate, int *errorCode) {
//    int tmpCode = 0;
//    OpusDecoder *pOpusDecoder = opus_decoder_create(sampleRate,
//                                                    DEFAULT_CHANNELS,
//                                                    &tmpCode);
//    if (!pOpusDecoder) {
//        pOpusDecoder = NULL;
//    }
//
//    *errorCode = tmpCode;
//
//    return pOpusDecoder;
//}

int opuEncoder(OpusEncoder* encoder,
               const uint8_t* frameBuff,
               const int frameLen,
               unsigned char* outputBuffer,
               int outputSize) {

    int16_t interBuffer[DEFAULT_FRAME_INTER_SIZE] = {0};
//    int interLen = DEFAULT_FRAME_INTER_SIZE;

    if (!encoder || !frameBuff || !outputBuffer || (frameLen != DEFAULT_FRAME_NORMAL_SIZE) || (outputSize <= 0)) {
        return 0;
    }

//    if (!outputBuffer || outputLen < DEFAULT_FRAME_INTER_SIZE) {
//        return 0;
//    }

    //uint8 to int16
    int i = 0;
    for (i = 0; i < DEFAULT_FRAME_NORMAL_SIZE; i += 2) {
        interBuffer[i / 2] = (int16_t) ((frameBuff[i + 1] << 8 & 0xff00) | (frameBuff[i] & 0xff));
    }

	unsigned char* tmp = NULL;
	tmp = (unsigned char*)malloc(outputSize);
	if (!tmp) {
		return 0;
	}
	memset(tmp, 0x0, outputSize);

    int encoderSize = opus_encode(encoder,
                              interBuffer,
                              DEFAULT_FRAME_INTER_SIZE,
                              tmp,
                              outputSize);
    if (encoderSize < 0) {
        return encoderSize;
    }

    *(outputBuffer + 0) = (unsigned char) encoderSize;
	memcpy((outputBuffer + 1), tmp, encoderSize);
	free(tmp);
	tmp = NULL;

    encoderSize += 1;

    return encoderSize;
}

//int opuDecoder(OpusDecoder* decoder,
//               const unsigned char *frameBuff,
//               const int frameLen ,
//               int16_t *outputBuffer,
//               int outputSize) {
//
//    if (!decoder || !frameBuff || (frameLen <= 0) || !outputBuffer|| (outputSize <= 0)) {
//        return 0;
//    }
//
////    if (!outputBuffer || outputLen < DEFAULT_FRAME_NORMAL_SIZE) {
////        return 0;
////    }
//
//    int decodeSize = opus_decode(decoder,
//                              frameBuff,
//                              frameLen,
//                              outputBuffer,
//                              outputSize,
//                              0);
//    if (decodeSize < 0) {
//        return decodeSize;
//    }
//
//    return decodeSize;
//}

void destroyOpuEncoder(OpusEncoder* opuEncoder) {
    if (!opuEncoder) {
        return ;
    }
    opus_encoder_destroy(opuEncoder);
}

//void destroyOpuDecoder(OpusDecoder* opuDecoder) {
//    if (!opuDecoder) {
//        return ;
//    }
//    opus_decoder_destroy(opuDecoder);
//}

