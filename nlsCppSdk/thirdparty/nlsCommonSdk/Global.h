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

#ifndef ALIBABANLS_COMMON_GLOBAL_H
#define ALIBABANLS_COMMON_GLOBAL_H

#if defined(_WIN32)
    #ifdef _MSC_VER
        #pragma warning ( disable : 4251 )
    #endif
    #define NLS_SDK_COMMON_DECL_EXPORT __declspec(dllexport)
    #define NLS_SDK_COMMON_DECL_IMPORT __declspec(dllimport)
#else
    #define NLS_SDK_COMMON_DECL_EXPORT __attribute__((visibility("default")))
    #define NLS_SDK_COMMON_DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(_NLS_COMMON_SDK_CPP_LIBRARY_)
    #define ALIBABANLS_COMMON_EXPORT NLS_SDK_COMMON_DECL_EXPORT
#else
	#define ALIBABANLS_COMMON_EXPORT NLS_SDK_COMMON_DECL_IMPORT
#endif

#endif //ALIBABANLS_COMMON_GLOBAL_H
