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

#include "dataStruct.h"
#include <cstring>

#if !defined(__APPLE__)
#include "openssl/bio.h"
#include "openssl/ssl.h"
#endif

namespace AlibabaNls {
namespace util {

void int2ByteArray(int32_t *data, int len, uint8_t *result, bool isBigEndian) {
    if (!data || len <= 0 || !result) {
        return;
    }

    int nIndex = 0;
    int32_t sh;
    for (int i = 0; i < len; ++i) {
        sh = data[i];
        if (isBigEndian) {
            result[nIndex++] = (uint8_t) (sh >> 24);
            result[nIndex++] = (uint8_t) (sh >> 16);
            result[nIndex++] = (uint8_t) (sh >> 8);
            result[nIndex++] = (uint8_t) (sh);
        } else {
            result[nIndex++] = (uint8_t) (sh);
            result[nIndex++] = (uint8_t) (sh >> 8);
            result[nIndex++] = (uint8_t) (sh >> 16);
            result[nIndex++] = (uint8_t) (sh >> 24);
        }
    }
}

//char *base64Encode(const char *input, int length, bool with_new_line) {
//    BIO *bmem = NULL;
//    BIO *b64 = NULL;
//    BUF_MEM *bptr = NULL;
//
//    b64 = BIO_new(BIO_f_base64());
//    if (!with_new_line) {
//        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
//    }
//
//    bmem = BIO_new(BIO_s_mem());
//    b64 = BIO_push(b64, bmem);
//    BIO_write(b64, input, length);
//    BIO_flush(b64);
//    BIO_get_mem_ptr(b64, &bptr);
//
//    char *buff = (char *) malloc(bptr->length + 1);
//    memcpy(buff, bptr->data, bptr->length);
//    buff[bptr->length] = 0;
//
//    BIO_free_all(b64);
//
//    return buff;
//}

}
}
