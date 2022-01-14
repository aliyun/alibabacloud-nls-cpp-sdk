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

#include <string.h>
#include <stdio.h>
#include "oggopusHeader.h"

/* Header contents:
   - "OpusHead" (64 bits)
   - version number (8 bits)
   - Channels C (8 bits)
   - Pre-skip (16 bits)
   - Sampling rate (32 bits)
   - Gain in dB (16 bits, S7.8)
   - Mapping (8 bits, 0=single stream (mono/stereo) 1=Vorbis mapping,
   2..254: reserved, 255: multistream with no mapping)

   - if (mapping != 0)
   - N = total number of streams (8 bits)
   - M = number of paired streams (8 bits)
   - C times channel origin
   - if (C<2*M)
   - stream = byte/2
   - if (byte&0x1 == 0)
   - left
   else
   - right
   - else
   - stream = byte-M
   */

struct Packet {
  unsigned char *data;
  int maxlen;
  int pos;
  Packet() : data(NULL), maxlen(0), pos(0) {}
};

struct ROPacket {
  const unsigned char *data;
  int maxlen;
  int pos;
  ROPacket() : data(NULL), maxlen(0), pos(0) {}
};

static int write_uint32(Packet *p, ogg_uint32_t val) {
  if (p->pos > p->maxlen-4)
    return 0;
  p->data[p->pos  ] = (val) & 0xFF;
  p->data[p->pos+1] = (val>> 8) & 0xFF;
  p->data[p->pos+2] = (val>>16) & 0xFF;
  p->data[p->pos+3] = (val>>24) & 0xFF;
  p->pos += 4;
  return 1;
}

static int write_uint16(Packet *p, ogg_uint16_t val) {
  if (p->pos > p->maxlen - 2)
    return 0;
  p->data[p->pos  ] = (val) & 0xFF;
  p->data[p->pos+1] = (val>> 8) & 0xFF;
  p->pos += 2;
  return 1;
}

static int write_chars(Packet *p, const unsigned char *str, int nb_chars) {
  int i;
  if (p->pos > p->maxlen - nb_chars)
    return 0;
  for (i = 0; i < nb_chars; i++)
    p->data[p->pos++] = str[i];
  return 1;
}

static int read_uint32(ROPacket *p, ogg_uint32_t *val) {
  if (p->pos > p->maxlen - 4)
    return 0;
  *val =  (ogg_uint32_t)p->data[p->pos  ];
  *val |= (ogg_uint32_t)p->data[p->pos+1]<< 8;
  *val |= (ogg_uint32_t)p->data[p->pos+2]<<16;
  *val |= (ogg_uint32_t)p->data[p->pos+3]<<24;
  p->pos += 4;
  return 1;
}

static int read_uint16(ROPacket *p, ogg_uint16_t *val) {
  if (p->pos > p->maxlen-2)
    return 0;
  *val =  (ogg_uint16_t)p->data[p->pos  ];
  *val |= (ogg_uint16_t)p->data[p->pos+1]<<8;
  p->pos += 2;
  return 1;
}

static int read_chars(ROPacket *p, unsigned char *str, int nb_chars) {
  int i;
  if (p->pos > p->maxlen-nb_chars)
    return 0;
  for (i = 0; i < nb_chars; i++)
    str[i] = p->data[p->pos++];
  return 1;
}

int OpusHeaderToPacket(const OpusHeader *h, unsigned char *packet, int len) {
  Packet p;
  unsigned char ch;

  p.data = packet;
  p.maxlen = len;
  p.pos = 0;
  if (len < 19)
    return 0;
  if (!write_chars(&p, (const unsigned char*)"OpusHead", 8))
    return 0;
  /* Version is 1 */
  ch = 1;
  if (!write_chars(&p, &ch, 1))
    return 0;

  ch = h->channels;
  if (!write_chars(&p, &ch, 1))
    return 0;

  if (!write_uint16(&p, h->preskip))
    return 0;

  if (!write_uint32(&p, h->input_sample_rate))
    return 0;

  if (!write_uint16(&p, h->gain))
    return 0;

  ch = h->channel_mapping;
  if (!write_chars(&p, &ch, 1))
    return 0;

  if (h->channel_mapping != 0) {
    ch = h->nb_streams;
    if (!write_chars(&p, &ch, 1))
      return 0;

    ch = h->nb_coupled;
    if (!write_chars(&p, &ch, 1))
      return 0;

    int i;
    /* Multi-stream support */
    for (i=0; i < h->channels; i++) {
      if (!write_chars(&p, &h->stream_map[i], 1))
        return 0;
    }
  }

  return p.pos;
}

/* This is just here because it's a convenient file linked by both opusenc and
   opusdec (to guarantee this maps stays in sync). */
const int wav_permute_matrix[8][8] = {
  {0},              /* 1.0 mono   */
  {0, 1},            /* 2.0 stereo */
  {0, 2, 1},          /* 3.0 channel ('wide') stereo */
  {0, 1, 2, 3},        /* 4.0 discrete quadraphonic */
  {0, 2, 1, 3, 4},      /* 5.0 surround */
  {0, 2, 1, 4, 5, 3},    /* 5.1 surround */
  {0, 2, 1, 5, 6, 4, 3},  /* 6.1 surround */
  {0, 2, 1, 6, 7, 4, 5, 3} /* 7.1 surround (classic theater 8-track) */
};

