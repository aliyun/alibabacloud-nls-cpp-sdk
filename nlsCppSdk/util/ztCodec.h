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

#ifndef ALITVASR_SDK_ZTCODEC2_H
#define ALITVASR_SDK_ZTCODEC2_H

#include "thirdparty/libopus/opus.h"
#include "thirdparty/libopus/opus_defines.h"
#include <stdint.h>

namespace util {

const static int WAVE_FRAM_SIZE = 320;

class ztCodec2 {
private:
	static bool sIsLibAvailable;

	OpusEncoder *createEncoder();

	int encode(OpusEncoder *pOpusEnc, int16_t *lin, int len, int offset, unsigned char *opus, int opus_len);

	void destroyEncoder(OpusEncoder *pOpusEnc);

	OpusDecoder *createDecoder();

	int decode(OpusDecoder *pOpusDec, unsigned char *opus, int opus_len, int offset, int16_t *lin, int len);

	void destoryDecoder(OpusDecoder *pOpusDec);

	OpusEncoder *enc;
	OpusDecoder *dec;

public:
	ztCodec2();

	static bool isAvailable() { return sIsLibAvailable; };

	static const char *getOpusVersion();

	bool open(bool isEncoder);

	void close();

	bool isOpen();

	int bufferFrame(int16_t *in, int offset, int inSize, uint8_t *out);

	~ztCodec2();
};

}

#endif //ALITVASR_SDK_ZTCODEC2_H
