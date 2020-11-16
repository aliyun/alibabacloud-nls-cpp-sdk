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

#include <stdint.h>
#include <stdio.h>
#include "com_alibaba_nls_client_util_OpuCodec.h"
#include "nlsOpuCoder.h"

JNIEXPORT jlong JNICALL Java_com_alibaba_nls_client_util_OpuCodec_createOpuEncoder(JNIEnv* env,
                                                                           jobject thiz,
                                                                           jint sampleRate) {
	int errorCode = 0;
    OpusEncoder* pOpusEncoder = createOpuEncoder(sampleRate, &errorCode);

    if (!pOpusEncoder) {
        printf("createOpuEncoder failed: %d.\n", errorCode);
    }

    return (jlong)pOpusEncoder;
}

JNIEXPORT jint JNICALL Java_com_alibaba_nls_client_util_OpuCodec_encode(JNIEnv* env,
                                                                    jobject thiz,
                                                                    jlong encoder,
                                                                    jbyteArray frameBuffer,
                                                                    jint frameLen,
                                                                    jbyteArray outputBuffer,
                                                                    jint outputLen) {

    OpusEncoder *pEncoder = (OpusEncoder*) encoder;
    if (!pEncoder || frameLen != DEFAULT_FRAME_NORMAL_SIZE) {
        return 0;
    }

    jbyte *pFrameBuffer = (*env)->GetByteArrayElements(env, frameBuffer, 0);
    jbyte *pOutputBuffer = (*env)->GetByteArrayElements(env, outputBuffer, 0);

    int encodeSzie = opuEncoder(pEncoder,
                                (uint8_t *)pFrameBuffer,
                                frameLen,
                                (unsigned char*)pOutputBuffer,
                                outputLen);

    (*env)->ReleaseByteArrayElements(env, frameBuffer, pFrameBuffer, 0);
    (*env)->ReleaseByteArrayElements(env, outputBuffer, pOutputBuffer, 0);

    return encodeSzie;
}


JNIEXPORT void JNICALL Java_com_alibaba_nls_client_util_OpuCodec_destroyOpuEncoder(JNIEnv* env,
                                                                           jobject thiz,
                                                                           jlong encoder){

printf("Begin destroy encode.\n");

    OpusEncoder *pEncoder = (OpusEncoder*) encoder;
    if (!pEncoder) {
        return;
    }

    destroyOpuEncoder(pEncoder);

printf("End destroy encode.\n");
}
