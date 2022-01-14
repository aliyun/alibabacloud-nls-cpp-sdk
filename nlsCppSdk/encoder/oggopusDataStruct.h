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

#ifndef ALIBABA_NLS_OGGOPUS_DATASTRUCT_H_
#define ALIBABA_NLS_OGGOPUS_DATASTRUCT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opus/opus_types.h"
#include "opus/opus_multistream.h"
#include "ogg/ogg.h"
#include "oggopusHeader.h"

namespace AlibabaNls {

typedef size_t(*EncodedDataCallback)(const unsigned char *encoded_data,
                                     int len, void *user_data);
typedef int64_t(*ReadFunc)(void *src, float *buffer, int samples,
                           char **buffers, int *lenth);

struct WavInfo {
  uint16_t channels;
  int16_t sample_bits;
  opus_int64 samplesread;
  int16_t bigendian;
  int16_t unsigned8bit;
  int *channel_permute;
  WavInfo() : channels(0), sample_bits(0), samplesread(0), bigendian(0),
  unsigned8bit(0), channel_permute(NULL) {}
};

struct Padder {
  ReadFunc read_func;
  void *read_info;
  ogg_int64_t *original_sample_number;
  int channels;
  int lpc_ptr;
  int *extra_samples;
  float *lpc_out;
  Padder() : read_func(NULL), read_info(NULL), original_sample_number(NULL),
  channels(0), lpc_ptr(0), extra_samples(NULL), lpc_out(NULL) {}
};

struct OggEncodeOpt {
  ReadFunc read_func;
  EncodedDataCallback callback_data_func;
  char *comments;
  void *user_data;
  void *read_info;
  opus_int64 total_samples_per_channel;
  int channels;
  int sample_bits;
  int endianness;
  int ignorelength;
  int skip;
  int extraout;
  int comments_length;
  int copy_comments;
  int copy_pictures;
  OggEncodeOpt() : read_func(NULL), callback_data_func(NULL), comments(NULL),
  user_data(NULL), read_info(NULL), total_samples_per_channel(0), channels(0),
  sample_bits(0), endianness(0), ignorelength(0), skip(0), extraout(0),
  comments_length(0), copy_comments(0), copy_pictures(0) {}
};

struct AudioFunctions {
  void(*open_func)(OggEncodeOpt *opt);
  void(*close_func)(void *);
};

#define readint(buf, base) (((buf[base+3]<<24)&0xff000000)| \
                            ((buf[base+2]<<16)&0xff0000)| \
                            ((buf[base+1]<<8)&0xff00)| \
                            (buf[base]&0xff))

#define writeint(buf, base, val) do { buf[base+3]=((val)>>24)&0xff; \
  buf[base+2]=((val)>>16)&0xff; \
  buf[base+1]=((val)>>8)&0xff; \
  buf[base]=(val)&0xff; \
} while (0)

} // namespace AlibabaNls

#endif  // ALIBABA_NLS_OGGOPUS_DATASTRUCT_H_

