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

#ifndef ALIBABA_NLS_OGGOPUS_AUDIO_IN_H_
#define ALIBABA_NLS_OGGOPUS_AUDIO_IN_H_

#include "opus/opus_types.h"
#include "ogg/ogg.h"
#include "opus/opus_multistream.h"
#include "oggopusDataStruct.h"

namespace AlibabaNls {

#ifdef __cplusplus
extern "C" {
#endif

void SetupPadder(OggEncodeOpt *ogg_encode_opt,
                 ogg_int64_t *original_sample_number);
void ClearPadder(OggEncodeOpt *ogg_encode_opt);

void RawOpen(OggEncodeOpt *ogg_encode_opt);
void WavClose(void *);

#ifdef __cplusplus
}
#endif

}  // namespace AlibabaNls

#endif   // ALIBABA_NLS_OGGOPUS_AUDIO_IN_H_

