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

#ifndef ALIBABA_OGGOPUS_HEADER_H_
#define ALIBABA_OGGOPUS_HEADER_H_

#include <cstdio>
#include <cstring>
#include "ogg/ogg.h"

struct OpusHeader {
  int version;
  int channels; /* Number of channels: 1..255 */
  int preskip;
  ogg_uint32_t input_sample_rate;
  int gain; /* in dB S7.8 should be zero whenever possible */
  int channel_mapping;
  /* The rest is only used if channel_mapping != 0 */
  int nb_streams;
  int nb_coupled;
  unsigned char stream_map[255];
  OpusHeader() : version(0), channels(0), preskip(0), input_sample_rate(0),
  gain(0), channel_mapping(0), nb_streams(0), nb_coupled(0) {
    memset(stream_map, 0, sizeof(stream_map));
  }
};

int OpusHeaderToPacket(const OpusHeader *h, unsigned char *packet, int len);

extern const int wav_permute_matrix[8][8];

#endif  // ALIBABA_OGGOPUS_HEADER_H_

