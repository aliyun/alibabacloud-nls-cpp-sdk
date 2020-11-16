/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <stdio.h>
#include <openssl/err.h>
#include <openssl/ts.h>

/* BEGIN ERROR CODES */
#ifndef OPENSSL_NO_ERR

# define ERR_FUNC(func) ERR_PACK(ERR_LIB_TS,func,0)
# define ERR_REASON(reason) ERR_PACK(ERR_LIB_TS,0,reason)

static ERR_STRING_DATA TS_str_functs[] = {
    {ERR_FUNC(TS_F_DEF_SERIAL_CB), "def_serial_cb"},
    {ERR_FUNC(TS_F_DEF_TIME_CB), "def_time_cb"},
    {ERR_FUNC(TS_F_ESS_ADD_SIGNING_CERT), "ESS_add_signing_cert"},
    {ERR_FUNC(TS_F_ESS_CERT_ID_NEW_INIT), "ess_CERT_ID_new_init"},
    {ERR_FUNC(TS_F_ESS_SIGNING_CERT_NEW_INIT), "ess_SIGNING_CERT_new_init"},
    {ERR_FUNC(TS_F_INT_TS_RESP_VERIFY_TOKEN), "int_ts_RESP_verify_token"},
    {ERR_FUNC(TS_F_PKCS7_TO_TS_TST_INFO), "PKCS7_to_TS_TST_INFO"},
    {ERR_FUNC(TS_F_TS_ACCURACY_SET_MICROS), "TS_ACCURACY_set_micros"},
    {ERR_FUNC(TS_F_TS_ACCURACY_SET_MILLIS), "TS_ACCURACY_set_millis"},
    {ERR_FUNC(TS_F_TS_ACCURACY_SET_SECONDS), "TS_ACCURACY_set_seconds"},
    {ERR_FUNC(TS_F_TS_CHECK_IMPRINTS), "ts_check_imprints"},
    {ERR_FUNC(TS_F_TS_CHECK_NONCES), "ts_check_nonces"},
    {ERR_FUNC(TS_F_TS_CHECK_POLICY), "ts_check_policy"},
    {ERR_FUNC(TS_F_TS_CHECK_SIGNING_CERTS), "ts_check_signing_certs"},
    {ERR_FUNC(TS_F_TS_CHECK_STATUS_INFO), "ts_check_status_info"},
    {ERR_FUNC(TS_F_TS_COMPUTE_IMPRINT), "ts_compute_imprint"},
    {ERR_FUNC(TS_F_TS_CONF_INVALID), "ts_CONF_invalid"},
    {ERR_FUNC(TS_F_TS_CONF_LOAD_CERT), "TS_CONF_load_cert"},
    {ERR_FUNC(TS_F_TS_CONF_LOAD_CERTS), "TS_CONF_load_certs"},
    {ERR_FUNC(TS_F_TS_CONF_LOAD_KEY), "TS_CONF_load_key"},
    {ERR_FUNC(TS_F_TS_CONF_LOOKUP_FAIL), "ts_CONF_lookup_fail"},
    {ERR_FUNC(TS_F_TS_CONF_SET_DEFAULT_ENGINE), "TS_CONF_set_default_engine"},
    {ERR_FUNC(TS_F_TS_GET_STATUS_TEXT), "ts_get_status_text"},
    {ERR_FUNC(TS_F_TS_MSG_IMPRINT_SET_ALGO), "TS_MSG_IMPRINT_set_algo"},
    {ERR_FUNC(TS_F_TS_REQ_SET_MSG_IMPRINT), "TS_REQ_set_msg_imprint"},
    {ERR_FUNC(TS_F_TS_REQ_SET_NONCE), "TS_REQ_set_nonce"},
    {ERR_FUNC(TS_F_TS_REQ_SET_POLICY_ID), "TS_REQ_set_policy_id"},
    {ERR_FUNC(TS_F_TS_RESP_CREATE_RESPONSE), "TS_RESP_create_response"},
    {ERR_FUNC(TS_F_TS_RESP_CREATE_TST_INFO), "ts_RESP_create_tst_info"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_ADD_FAILURE_INFO),
     "TS_RESP_CTX_add_failure_info"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_ADD_MD), "TS_RESP_CTX_add_md"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_ADD_POLICY), "TS_RESP_CTX_add_policy"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_NEW), "TS_RESP_CTX_new"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_SET_ACCURACY), "TS_RESP_CTX_set_accuracy"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_SET_CERTS), "TS_RESP_CTX_set_certs"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_SET_DEF_POLICY), "TS_RESP_CTX_set_def_policy"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_SET_SIGNER_CERT),
     "TS_RESP_CTX_set_signer_cert"},
    {ERR_FUNC(TS_F_TS_RESP_CTX_SET_STATUS_INFO),
     "TS_RESP_CTX_set_status_info"},
    {ERR_FUNC(TS_F_TS_RESP_GET_POLICY), "ts_RESP_get_policy"},
    {ERR_FUNC(TS_F_TS_RESP_SET_GENTIME_WITH_PRECISION),
     "TS_RESP_set_genTime_with_precision"},
    {ERR_FUNC(TS_F_TS_RESP_SET_STATUS_INFO), "TS_RESP_set_status_info"},
    {ERR_FUNC(TS_F_TS_RESP_SET_TST_INFO), "TS_RESP_set_tst_info"},
    {ERR_FUNC(TS_F_TS_RESP_SIGN), "ts_RESP_sign"},
    {ERR_FUNC(TS_F_TS_RESP_VERIFY_SIGNATURE), "TS_RESP_verify_signature"},
    {ERR_FUNC(TS_F_TS_TST_INFO_SET_ACCURACY), "TS_TST_INFO_set_accuracy"},
    {ERR_FUNC(TS_F_TS_TST_INFO_SET_MSG_IMPRINT),
     "TS_TST_INFO_set_msg_imprint"},
    {ERR_FUNC(TS_F_TS_TST_INFO_SET_NONCE), "TS_TST_INFO_set_nonce"},
    {ERR_FUNC(TS_F_TS_TST_INFO_SET_POLICY_ID), "TS_TST_INFO_set_policy_id"},
    {ERR_FUNC(TS_F_TS_TST_INFO_SET_SERIAL), "TS_TST_INFO_set_serial"},
    {ERR_FUNC(TS_F_TS_TST_INFO_SET_TIME), "TS_TST_INFO_set_time"},
    {ERR_FUNC(TS_F_TS_TST_INFO_SET_TSA), "TS_TST_INFO_set_tsa"},
    {ERR_FUNC(TS_F_TS_VERIFY), "TS_VERIFY"},
    {ERR_FUNC(TS_F_TS_VERIFY_CERT), "ts_verify_cert"},
    {ERR_FUNC(TS_F_TS_VERIFY_CTX_NEW), "TS_VERIFY_CTX_new"},
    {0, NULL}
};

static ERR_STRING_DATA TS_str_reasons[] = {
    {ERR_REASON(TS_R_BAD_PKCS7_TYPE), "bad pkcs7 type"},
    {ERR_REASON(TS_R_BAD_TYPE), "bad type"},
    {ERR_REASON(TS_R_CANNOT_LOAD_CERT), "cannot load certificate"},
    {ERR_REASON(TS_R_CANNOT_LOAD_KEY), "cannot load private key"},
    {ERR_REASON(TS_R_CERTIFICATE_VERIFY_ERROR), "certificate verify error"},
    {ERR_REASON(TS_R_COULD_NOT_SET_ENGINE), "could not set engine"},
    {ERR_REASON(TS_R_COULD_NOT_SET_TIME), "could not set time"},
    {ERR_REASON(TS_R_DETACHED_CONTENT), "detached content"},
    {ERR_REASON(TS_R_ESS_ADD_SIGNING_CERT_ERROR),
     "ess add signing cert error"},
    {ERR_REASON(TS_R_ESS_SIGNING_CERTIFICATE_ERROR),
     "ess signing certificate error"},
    {ERR_REASON(TS_R_INVALID_NULL_POINTER), "invalid null pointer"},
    {ERR_REASON(TS_R_INVALID_SIGNER_CERTIFICATE_PURPOSE),
     "invalid signer certificate purpose"},
    {ERR_REASON(TS_R_MESSAGE_IMPRINT_MISMATCH), "message imprint mismatch"},
    {ERR_REASON(TS_R_NONCE_MISMATCH), "nonce mismatch"},
    {ERR_REASON(TS_R_NONCE_NOT_RETURNED), "nonce not returned"},
    {ERR_REASON(TS_R_NO_CONTENT), "no content"},
    {ERR_REASON(TS_R_NO_TIME_STAMP_TOKEN), "no time stamp token"},
    {ERR_REASON(TS_R_PKCS7_ADD_SIGNATURE_ERROR), "pkcs7 add signature error"},
    {ERR_REASON(TS_R_PKCS7_ADD_SIGNED_ATTR_ERROR),
     "pkcs7 add signed attr error"},
    {ERR_REASON(TS_R_PKCS7_TO_TS_TST_INFO_FAILED),
     "pkcs7 to ts tst info failed"},
    {ERR_REASON(TS_R_POLICY_MISMATCH), "policy mismatch"},
    {ERR_REASON(TS_R_PRIVATE_KEY_DOES_NOT_MATCH_CERTIFICATE),
     "private key does not match certificate"},
    {ERR_REASON(TS_R_RESPONSE_SETUP_ERROR), "response setup error"},
    {ERR_REASON(TS_R_SIGNATURE_FAILURE), "signature failure"},
    {ERR_REASON(TS_R_THERE_MUST_BE_ONE_SIGNER), "there must be one signer"},
    {ERR_REASON(TS_R_TIME_SYSCALL_ERROR), "time syscall error"},
    {ERR_REASON(TS_R_TOKEN_NOT_PRESENT), "token not present"},
    {ERR_REASON(TS_R_TOKEN_PRESENT), "token present"},
    {ERR_REASON(TS_R_TSA_NAME_MISMATCH), "tsa name mismatch"},
    {ERR_REASON(TS_R_TSA_UNTRUSTED), "tsa untrusted"},
    {ERR_REASON(TS_R_TST_INFO_SETUP_ERROR), "tst info setup error"},
    {ERR_REASON(TS_R_TS_DATASIGN), "ts datasign"},
    {ERR_REASON(TS_R_UNACCEPTABLE_POLICY), "unacceptable policy"},
    {ERR_REASON(TS_R_UNSUPPORTED_MD_ALGORITHM), "unsupported md algorithm"},
    {ERR_REASON(TS_R_UNSUPPORTED_VERSION), "unsupported version"},
    {ERR_REASON(TS_R_VAR_BAD_VALUE), "var bad value"},
    {ERR_REASON(TS_R_VAR_LOOKUP_FAILURE), "cannot find config variable"},
    {ERR_REASON(TS_R_WRONG_CONTENT_TYPE), "wrong content type"},
    {0, NULL}
};

#endif

int ERR_load_TS_strings(void)
{
#ifndef OPENSSL_NO_ERR

    if (ERR_func_error_string(TS_str_functs[0].error) == NULL) {
        ERR_load_strings(0, TS_str_functs);
        ERR_load_strings(0, TS_str_reasons);
    }
#endif
    return 1;
}
