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

#ifndef NLS_SDK_GLOBAL_H
#define NLS_SDK_GLOBAL_H

#if defined(_WIN32)

//    #ifdef _MSC_VER
//        #pragma warning (disable : 4251)
//    #endif

    #define NLS_SDK_DECL_EXPORT __declspec(dllexport)
    #define NLS_SDK_DECL_IMPORT __declspec(dllimport)

    #if defined(_NLS_SDK_SHARED_)
        #define NLS_SDK_CLIENT_EXPORT NLS_SDK_DECL_EXPORT
    #else
        #define NLS_SDK_CLIENT_EXPORT NLS_SDK_DECL_IMPORT
    #endif

#else

    #if defined(_NLS_SDK_SHARED_)
        #define NLS_SDK_CLIENT_EXPORT __attribute__((visibility("default")))
    #else
        #define NLS_SDK_CLIENT_EXPORT
    #endif

#endif

#endif //NLS_SDK_GLOBAL_H
