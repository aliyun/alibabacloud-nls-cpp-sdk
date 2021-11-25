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

#ifndef NLS_SDK_UTILITY_H
#define NLS_SDK_UTILITY_H

namespace AlibabaNls {
namespace utility {

#ifdef _WIN32
#define _ssnprintf _snprintf
#else
#define _ssnprintf snprintf
#endif

/*
#if defined(_WIN32)
	#define _ssnprintf _snprintf
	#ifndef snprintf
		#define snprintf _snprintf_s
	#endif
#else
	#define _ssnprintf snprintf
#endif 
*/

#define INPUT_PARAM_STRING_CHECK(x) if (x == NULL) {return -1;};

int getLastErrorCode();

}  // namespace utility
}  // namespace AlibabaNls

#endif //NLS_SDK_UTILITY_H
