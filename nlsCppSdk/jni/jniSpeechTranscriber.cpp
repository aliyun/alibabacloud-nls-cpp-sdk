#include <jni.h>
#include <string>
#include <cstdlib>
#include <vector>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "st/speechTranscriberRequest.h"
#include "log.h"
#include "NlsRequestWarpper.h"
#include "native-lib.h"
using namespace AlibabaNls;
using namespace utility;

extern "C"
{

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_createTranscriberCallback(JNIEnv *env, jobject instance, jobject _callback);

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_buildTranscriberRequest(JNIEnv *env, jobject instance, jlong callback);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_start__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_stop__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_cancel__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring token_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setIntermediateResult(JNIEnv *env, jobject instance, jlong id, jboolean value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setInverseTextNormalization(JNIEnv *env, jobject instance, jlong id, jboolean value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setPunctuationPrediction(JNIEnv *env, jobject instance, jlong id, jboolean value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id, jint _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setMaxSentenceSilence__JI(JNIEnv *env, jobject instance, jlong id, jint _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setSemanticSentenceDetection(JNIEnv *env, jobject instance, jlong id, jboolean _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setCustomizationId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring customizationId_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setVocabularyId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring vocabularyId_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_sendAudio(JNIEnv *env, jobject instance, jlong id, jbyteArray data_, jint num_byte);

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_releaseCallback(JNIEnv *env, jobject instance, jlong id);

}

void OnTranscriptionStarted(NlsEvent* str, void* para = NULL) {
    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        LOG_DEBUG("ST Started Callback is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if(env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onTranscriptionStarted","(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef(msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}

void OnTranscriptionCompleted(NlsEvent* str, void* para = NULL) {
    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if(tst == NULL || tst->_callback == NULL) {
        LOG_DEBUG("ST Completed Callback is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if(env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onTranscriptionCompleted","(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef( msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

}

void OnTranscriptionResultChanged(NlsEvent* str, void* para = NULL) {
    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if(tst == NULL || tst->_callback == NULL) {
        LOG_DEBUG("ST ResultChanged Callback is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if(env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onTranscriptionResultChanged","(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef( msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);

}

void OnSentenceBegin(NlsEvent* str, void* para = NULL) {
    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if (tst == NULL || tst->_callback == NULL) {
        LOG_DEBUG("ST SentenceBegin Callback is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if(env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onSentenceBegin","(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef(msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}

void OnSentenceEnd(NlsEvent* str, void* para = NULL) {
    NlsRequestWarpper* tst = (NlsRequestWarpper *)para;
    JNIEnv* env = NULL;
    bool attached = false;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(tst) == NlsRequestWarpper::_requestMap.end()) {
        LOG_DEBUG("NlsRequestWarpper::_requestMap request:%p is earse.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    } else {
        LOG_DEBUG("NlsRequestWarpper::_requestMap find request: %p .", tst);
    }

    if(tst == NULL || tst->_callback == NULL) {
        LOG_DEBUG("ST SentenceEnd Callback is free.");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    switch (tst->_jvm->GetEnv((void**)&env, JNI_VERSION_1_6)) {
        case JNI_OK:
            break;
        case JNI_EDETACHED:
            if (tst->_jvm->AttachCurrentThread(&env, NULL)!=0) {
                LOG_DEBUG("attach fail");
                pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
                return;
            }
            attached = true;
            break;
        case JNI_EVERSION:
            LOG_DEBUG("Invalid java version");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
        case JNI_ERR:
            LOG_ERROR("GetEnv error");
            pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
            return;
    }

    if(env == NULL) {
        LOG_ERROR("Env NULL");
        pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
        return;
    }

    jclass native_clazz = env->GetObjectClass(tst->_callback);
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onSentenceEnd","(Ljava/lang/String;I)V");
    jstring msg = env->NewStringUTF(str->getAllResponse());
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef( msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_createTranscriberCallback(JNIEnv *env, jobject instance, jobject _callback) {
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
Java_com_alibaba_idst_util_SpeechTranscriber_buildTranscriberRequest(JNIEnv *env, jobject instance, jlong wrapper) {
    NlsRequestWarpper* pWrapper = (NlsRequestWarpper*)wrapper;
    SpeechTranscriberRequest* request = gnlsClient->createTranscriberRequest();

    request->setOnTaskFailed(OnTaskFailed, pWrapper);
    request->setOnTranscriptionStarted(OnTranscriptionStarted, pWrapper);
    request->setOnTranscriptionCompleted(OnTranscriptionCompleted, pWrapper);
    request->setOnTranscriptionResultChanged(OnTranscriptionResultChanged, pWrapper);
    request->setOnChannelClosed(OnChannelClosed, pWrapper);
    request->setOnSentenceBegin(OnSentenceBegin, pWrapper);
    request->setOnSentenceEnd(OnSentenceEnd, pWrapper);

    return (jlong) request;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_start__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->start();
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_stop__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    if (request != NULL) {
        int ret = request->stop();
        gnlsClient->releaseTranscriberRequest(request);
        return ret;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_cancel__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    if (request != NULL) {
        int ret = request->cancel();
        gnlsClient->releaseTranscriberRequest(request);
        return ret;
    }
    return 0;
}


JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                           jstring token_) {
    if (token_ == NULL) {
        return -1;
    }

    const char *token = env->GetStringUTFChars(token_, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setToken(token);
    env->ReleaseStringUTFChars(token_, token);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setCustomizationId__JLjava_lang_String_2(JNIEnv *env,
                                                                                      jobject instance,
                                                                                      jlong id,
                                                                                      jstring customizationId_) {
    if (customizationId_ == NULL) {
        return -1;
    }

    const char *customizationId = env->GetStringUTFChars(customizationId_, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setCustomizationId(customizationId);
    env->ReleaseStringUTFChars(customizationId_, customizationId);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setVocabularyId__JLjava_lang_String_2(JNIEnv *env,
                                                                                   jobject instance,
                                                                                   jlong id,
                                                                                   jstring vocabularyId_) {
    if (vocabularyId_ == NULL) {
        return -1;
    }

    const char *vocabularyId = env->GetStringUTFChars(vocabularyId_, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setVocabularyId(vocabularyId);
    env->ReleaseStringUTFChars(vocabularyId_, vocabularyId);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                         jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setUrl(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                            jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setAppKey(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}


JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                            jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setFormat(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id,
                                                               jint _value) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    return request->setSampleRate(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setMaxSentenceSilence__JI(JNIEnv *env,
                                                                       jobject instance, jlong id, jint _value) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    return request->setMaxSentenceSilence(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setSemanticSentenceDetection(JNIEnv *env, jobject instance, jlong id,
                                                              jboolean _value) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    return request->setSemanticSentenceDetection(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_)
{
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setPayloadParam(value);
    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_) {
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setContextParam(value);
    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_) {
    if (key_ == NULL || value_ == NULL) {
        return -1;
    }

    const char *key = env->GetStringUTFChars(key_, 0);
    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->AppendHttpHeaderParam(key, value);
    env->ReleaseStringUTFChars(value_, value);
    env->ReleaseStringUTFChars(key_, key);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setIntermediateResult(JNIEnv *env, jobject instance, jlong id,
                                                                                        jboolean _value) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setIntermediateResult(_value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setInverseTextNormalization(JNIEnv *env, jobject instance, jlong id,
                                                                                              jboolean _value) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setInverseTextNormalization(_value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_setPunctuationPrediction(JNIEnv *env, jobject instance, jlong id,
                                                                                           jboolean _value) {
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->setPunctuationPrediction(_value);
    return ret;
}
JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_sendAudio(JNIEnv *env, jobject instance, jlong id, jbyteArray data_, jint num_byte)
{
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    SpeechTranscriberRequest* request = (SpeechTranscriberRequest*) id;
    int ret = request->sendAudio((uint8_t*)data, num_byte);
    env->ReleaseByteArrayElements(data_, data, 0);
    return ret;
}

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_SpeechTranscriber_releaseCallback(JNIEnv *env, jobject instance, jlong id) {
    NlsRequestWarpper* wrapper = (NlsRequestWarpper*) id;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(wrapper) != NlsRequestWarpper::_requestMap.end()) {
        NlsRequestWarpper::_requestMap.erase(wrapper);
        LOG_DEBUG("Set request: %p false, size: %d", wrapper, NlsRequestWarpper::_requestMap.size());
    }

    if (wrapper != NULL) {
        LOG_DEBUG("Notify release st callback.");
        delete wrapper;
        wrapper = NULL;
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}
