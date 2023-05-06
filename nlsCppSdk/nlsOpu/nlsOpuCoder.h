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

#ifndef ALIBABA_NLS_OPU_H
#define ALIBABA_NLS_OPU_H

#include <stdint.h>
#include "opus/opus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_FRAME_NORMAL_SIZE 640
#define DEFAULT_FRAME_INTER_SIZE 320

/*
#if defined(_WIN32)
    #if defined(_NLS_OPU_SHARED_)
        #define NLS_OPU_EXPORT __declspec(dllexport)
    #else
        #define NLS_OPU_EXPORT __declspec(dllimport)
    #endif
#else
    #if defined(_NLS_OPU_SHARED_)
		#define NLS_OPU_EXPORT __attribute__((visibility("default")))
    #else
		#define NLS_OPU_EXPORT
    #endif
#endif
*/

/**
    * @brief 建立opu编码器
    * @param _event	sampleRate 采样率
    * @param errorCode	opus错误代码
    * @return 成功返回编码器指针，失败返回NULL
    */
//NLS_OPU_EXPORT OpusEncoder* createOpuEncoder(const int sampleRate, int *errorCode);

	OpusEncoder* createOpuEncoder(const int sampleRate, int *errorCode);

/**
    * @brief 对数据进行opu编码
    * @param encoder	编码器
    * @param frameBuff	原始PCM音频数据
    * @param frameLen	原始PCM音频数据长度。目前仅支持640字节。
    * @param outputBuffer	装载编码后音频数据的数组
    * @param outputSize	 outputBuffer长度
    * @return 成功返回opu编码后的数据长度，失败返回opus错误代码
    */
/*
NLS_OPU_EXPORT int opuEncoder(OpusEncoder* encoder,
                              const uint8_t* frameBuff,
                              const int frameLen,
                              unsigned char* outputBuffer,
                              int outputSize);
*/

int opuEncoder(OpusEncoder* encoder,
		const uint8_t* frameBuff,
		const int frameLen,
		unsigned char* outputBuffer,
		int outputSize);

/**
    * @brief 释放opu编码器
    * @param encoder 编码器指针
    * @return void
    */
//NLS_OPU_EXPORT void destroyOpuEncoder(OpusEncoder* encoder);
void destroyOpuEncoder(OpusEncoder* encoder);

#ifdef  __cplusplus
}
#endif

#endif //ALIBABA_NLS_OPU_H
