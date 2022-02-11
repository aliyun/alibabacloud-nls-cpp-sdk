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

#ifndef NLS_SDK_DIALOG_ASSISTANT_REQUEST_PARAM_H
#define NLS_SDK_DIALOG_ASSISTANT_REQUEST_PARAM_H

#include "iNlsRequestParam.h"

namespace AlibabaNls {

class DialogAssistantParam : public INlsRequestParam {

 public:
   DialogAssistantParam(int version, const char* sdkName = "cpp");
   ~DialogAssistantParam();

   virtual const char*  getStartCommand();
   virtual const char*  getStopCommand();
   virtual const char*  getExecuteDialog();
   virtual const char*  getStopWakeWordCommand();

   virtual int setQueryParams(const char* value);
   virtual int setQueryContext(const char* value);
   virtual int setQuery(const char* value);

   int setWakeWordModel(const char* value);
   int setWakeWord(const char* value);

   void setEnableMultiGroup(bool value);
};

}

#endif //NLS_SDK_DIALOG_ASSISTANT_REQUEST_PARAM_H
