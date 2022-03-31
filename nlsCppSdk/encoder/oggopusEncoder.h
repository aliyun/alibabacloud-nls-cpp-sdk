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

#ifndef ALIBABA_OGGOPUS_ENCODER_H_
#define ALIBABA_OGGOPUS_ENCODER_H_

#include "oggopusDataStruct.h"

#define PACKAGE_NAME "opus-tools of Alibaba iDST"
#define PACKAGE_VERSION "1.3.5"

namespace AlibabaNls {

class OggOpusDataEncoderPara {
 public:
  OggOpusDataEncoderPara() :
    opus_multistream_encoder(NULL),
    opus_version(NULL),
    packet(NULL),
    input(NULL),
    audio_functions(NULL),
    last_granulepos(0),
    enc_granulepos(0),
    original_sample_number(0),
    id(0),
    last_segments(-1),
    nbBytes(0),
    nb_samples(0),
    max_frame_bytes(0),
    complexity(0),
    max_ogg_delay(0),
    serialno(0),
    lookahead(0) {
    snprintf(ENCODER_string, sizeof(ENCODER_string), "opusenc from %s %s",
             PACKAGE_NAME, PACKAGE_VERSION);
  }
  ~OggOpusDataEncoderPara() {}

  int InitComment();
  int AddComment();
  int WritePage();

 public:
  OpusMSEncoder *opus_multistream_encoder;
  const char *opus_version;
  unsigned char *packet;
  float *input;
  /* I/O */
  OggEncodeOpt ogg_encode_opt;
  const AudioFunctions *audio_functions;
  ogg_stream_state os;
  ogg_page og;
  ogg_packet op;
  ogg_int64_t last_granulepos;
  ogg_int64_t enc_granulepos;
  ogg_int64_t original_sample_number;
  ogg_int32_t id;
  int last_segments;
  OpusHeader header;
  char ENCODER_string[1024];
  /* Counters */
  opus_int32 nbBytes;
  opus_int32 nb_samples;
  time_t start_time;
  /* Settings */
  int max_frame_bytes;
  opus_int32 bitrate;
  int complexity;
  int max_ogg_delay;  /*48kHz samples*/
  int serialno;
  opus_int32 lookahead;
};


class OggOpusDataEncoder {
 public:
  OggOpusDataEncoder();
  ~OggOpusDataEncoder();

  int OggopusEncoderCreate(EncodedDataCallback encoded_data_callback,
            void *user_data, int samplerate = 16000);
  int OggopusEncode(const char *input_data, int len);
  int OggopusFinish();
  int OggopusSoftRestart();
  int OggopusDestroy();

  void SetSampleRate(int sample_rate) {
    sample_rate_ = sample_rate;
    frame_sample_num_ = sample_rate_ / 50;
    frame_sample_bytes_ = frame_sample_num_ * 2;
  }

  void SetBitrate(int bitrate) {
    encoder_bitrate_ = bitrate;
  }
  int GetBitrate() const { return encoder_bitrate_; }
  
  void SetComplexity(int complexity) {
    encoder_complexity_ = complexity;
  }
  int GetComplexity() const { return encoder_complexity_; }

  void SetFrameSampleBytes(int bytes) {
    if (bytes != frame_sample_bytes_) {
      frame_sample_bytes_ = bytes;
      frame_sample_num_ = bytes / 2;
      if (frame_sample_bytes_ > ogg_opus_para_->max_frame_bytes) {
        ogg_opus_para_->max_frame_bytes = frame_sample_bytes_ * 2;
        if (ogg_opus_para_ && ogg_opus_para_->packet) {
          ogg_opus_para_->packet = reinterpret_cast<unsigned char *>(
              realloc(ogg_opus_para_->packet,
                      sizeof(unsigned char) * ogg_opus_para_->max_frame_bytes));
          if (ogg_opus_para_->packet == NULL) {
            exit(1);
          }
        }
      }

      ogg_opus_para_->input = reinterpret_cast<float *>(
              realloc(ogg_opus_para_->input,
                      sizeof(float) * frame_sample_num_ * channel_num_));
      if (ogg_opus_para_->input == NULL) {
        exit(1);
      }
    }
  }
  int GetFrameSampleBytes() const { return frame_sample_bytes_; }

 private:
  void ResetParameters(EncodedDataCallback encoded_data_callback,
                       void *user_data);

 private:
  OggOpusDataEncoderPara *ogg_opus_para_;
  bool is_first_frame_processed_;
  int sample_rate_;
  int frame_sample_num_;
  int frame_sample_bytes_;
  int channel_num_;
  int encoder_bitrate_;
  int encoder_complexity_;
};

} // namespace AlibabaNls

#endif // ALIBABA_OGGOPUS_ENCODER_H_

