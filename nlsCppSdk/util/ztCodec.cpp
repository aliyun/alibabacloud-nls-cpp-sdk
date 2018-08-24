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

#include <cstring>
#include "ztCodec.h"
#include <vector>

using std::vector;

namespace util {

bool ztCodec2::sIsLibAvailable = true;

bool ztCodec2::open(bool isEnc) {
    if (isEnc) {
        enc = createEncoder();
        if (enc) {
            return true;
        }
    } else {
        dec = createDecoder();
        if (dec) {
            return true;
        }
    }
    return false;
}

void ztCodec2::close() {
    if (enc) {
        destroyEncoder(enc);
        enc = NULL;
    }
    if (dec) {
        destoryDecoder(dec);
        dec = NULL;
    }
}

int ztCodec2::bufferFrame(int16_t *in, int offset, int inSize, uint8_t *out) {
    if (isOpen() && out) {
        vector<uint8_t> temp(inSize * 2, 0);
        int size = encode(enc, in, inSize, 0, &temp[0], inSize * 2);
        out[0] = (uint8_t) size;
        memcpy(&out[1], &temp[0], size);
        return size + 1;
    }
    return 0;
}

bool ztCodec2::isOpen() {
    return enc != NULL;
}

const char *ztCodec2::getOpusVersion() {
    return opus_get_version_string();
}

OpusEncoder *ztCodec2::createEncoder() {
    int error;
    OpusEncoder *pOpusEnc = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, &error);
    if (pOpusEnc) {
        opus_encoder_ctl(pOpusEnc, OPUS_SET_VBR(1));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_BITRATE(27800));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_COMPLEXITY(8));
        opus_encoder_ctl(pOpusEnc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    } else {
        sIsLibAvailable = false;
    }
    return pOpusEnc;
}

OpusDecoder *ztCodec2::createDecoder() {
    int error;
    OpusDecoder *pOpusDec = opus_decoder_create(WAVE_FRAM_SIZE, 1, &error);
    if (!pOpusDec) {
        sIsLibAvailable = false;
    }
    return pOpusDec;
}

int ztCodec2::encode(OpusEncoder *pOpusEnc, int16_t *lin, int len, int offset, unsigned char *opus, int opus_len) {
    if (!pOpusEnc || !lin || !opus) {
        return 0;
    }
    if (len - offset < util::WAVE_FRAM_SIZE || opus_len <= 0) {
        return 0;
    }
    int nRet = opus_encode(pOpusEnc, lin + offset, WAVE_FRAM_SIZE, opus, opus_len);
    return nRet;
}

int ztCodec2::decode(OpusDecoder *pOpusDec, unsigned char *opus, int opus_len, int offset, int16_t *lin, int len) {
    //TODO don't need in client
    return 1;
}

void ztCodec2::destroyEncoder(OpusEncoder *pOpusEnc) {
    if (!pOpusEnc)
        return;
    opus_encoder_destroy(pOpusEnc);
}

void ztCodec2::destoryDecoder(OpusDecoder *pOpusDec) {
    if (!pOpusDec)
        return;
    opus_decoder_destroy(pOpusDec);
}

ztCodec2::~ztCodec2() {
    close();
}

ztCodec2::ztCodec2() {
    enc = NULL;
    dec = NULL;
    open(true);
}

}


