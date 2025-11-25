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

#ifndef NLS_SDK_DASH_PARAFORMER_TRANSCRIBER_REQUEST_PARAM_H
#define NLS_SDK_DASH_PARAFORMER_TRANSCRIBER_REQUEST_PARAM_H

#include <string>

#include "iNlsRequestParam.h"

namespace AlibabaNls {

class DashParaformerTranscriberParam : public INlsRequestParam {
 public:
  explicit DashParaformerTranscriberParam(const char* sdkName = "cpp");
  ~DashParaformerTranscriberParam();

  const char* getStartCommand();
  const char* getStopCommand();

  int setDisfluencyRemovalEnabled(bool enable);
  int setLanguageHints(const char* jsonArrayStr);
  int setResources(const char* jsonArrayStr);

 private:
  bool _disfluencyRemovalEnabled;
  std::string _languageHintsJsonArray;
  std::string _resourcesJsonArray;
  std::string _inputJsonObj;
};

}  // namespace AlibabaNls

#endif  // NLS_SDK_DASH_PARAFORMER_TRANSCRIBER_REQUEST_PARAM_H
