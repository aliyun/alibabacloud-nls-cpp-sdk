#include <jni.h>
#include <string>
#include <cstdlib>
#include <vector>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "feature/da/dialogAssistantRequest.h"
#include "log.h"
#include "NlsRequestWarpper.h"
#include "native-lib.h"
using namespace AlibabaNls;
using namespace AlibabaNls::utility;

extern "C"
{

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_DialogAssistant_createDialogAssistantCallback(JNIEnv *env, jobject instance, jobject _callback);

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_DialogAssistant_buildDialogAssistant(JNIEnv *env, jobject instance, jlong _callback);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_start__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_stop__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_cancel__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_queryText__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring text);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring token_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setSessionId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setQueryContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setQueryParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id, jint _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setParams(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setContext(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_sendAudio(JNIEnv *env, jobject instance, jlong id, jbyteArray data_, jint num_byte);

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_DialogAssistant_releaseCallback(JNIEnv *env, jobject instance, jlong id);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_DialogAssistant_createDialogAssistantCallback(JNIEnv *env, jobject instance, jobject _callback) {
    jobject callback = env->NewGlobalRef(_callback);
//    NlsRequestWarpper* wrapper = new NlsRequestWarpper(callback, &NlsRequestWarpper::_global_mtx);
    NlsRequestWarpper* wrapper = new NlsRequestWarpper(callback);
    env->GetJavaVM(&wrapper->_jvm);

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);
    NlsRequestWarpper::_requestMap.insert(std::make_pair(wrapper, true));
    LOG_DEBUG("Set request: %p true, size: %d", wrapper, NlsRequestWarpper::_requestMap.size());
    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

    return (jlong) wrapper;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_DialogAssistant_buildDialogAssistant(JNIEnv *env, jobject instance, jlong wrapper) {
    NlsRequestWarpper* pWrapper = (NlsRequestWarpper*)wrapper;

    DialogAssistantRequest* request = gnlsClient->createDialogAssistantRequest();

    request->setOnTaskFailed(OnTaskFailed, pWrapper);
    request->setOnRecognitionStarted(OnRecognizerStarted, pWrapper);
    request->setOnRecognitionCompleted(OnRecognizerCompleted, pWrapper);
    request->setOnRecognitionResultChanged(OnRecognizedResultChanged, pWrapper);
    request->setOnChannelClosed(OnChannelClosed, pWrapper);
    request->setOnDialogResultGenerated(OnDialogResultGenerated, pWrapper);

    return (jlong) request;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_start__J(JNIEnv *env, jobject instance, jlong id) {
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->start();
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_stop__J(JNIEnv *env, jobject instance, jlong id) {
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    if (request != NULL) {
        int ret = request->stop();
        gnlsClient->releaseDialogAssistantRequest(request);
        return ret;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_cancel__J(JNIEnv *env, jobject instance, jlong id) {
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    if (request != NULL) {
        int ret = request->cancel();
        gnlsClient->releaseDialogAssistantRequest(request);
        return ret;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_queryText__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring text) {
    if (text == NULL) {
        return -1;
    }

    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    const char *_text = env->GetStringUTFChars(text, 0);
    request->setQuery(_text);
    env->ReleaseStringUTFChars(text, _text);
    return request->queryText();
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                           jstring token_) {
    if (token_ == NULL) {
        return -1;
    }

    const char *token = env->GetStringUTFChars(token_, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setToken(token);
    env->ReleaseStringUTFChars(token_, token);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                         jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setUrl(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                            jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setAppKey(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}


JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                            jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setFormat(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id,
                                                              jint _value) {
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    return request->setSampleRate(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setSessionId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setSessionId(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setQueryContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setQueryContext(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setQueryParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setQueryParams(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setParams(JNIEnv *env, jobject instance, jlong id, jstring value_) {
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setPayloadParam(value);
    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_setContext(JNIEnv *env, jobject instance, jlong id, jstring value_) {
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->setContextParam(value);
    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_) {
    if (key_ == NULL || value_ == NULL) {
        return -1;
    }

    const char *key = env->GetStringUTFChars(key_, 0);
    const char *value = env->GetStringUTFChars(value_, 0);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->AppendHttpHeaderParam(key, value);
    env->ReleaseStringUTFChars(value_, value);
    env->ReleaseStringUTFChars(key_, key);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_DialogAssistant_sendAudio(JNIEnv *env, jobject instance, jlong id, jbyteArray data_, jint num_byte)
{
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    DialogAssistantRequest* request = (DialogAssistantRequest*) id;
    int ret = request->sendAudio((uint8_t*)data, num_byte);
    env->ReleaseByteArrayElements(data_, data, 0);
    return ret;
}

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_DialogAssistant_releaseCallback(JNIEnv *env, jobject instance, jlong id) {
    NlsRequestWarpper* wrapper = (NlsRequestWarpper*) id;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(wrapper) != NlsRequestWarpper::_requestMap.end()) {
        NlsRequestWarpper::_requestMap.erase(wrapper);
        LOG_DEBUG("Set request: %p false, size: %d", wrapper, NlsRequestWarpper::_requestMap.size());
    }

    if (wrapper != NULL) {
        LOG_DEBUG("Notify release da callback.");
        delete wrapper;
        wrapper = NULL;
    }
    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

}
