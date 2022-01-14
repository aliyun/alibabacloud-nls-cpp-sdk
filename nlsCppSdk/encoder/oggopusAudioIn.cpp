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

#include "oggopusAudioIn.h"

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>

#ifdef WIN32
# include <windows.h> /*GetFileType()*/
# include <io.h>      /*_get_osfhandle()*/
#endif

#include "ogg/ogg.h"
#include "oggopusHeader.h"
#include "lpc.h"

namespace AlibabaNls {

// return: sample number actually read
int ReadBuffer(char *requested_buffer, int requested_len, char **src_buffer,
               int *length) {
  if (*length == 0 || NULL == *src_buffer)
    return 0;

  // input length is smaller than the length requested
  if (*length < requested_len) {
    memcpy(requested_buffer, *src_buffer, *length);
    requested_len = *length;
    *length = 0;
    free(*src_buffer);
    *src_buffer = NULL;
    return requested_len / 2;
  }

  memcpy(requested_buffer, *src_buffer, requested_len);

  // save the left data to tmp buffer
  *length = *length - requested_len;
  if (*length > 0) {
    char *tmp_buffer = (char *)malloc(*length);
    memcpy(tmp_buffer, *src_buffer + requested_len, *length);
    free(*src_buffer);
    *src_buffer = tmp_buffer;
  } else {
    *length = 0;
    free(*src_buffer);
    *src_buffer = NULL;
  }

  return requested_len / 2;
}

int64_t WavRead(void *read_info, float *buffer, int samples,
             char** inbuf, int* length) {
  WavInfo *wav_info = reinterpret_cast<WavInfo *>(read_info);
  int sample_bytes = wav_info->sample_bits / 8;
  int requested_sample_num = samples;
  int requested_len = requested_sample_num * sample_bytes * wav_info->channels;
  signed char *requested_buf = (signed char *)alloca(requested_len);
  int *ch_permute = wav_info->channel_permute;

  requested_sample_num =
    ReadBuffer((char *)requested_buf, requested_len, inbuf, length);
  wav_info->samplesread += requested_sample_num;

  for (int i = 0; i < requested_sample_num; i++) {
    for (int j = 0; j < wav_info->channels; j++) {
      buffer[i*wav_info->channels + j] =
        ((requested_buf[i * 2 * wav_info->channels + 2 * ch_permute[j] + 1] << 8) |
         (requested_buf[i * 2 * wav_info->channels + 2 * ch_permute[j]] & 0xff)) / 32768.0f;
    }
  }
  return requested_sample_num;
}

void WavClose(void *info) {
  WavInfo *wav_info = reinterpret_cast<WavInfo *>(info);
  free(wav_info->channel_permute);
  delete wav_info;
}

void RawOpen(OggEncodeOpt *ogg_encode_opt) {
  WavInfo *wav_info = new WavInfo();

  wav_info->samplesread = 0;
  wav_info->bigendian = ogg_encode_opt->endianness;
  wav_info->unsigned8bit = (ogg_encode_opt->sample_bits == 8);
  wav_info->channels = ogg_encode_opt->channels;
  wav_info->sample_bits = ogg_encode_opt->sample_bits;
  wav_info->channel_permute = (int *)malloc(wav_info->channels * sizeof(int));
  for (int i = 0; i < wav_info->channels; i++)
    wav_info->channel_permute[i] = i;

  ogg_encode_opt->read_func = WavRead;
  ogg_encode_opt->read_info = (void *)wav_info;
  ogg_encode_opt->total_samples_per_channel = 0; /* raw mode, don't bother */
}

/* Read audio data, appending padding to make up any gap between the available
 * and requested number of samples with LPC-predicted data to minimize the
 * pertubation of the valid data that falls in the same frame. */
static int64_t ReadPadder(void *read_info, float *buffer, int samples,
                          char** inbuf, int *length) {
  Padder *padder = reinterpret_cast<Padder *>(read_info);
  // use WavRead Func here
  int64_t in_samples = padder->read_func(padder->read_info, buffer, samples,
                                         inbuf, length);
  if (in_samples <= 0) {
    return in_samples;  // no sample fetched
  }
  int extra = 0;
  const int lpc_order = 32;

  if (padder->original_sample_number)
    *padder->original_sample_number += in_samples;

  if (in_samples < samples) {
    if (padder->lpc_ptr < 0) {
      padder->lpc_out = reinterpret_cast<float *>(
		      calloc(padder->channels * (*padder->extra_samples),
			      sizeof(*padder->lpc_out)));
      if (in_samples > lpc_order * 2) {
        float *lpc = reinterpret_cast<float *>(
			alloca(lpc_order * sizeof(*lpc)));
        for (int i = 0; i < padder->channels; i++) {
          vorbis_lpc_from_data(buffer + i, lpc, in_samples, lpc_order,
                               padder->channels);
          vorbis_lpc_predict(
              lpc, buffer + i + (in_samples - lpc_order) * padder->channels,
              lpc_order, padder->lpc_out + i, *padder->extra_samples,
              padder->channels);
        }
      }
      padder->lpc_ptr = 0;
    }
    extra = samples - in_samples;
    if (extra > *padder->extra_samples)
      extra = *padder->extra_samples;
    *padder->extra_samples -= extra;
  }
  memcpy(buffer + in_samples * padder->channels,
         padder->lpc_out + padder->lpc_ptr * padder->channels,
         extra * padder->channels * sizeof(*buffer));
  padder->lpc_ptr += extra;
  return in_samples + extra;
}

void SetupPadder(OggEncodeOpt *ogg_encode_opt,
                 ogg_int64_t *original_sample_number) {
  Padder *padder = new Padder();

  padder->read_func = ogg_encode_opt->read_func;
  padder->read_info = ogg_encode_opt->read_info;
  padder->channels = ogg_encode_opt->channels;
  padder->extra_samples = &ogg_encode_opt->extraout;
  padder->original_sample_number = original_sample_number;
  padder->lpc_ptr = -1;
  padder->lpc_out = NULL;

  ogg_encode_opt->read_func = ReadPadder;
  ogg_encode_opt->read_info = (Padder *)padder;  // use the padder's read data
}

void ClearPadder(OggEncodeOpt *ogg_encode_opt) {
  Padder *padder = (Padder *)ogg_encode_opt->read_info;
  ogg_encode_opt->read_func = padder->read_func;
  ogg_encode_opt->read_info = padder->read_info;
  if (padder->lpc_out)
    free(padder->lpc_out);
  delete padder;
}

}  // namespace AlibabaNls

