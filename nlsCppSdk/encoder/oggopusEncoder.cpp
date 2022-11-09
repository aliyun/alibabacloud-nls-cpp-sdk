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

#include "oggopusEncoder.h"
#include "nlsEncoderCommon.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "oggopusAudioIn.h"
#include "oggopusHeader.h"
#include "nlog.h"
#include "nlsGlobal.h"

namespace AlibabaNls {

int OggOpusDataEncoderPara::InitComment() {
  int opus_version_len = strlen(opus_version);
  int len = 8 + 4 + opus_version_len + 4;
  char *p = reinterpret_cast<char *>(malloc(len));
  if (p == NULL) {
    LOG_ERROR("malloc failed in CommentInit()");
    return -(MallocFailed);
  }
  memset(p, 0, len);
  memcpy(p, "OpusTags", 8);
  writeint(p, 8, opus_version_len);
  memcpy(p + 12, opus_version, opus_version_len);
  writeint(p, 12 + opus_version_len, 0);
  ogg_encode_opt.comments_length = len;
  ogg_encode_opt.comments = p;

  return Success;
}

int OggOpusDataEncoderPara::AddComment() {
  const char *tag = "ENCODER";
  char *p = ogg_encode_opt.comments;
  int vendor_length = readint(p, 8);
  int user_comment_list_length = readint(p, 8 + 4 + vendor_length);
  int tag_len = strlen(tag);
  int val_len = strlen(ENCODER_string);
  int len = ogg_encode_opt.comments_length + 4 + tag_len + val_len;

  p = reinterpret_cast<char *>(realloc(p, len));
  if (p == NULL) {
    LOG_ERROR("realloc failed in CommentAdd()");
    return -(ReallocFailed);
  }
  /* length of comment */
  writeint(p, ogg_encode_opt.comments_length, tag_len+val_len);
  /* comment tag */
  memcpy(p + ogg_encode_opt.comments_length + 4, tag, tag_len);
  (p + ogg_encode_opt.comments_length + 4)[tag_len-1] = '=';  /* separator */
  /* comment */
  memcpy(p+ogg_encode_opt.comments_length+4+tag_len, ENCODER_string, val_len);
  writeint(p, 8 + 4 + vendor_length, user_comment_list_length + 1);
  ogg_encode_opt.comments_length = len;
  ogg_encode_opt.comments = p;

  return Success;
}

/* 通过回调把数据推送到数据队列中 */
int OggOpusDataEncoderPara::WritePage() {
  int written = 0;
  written = ogg_encode_opt.callback_data_func(og.header, og.header_len,
                                              ogg_encode_opt.user_data);
  written += ogg_encode_opt.callback_data_func(og.body, og.body_len,
                                               ogg_encode_opt.user_data);
  return written;
}

static void PrintOggOpusDataEncoderPara(const OggOpusDataEncoderPara *paras) {
  if (paras) {
    LOG_DEBUG("header.nb_streams = %d", paras->header.nb_streams);
    LOG_DEBUG("header.nb_coupled = %d", paras->header.nb_coupled);
    LOG_DEBUG("header.channel_mapping = %d",
         paras->header.channel_mapping);
    LOG_DEBUG("nbBytes = %d", paras->nbBytes);
    LOG_DEBUG("nb_samples = %d", paras->nb_samples);
  }
}

OggOpusDataEncoder::OggOpusDataEncoder() :
  ogg_opus_para_(NULL),
  is_first_frame_processed_(false),
  sample_rate_(16000),
  frame_sample_num_(320),
  frame_sample_bytes_(640),
  encoder_bitrate_(16000),
  channel_num_(1),
  encoder_complexity_(8) {
}

void OggOpusDataEncoder::ResetParameters(
    EncodedDataCallback encoded_data_callback, void *user_data) {
  ogg_opus_para_->ogg_encode_opt.callback_data_func = encoded_data_callback;
  ogg_opus_para_->ogg_encode_opt.user_data = user_data;

  ogg_opus_para_->last_granulepos = 0;
  ogg_opus_para_->enc_granulepos = 0;
  ogg_opus_para_->original_sample_number = 0;
  ogg_opus_para_->id = -1;
  ogg_opus_para_->last_segments = 0;
  ogg_opus_para_->nbBytes = -1;
  ogg_opus_para_->max_frame_bytes = 1280;
  ogg_opus_para_->bitrate = encoder_bitrate_;
  ogg_opus_para_->complexity = encoder_complexity_;
  ogg_opus_para_->max_ogg_delay = 200;
  ogg_opus_para_->lookahead = 0;

  static const AudioFunctions audio_funcs = {RawOpen, WavClose};
  ogg_opus_para_->audio_functions = &audio_funcs;

  ogg_opus_para_->ogg_encode_opt.sample_bits = 16;
  ogg_opus_para_->ogg_encode_opt.endianness = 0;
  ogg_opus_para_->ogg_encode_opt.ignorelength = 0;
  ogg_opus_para_->ogg_encode_opt.copy_comments = 1;
  ogg_opus_para_->ogg_encode_opt.copy_pictures = 1;
  ogg_opus_para_->ogg_encode_opt.channels = 1;
  ogg_opus_para_->ogg_encode_opt.skip = 0;

  ogg_opus_para_->start_time = time(NULL);
  srand(((getpid() & 65535) << 15) ^ ogg_opus_para_->start_time);
  ogg_opus_para_->serialno = 0;

  ogg_opus_para_->opus_version = opus_get_version_string();

  ogg_opus_para_->os.body_data = NULL;
  ogg_opus_para_->os.lacing_vals = NULL;
  ogg_opus_para_->os.granule_vals = NULL;
}

int OggOpusDataEncoder::OggopusEncoderCreate(
    EncodedDataCallback encoded_data_callback,
    void *user_data, int samplerate) {
  sample_rate_ = samplerate;
  if (ogg_opus_para_) {
    delete ogg_opus_para_;
    ogg_opus_para_ = NULL;
  }
  ogg_opus_para_ = new OggOpusDataEncoderPara();
  if (NULL == ogg_opus_para_) {
    return kNlsStartOpusFailed;
  }
  is_first_frame_processed_ = false;

  ResetParameters(encoded_data_callback, user_data); // 初始化ogg_opus_para_

  // OpusTags0007opus_version0000，这个指向ogg_encode_opt.comments
  ogg_opus_para_->InitComment();
  // OpusTags0007opus_version0000ENCODER=opusenc from xxxxx,
  // 继续指向ogg_encode_opt.comments
  ogg_opus_para_->AddComment();

  /* 填充read_func和read_info */
  ogg_opus_para_->audio_functions->open_func(&ogg_opus_para_->ogg_encode_opt);

  SetupPadder(&ogg_opus_para_->ogg_encode_opt,
              &ogg_opus_para_->original_sample_number);

  /* reset opus header */
  ogg_opus_para_->header.version = 0;
  ogg_opus_para_->header.channels = channel_num_;
  ogg_opus_para_->header.nb_streams = 1;
  ogg_opus_para_->header.nb_coupled = 0;
  ogg_opus_para_->header.input_sample_rate = sample_rate_;
  ogg_opus_para_->header.gain = 0;
  ogg_opus_para_->header.channel_mapping = 0;
  memset(ogg_opus_para_->header.stream_map, 0,
         sizeof(ogg_opus_para_->header.stream_map));

  int ret = OPUS_OK;
  ogg_opus_para_->opus_multistream_encoder =
      /* 分配和初始化多流编码器状态 */
      opus_multistream_encoder_create(
          /* 解码的采样率(Hz) */
          sample_rate_,
          /* 输入信号中的通道数 */
          channel_num_,
          /* 输入要编码的数据流的总数，必须不能超过通道数 */
          ogg_opus_para_->header.nb_streams,
          /* 要编码的组对（2通道）数据流的总数，必须不大于数据流的总数 */
          ogg_opus_para_->header.nb_coupled,
          /* 被编码通道与输入通道的映射关系表 */
          ogg_opus_para_->header.stream_map,
          /* 目标编码器应用程序, 偏好与原始输入的正确性 */
          OPUS_APPLICATION_AUDIO,
          &ret);

  if (ret != OPUS_OK) {
    LOG_ERROR("error cannot create encoder: %s", opus_strerror(ret));
    exit(1);
  } else {
    LOG_DEBUG(
        "opus_multistream_encoder_create success. sample_rate_:%d channel_num_:%d",
        sample_rate_, channel_num_);
  }

  ogg_opus_para_->packet = reinterpret_cast<unsigned char *>(
      malloc(sizeof(unsigned char)*ogg_opus_para_->max_frame_bytes));
  if (ogg_opus_para_->packet == NULL) {
    LOG_ERROR("error allocating packet buffer.");
    exit(1);
  }
  memset(ogg_opus_para_->packet, 0,
         sizeof(unsigned char)*ogg_opus_para_->max_frame_bytes);

  LOG_INFO(
       "nb_streams %d, nb_coupled %d, bitrate %d, max frame bytes: %d",
       ogg_opus_para_->header.nb_streams,
       ogg_opus_para_->header.nb_coupled,
       ogg_opus_para_->bitrate,
       ogg_opus_para_->max_frame_bytes);

  ret = opus_multistream_encoder_ctl( /* 向一个Opus多流编码器执行一个CTL函数 */
      ogg_opus_para_->opus_multistream_encoder,
      OPUS_SET_BITRATE(ogg_opus_para_->bitrate));
  if (ret != OPUS_OK) {
    LOG_ERROR("error OPUS_SET_BITRATE returned: %s", opus_strerror(ret));
    exit(1);
  }

  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_multistream_encoder,
      OPUS_SET_VBR(true));
  if (ret != OPUS_OK) {
    LOG_ERROR("error OPUS_SET_VBR returned: %s", opus_strerror(ret));
    exit(1);
  }

  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_multistream_encoder,
      OPUS_SET_VBR_CONSTRAINT(0));
  if (ret != OPUS_OK) {
    LOG_ERROR("error OPUS_SET_VBR_CONSTRAINT returned: %s",
         opus_strerror(ret));
    exit(1);
  }

  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_multistream_encoder,
      OPUS_SET_COMPLEXITY(ogg_opus_para_->complexity));
  if (ret != OPUS_OK) {
    LOG_ERROR("error OPUS_SET_COMPLEXITY returned: %s",
         opus_strerror(ret));
    exit(1);
  }

  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_multistream_encoder,
      OPUS_SET_PACKET_LOSS_PERC(0));
  if (ret != OPUS_OK) {
    LOG_ERROR("error OPUS_SET_PACKET_LOSS_PERC returned: %s",
         opus_strerror(ret));
    exit(1);
  }

#ifdef OPUS_SET_LSB_DEPTH
  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_multistream_encoder,
      OPUS_SET_LSB_DEPTH(ogg_opus_para_->ogg_encode_opt.sample_bits));
  if (ret != OPUS_OK) {
    LOG_ERROR("warning OPUS_SET_LSB_DEPTH returned: %s",
         opus_strerror(ret));
  }
#endif

  /* get lookahead value */
  ret = opus_multistream_encoder_ctl(
      ogg_opus_para_->opus_multistream_encoder,
      OPUS_GET_LOOKAHEAD(&ogg_opus_para_->lookahead));
  if (ret != OPUS_OK) {
    LOG_ERROR("error OPUS_GET_LOOKAHEAD returned: %s", opus_strerror(ret));
    exit(1);
  }
  ogg_opus_para_->ogg_encode_opt.skip += ogg_opus_para_->lookahead;
  ogg_opus_para_->header.preskip = ogg_opus_para_->ogg_encode_opt.skip * 3.0;
  ogg_opus_para_->ogg_encode_opt.extraout = ogg_opus_para_->header.preskip / 3;

  if (ogg_stream_init(&ogg_opus_para_->os, ogg_opus_para_->serialno) == -1) {
    LOG_ERROR("error: stream init failed");
    exit(1);
  }

  ogg_opus_para_->input =
    reinterpret_cast<float *>(malloc(sizeof(float) * frame_sample_num_ *
                                     channel_num_));
  if (ogg_opus_para_->input == NULL) {
    LOG_ERROR("error: couldn't allocate sample buffer.");
    exit(1);
  }
  memset(ogg_opus_para_->input, 0,
         sizeof(float) * frame_sample_num_ * channel_num_);

  // prepare for encoding
  ogg_opus_para_->op.e_o_s = 0;
  ogg_opus_para_->nb_samples = -1;
  is_first_frame_processed_ = false;

  return kNlsOk;
}

int OggOpusDataEncoder::OggopusEncode(const char *input_data, int length) {
  if (NULL == ogg_opus_para_)  {
    return kNlsInvalidState;
  }

  int ret = 0;
  char *tmp_buf = NULL;
  // short *tmp_2 = (short *)input_data;
  int tmp_length = 0;
  if (is_first_frame_processed_ && (length > 0)) {
    // the first frame has been processed
    tmp_length = length;
    tmp_buf = reinterpret_cast<char *>(malloc(tmp_length));
    memset(tmp_buf, 0, tmp_length);
    memcpy(tmp_buf, input_data, tmp_length);
  } else {
    // 第一帧
    if (length == frame_sample_bytes_) {
      unsigned char header_data[276] = {0};
      // 将header结构写入header_data中
      int packet_size = OpusHeaderToPacket(&ogg_opus_para_->header,
                                           header_data, sizeof(header_data));
      ogg_opus_para_->op.packet = header_data; // 填写ogg_packet
      ogg_opus_para_->op.bytes = packet_size;
      ogg_opus_para_->op.b_o_s = 1;
      ogg_opus_para_->op.e_o_s = 0;
      ogg_opus_para_->op.granulepos = 0;
      ogg_opus_para_->op.packetno = 0;
      // 原始数据要封装在ogg_packet中通过ogg_stream_packetin方法
      // 写入到ogg_stream_state
      ogg_stream_packetin(&ogg_opus_para_->os, &ogg_opus_para_->op);

      // os写入og（page的信息），直到写完
      while ((ret = ogg_stream_flush(&ogg_opus_para_->os,
                                     &ogg_opus_para_->og))) {
        if (!ret)
          break;
        ret = ogg_opus_para_->WritePage(); //把og的header和body送出去
        if (ret !=
            ogg_opus_para_->og.header_len + ogg_opus_para_->og.body_len) {
          LOG_ERROR("error: failed writing header to output stream");
          return kNlsOpusEncodeFailed;
        }
      } // while

      // start()函数中已经填充了comments
      ogg_opus_para_->op.packet =
        (unsigned char *)ogg_opus_para_->ogg_encode_opt.comments;
      ogg_opus_para_->op.bytes = ogg_opus_para_->ogg_encode_opt.comments_length;
      ogg_opus_para_->op.b_o_s = 0;
      ogg_opus_para_->op.packetno = 1;
      // 原始数据要封装在ogg_packet中通过ogg_stream_packetin方法
      // 写入到ogg_stream_state
      ogg_stream_packetin(&ogg_opus_para_->os, &ogg_opus_para_->op);

      // os写入og（page的信息），直到写完
      while ((ret = ogg_stream_flush(&ogg_opus_para_->os,
                                     &ogg_opus_para_->og))) {
        if (!ret)
          break;
        ret = ogg_opus_para_->WritePage(); //把og的header和body送出去
        if (ret !=
            ogg_opus_para_->og.header_len + ogg_opus_para_->og.body_len) {
          LOG_ERROR("error: failed writing header to output stream");
          return kNlsOpusEncodeFailed;
        }
      } // while

      tmp_length = length * 2;
      tmp_buf = reinterpret_cast<char *>(malloc(tmp_length));
      memset(tmp_buf, 0, tmp_length);
      memcpy(tmp_buf + length, input_data, length); // 音频数据封入
      is_first_frame_processed_ = true;
    } else {  // input length is invalid
      ogg_opus_para_->op.e_o_s = 1;
    }
  }

  int size_segments, cur_frame_size;
  ogg_opus_para_->id++;

  if (ogg_opus_para_->nb_samples < 0) {
    ogg_opus_para_->nb_samples =
        ogg_opus_para_->ogg_encode_opt.read_func( /* WavRead() */
            ogg_opus_para_->ogg_encode_opt.read_info,
            ogg_opus_para_->input,
            frame_sample_num_,
            /* 将tmp_buf（原始数据）Wav写入到 ogg_opus_para中 */
            &tmp_buf, &tmp_length);
  }

  if (ogg_opus_para_->start_time == 0) {
    ogg_opus_para_->start_time = time(NULL);
  }

  cur_frame_size = frame_sample_num_;

  if (ogg_opus_para_->nb_samples < cur_frame_size) {
    /*Avoid making the final packet 20ms or more longer than needed.*/
    cur_frame_size -= ((cur_frame_size -
                        (ogg_opus_para_->nb_samples >= 0 ?
                         ogg_opus_para_->nb_samples : 1)) / 320) * 320;
    /*No fancy end padding, just fill with zeros for now.*/
    for (int i = ogg_opus_para_->nb_samples * channel_num_;
         i < cur_frame_size * channel_num_; i++) {
      ogg_opus_para_->input[i] = 0;
    }
  }

  if (cur_frame_size <= 0) {
    LOG_WARN("cur_frame_size = %d, nb_samples = %d", cur_frame_size,
         ogg_opus_para_->nb_samples);
    if (tmp_buf) {
      free(tmp_buf);
      tmp_buf = NULL;
    }
    return kNlsOpusEncodeFailed;
  }

  /* 成功，是被编码包的长度（字节数），失败，一个负的错误代码 */
  ogg_opus_para_->nbBytes =
    /* 根据浮点输入对一个 Opus帧进行编码. */
    opus_multistream_encode_float(ogg_opus_para_->opus_multistream_encoder,
                                  ogg_opus_para_->input,
                                  cur_frame_size, ogg_opus_para_->packet,
                                  ogg_opus_para_->max_frame_bytes);
  if (ogg_opus_para_->nbBytes < 0) {
    LOG_ERROR("encoding failed: %d %s. aborting ...",
         ogg_opus_para_->nbBytes, opus_strerror(ogg_opus_para_->nbBytes));
    LOG_INFO("cur_frame_size = %d", cur_frame_size);
    LOG_INFO("ogg_opus_para_->nbBytes = %d", ogg_opus_para_->nbBytes);
    if (tmp_buf) {
      free(tmp_buf);
      tmp_buf = NULL;
    }
    return kNlsOpusEncodeFailed;
  }
  ogg_opus_para_->enc_granulepos += 320 * 3;
  size_segments = (ogg_opus_para_->nbBytes + 255) / 255;

  /*Flush early if adding this packet would make us end up with a
    continued page which we wouldn't have otherwise.*/
  while ((((size_segments <= 255) &&
           (ogg_opus_para_->last_segments + size_segments > 255))
          || (ogg_opus_para_->enc_granulepos -
              ogg_opus_para_->last_granulepos > ogg_opus_para_->max_ogg_delay))
         && ogg_stream_flush_fill(&ogg_opus_para_->os, &ogg_opus_para_->og,
                                  255 * 255)
        ) {
    if (ogg_page_packets(&ogg_opus_para_->og) != 0) {
      ogg_opus_para_->last_granulepos =
        ogg_page_granulepos(&ogg_opus_para_->og);
    }
    ogg_opus_para_->last_segments -= ogg_opus_para_->og.header[26];
    ret = ogg_opus_para_->WritePage();
    if (ret != ogg_opus_para_->og.header_len + ogg_opus_para_->og.body_len) {
      LOG_ERROR("error: failed writing data to output stream");
      if (tmp_buf) {
        free(tmp_buf);
        tmp_buf = NULL;
      }
      return kNlsOpusEncodeFailed;
    }
  }

  if ((!ogg_opus_para_->op.e_o_s) && ogg_opus_para_->max_ogg_delay > 5760) {
    ogg_opus_para_->nb_samples =
        ogg_opus_para_->ogg_encode_opt.read_func( /* WavRead() */
            ogg_opus_para_->ogg_encode_opt.read_info, ogg_opus_para_->input,
            frame_sample_num_,
            /* 将tmp_buf（原始数据）Wav写入到 ogg_opus_para中 */
            &tmp_buf, &tmp_length);

    if (ogg_opus_para_->nb_samples == 0) {
      LOG_INFO("nb_samples = %d, max_ogg_delay = %d",
           ogg_opus_para_->nb_samples, ogg_opus_para_->max_ogg_delay);
      ogg_opus_para_->op.e_o_s = 1;
    }
  } else {
    ogg_opus_para_->nb_samples = -1;
  }

  ogg_opus_para_->op.packet = (unsigned char *)ogg_opus_para_->packet;
  ogg_opus_para_->op.bytes = ogg_opus_para_->nbBytes;
  ogg_opus_para_->op.b_o_s = 0;
  ogg_opus_para_->op.granulepos = ogg_opus_para_->enc_granulepos;
  if (ogg_opus_para_->op.e_o_s) {
    ogg_opus_para_->op.granulepos =
      ((ogg_opus_para_->original_sample_number * 48000 + sample_rate_ - 1) /
       sample_rate_) + ogg_opus_para_->header.preskip;
  }
  ogg_opus_para_->op.packetno = 2 + ogg_opus_para_->id;
  ogg_stream_packetin(&ogg_opus_para_->os, &ogg_opus_para_->op);
  ogg_opus_para_->last_segments += size_segments;

  /*If the stream is over or we're sure that the delayed flush will fire,
    go ahead and flush now to avoid adding delay.*/
  while ((ogg_opus_para_->op.e_o_s ||
          (ogg_opus_para_->enc_granulepos + (320 * 3) -
           ogg_opus_para_->last_granulepos > ogg_opus_para_->max_ogg_delay) ||
          (ogg_opus_para_->last_segments >= 255)) ?
         ogg_stream_flush_fill(&ogg_opus_para_->os, &ogg_opus_para_->og,
                               255 * 255) :
         ogg_stream_pageout_fill(&ogg_opus_para_->os, &ogg_opus_para_->og,
                                 255 * 255)
        ) {
    if (ogg_page_packets(&ogg_opus_para_->og) != 0) {
      ogg_opus_para_->last_granulepos =
        ogg_page_granulepos(&ogg_opus_para_->og);
    }
    ogg_opus_para_->last_segments -= ogg_opus_para_->og.header[26];
    ret = ogg_opus_para_->WritePage();
    if (ret != ogg_opus_para_->og.header_len + ogg_opus_para_->og.body_len) {
      LOG_ERROR("error: failed writing data to output stream");
      if (tmp_buf) {
        free(tmp_buf);
        tmp_buf = NULL;
      }
      return kNlsOpusEncodeFailed;
    }
  }
  if (tmp_buf) {
    free(tmp_buf);
    tmp_buf = NULL;
  }

  return kNlsOk;
}

int OggOpusDataEncoder::OggopusFinish() {
  if (NULL == ogg_opus_para_) {
    return kNlsFinishOpusFailed;
  }

  OggopusEncode(NULL, 0);
  OggopusEncode(NULL, 0);
  return kNlsOk;
}

int OggOpusDataEncoder::OggopusSoftRestart() {
  is_first_frame_processed_ = false;
  return kNlsOk;
}

int OggOpusDataEncoder::OggopusDestroy() {
  if (NULL == ogg_opus_para_) {
    return kNlsStopOpusFailed;
  }

  is_first_frame_processed_ = false;
  free(ogg_opus_para_->ogg_encode_opt.comments);
  opus_multistream_encoder_destroy(ogg_opus_para_->opus_multistream_encoder);
  ogg_stream_clear(&ogg_opus_para_->os);
  free(ogg_opus_para_->packet);
  ogg_opus_para_->packet = NULL;
  free(ogg_opus_para_->input);
  ogg_opus_para_->input = NULL;

  ClearPadder(&ogg_opus_para_->ogg_encode_opt);
  /* 释放wav_info */
  ogg_opus_para_->audio_functions->close_func(
      ogg_opus_para_->ogg_encode_opt.read_info);
  if (ogg_opus_para_) {
    delete ogg_opus_para_;
    ogg_opus_para_ = NULL;
  }

  return kNlsOk;
}

OggOpusDataEncoder::~OggOpusDataEncoder() {}

}  // namespace AlibabaNls

