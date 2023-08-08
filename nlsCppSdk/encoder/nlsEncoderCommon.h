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

#ifndef ALIBABA_NLS_ENCODER_COMMON_H
#define ALIBABA_NLS_ENCODER_COMMON_H


namespace AlibabaNls {

enum EncoderRet {
  kNlsOk = 0,

  // common error code
  kNlsMallocFailed = 100,
  kNlsFileNotFound = 101,
  kNlsDirNotFound = 102,
  kNlsInvalidParam = 103,
  kNlsUnsupportedAudioType = 104,
  kNlsNullPointer = 105,
  kNlsInvalidState = 106,
  kNlsOpenFileFailed = 107,
  kNlsWriteFileFailed = 108,
  kNlsCheckFileFailed = 109,

  // opus error code
  kNlsCreateOpusEncoderFailed = 501,
  kNlsReleaseOpusEncoderFailed = 502,
  kNlsCreateOpusDecoderFailed = 503,
  kNlsReleaseOpusDecoderFailed = 504,
  kNlsStartOpusFailed = 505,
  kNlsOpusEncodeFailed = 506,
  kNlsOpusDecodeFailed = 507,
  kNlsFinishOpusFailed = 508,
  kNlsStopOpusFailed = 509,

  // Opus related error code.
  kNlsEncodeInitializeFailed = 601,
  kNlsEncodeNotExist = 602,
  kNlsEncodeProcessFailed = 603,
  kNlsEncodeNotInitialized = 604,
};

}

#endif // ALIBABA_NLS_ENCODER_COMMON_H
