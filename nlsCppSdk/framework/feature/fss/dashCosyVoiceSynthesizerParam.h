/*
 * Copyright 2025 Alibaba Group Holding Limited
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

#ifndef NLS_SDK_DASH_COSYVOICE_SYNTHESIZER_REQUEST_PARAM_H
#define NLS_SDK_DASH_COSYVOICE_SYNTHESIZER_REQUEST_PARAM_H

#include <string>

#include "iNlsRequestParam.h"

namespace AlibabaNls {

class DashCosyVoiceSynthesizerParam : public INlsRequestParam {
 public:
  explicit DashCosyVoiceSynthesizerParam(const char* sdkName);
  ~DashCosyVoiceSynthesizerParam();

  int setSingleRoundText(const char* value);
  int setVolume(int value);
  int setSpeechRate(float rate);
  int setPitchRate(float pitch);
  int setLanguageHints(const char* jsonArrayStr);
  int setInstruction(const char* value);
  int setSeed(int seed);

  const char* getStartCommand();
  const char* getStopCommand();
  const char* getRunFlowingSynthesisCommand(const char* text);
  std::string& getSingleRoundText();
  void clearSingleRoundText();

  const int MaximumNumberOfWords;

 private:
  std::string _runFlowingSynthesisCommand;
  int _volume; /* 音量，取值范围：0～100。默认值：50 */
  float _rate; /* 合成音频的语速，取值范围：0.5~2, 默认值：1.0 */
  float _pitch; /* 合成音频的语调，取值范围：0.5~2, 默认值：1.0 */
  int _seed; /* 生成时使用的随机数种子，使合成的效果产生变化。默认值0。取值范围：0~65535
              */
  std::string _singeRoundText;
  std::string _languageHintsJsonArray;
  std::string _instruction;
  std::string _inputJsonObj;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_DASH_COSYVOICE_SYNTHESIZER_REQUEST_PARAM_H