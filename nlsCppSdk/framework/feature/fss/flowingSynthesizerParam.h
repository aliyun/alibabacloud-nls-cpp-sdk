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

#ifndef NLS_SDK_FLOWING_SYNTHESIZER_REQUEST_PARAM_H
#define NLS_SDK_FLOWING_SYNTHESIZER_REQUEST_PARAM_H

#include <string>

#include "iNlsRequestParam.h"

namespace AlibabaNls {

class FlowingSynthesizerParam : public INlsRequestParam {
 public:
  explicit FlowingSynthesizerParam(const char* sdkName);
  ~FlowingSynthesizerParam();

  int setVoice(const char* value);
  int setSingleRoundText(const char* value);
  int setVolume(int value);
  int setSpeechRate(int value);
  int setPitchRate(int value);
  void setEnableSubtitle(bool value);

  const char* getStartCommand();
  const char* getStopCommand();
  const char* getRunFlowingSynthesisCommand(const char* text);
  const char* getFlushFlowingTextCommand(const char* parameters);
  std::string& getSingleRoundText();
  void clearSingleRoundText();

  const int MaximumNumberOfWords;

 private:
  std::string _runFlowingSynthesisCommand;
  std::string _flushFlowingTextCommand;
  std::string _singeRoundText;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_FLOWING_SYNTHESIZER_REQUEST_PARAM_H