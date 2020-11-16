#include <jni.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <map>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "sy/speechSynthesizerRequest.h"
#include "log.h"
#include "NlsRequestWarpper.h"
#include "native-lib.h"
using namespace AlibabaNls;
using namespace utility;

extern "C"
{
JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_createSynthesizerCallback(JNIEnv *env, jobject instance, jobject _callback);

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_buildSynthesizerRequest(JNIEnv *env, jobject instance, jlong wrapper);

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_buildLongSynthesizerRequest(JNIEnv *env, jobject instance, jlong wrapper);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_start__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_stop__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_cancel__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring token_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id, jint _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setText__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setVoice__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setVolume__JI(JNIEnv *env, jobject instance, jlong id, jint value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setSpeechRate__JI(JNIEnv *env, jobject instance, jlong id, jint value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setPitchRate__JI(JNIEnv *env, jobject instance, jlong id, jint value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setMethod__JI(JNIEnv *env, jobject instance, jlong id, jint value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setEnableSubtitle(JNIEnv *env, jobject instance, jlong id, jboolean _value);

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_releaseCallback(JNIEnv *env, jobject instance, jlong id);

}

void OnSynthesisStarted(NlsEvent* str, void* para = NULL) {
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
        LOG_DEBUG("TTS Started Callback is free.");
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
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onSynthesisStarted","(Ljava/lang/String;I)V");
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

void OnMetaInfo(NlsEvent* str, void* para = NULL) {
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
        LOG_DEBUG("TTS Completed Callback is free.");
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
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onMetaInfo","(Ljava/lang/String;I)V");

    jstring msg = env->NewStringUTF(str->getAllResponse());

//    jbyteArray array = env->NewByteArray(env, strlen(str->getAllResponse()));
//    env->SetByteArrayRegion(env, array, 0, strlen(str->getAllResponse()), str->getAllResponse());
//    jstring strEncode = env->NewStringUTF(env, "UTF-8");
//    jclass cls = env->FindClass(env, "java/lang/String");
//    jmethodID ctor = env->GetMethodID(env, cls, "<init>", "([BLjava/lang/String;)V");
//    jstring msg = (jstring) (*env)->NewObject(env, cls, ctor, array, strEncode);

    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,msg,code);
    env->DeleteLocalRef(native_clazz);
    env->DeleteLocalRef( msg);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}

void OnSynthesisCompleted(NlsEvent* str, void* para = NULL) {
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
        LOG_DEBUG("TTS Completed Callback is free.");
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
    jmethodID methodID_func= env->GetMethodID(native_clazz,"onSynthesisCompleted","(Ljava/lang/String;I)V");
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

void OnBinaryReceived(NlsEvent* str, void* para = NULL) {
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
        LOG_DEBUG("TTS BinaryReceived Callback is free.");
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
    jmethodID methodID_func=env->GetMethodID(native_clazz,"onBinaryReceived","([BI)V");
    std::vector<unsigned char> msg = str->getBinaryData();
    jbyteArray array_data = env->NewByteArray(msg.size());
    env->SetByteArrayRegion(array_data, 0, msg.size(), (jbyte*)&msg[0]);
    jint code = str->getStatusCode();
    env->CallVoidMethod(tst->_callback,methodID_func,array_data,code);
    env->DeleteLocalRef(array_data);
    env->DeleteLocalRef(native_clazz);

    if (attached) {
        tst->_jvm->DetachCurrentThread();
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_createSynthesizerCallback(JNIEnv *env, jobject instance, jobject _callback) {
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
Java_com_alibaba_idst_util_SpeechSynthesizer_buildSynthesizerRequest(JNIEnv *env, jobject instance, jlong wrapper) {
    NlsRequestWarpper* pWrapper = (NlsRequestWarpper*)wrapper;
    SpeechSynthesizerRequest* request = gnlsClient->createSynthesizerRequest(AlibabaNls::TtsVersion::ShortTts);

    request->setOnTaskFailed(OnTaskFailed, pWrapper);
    request->setOnSynthesisCompleted(OnSynthesisCompleted, pWrapper);
    request->setOnMetaInfo(OnMetaInfo, pWrapper);
    request->setOnBinaryDataReceived(OnBinaryReceived, pWrapper);
    request->setOnChannelClosed(OnChannelClosed, pWrapper);

    return (jlong) request;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_buildLongSynthesizerRequest(JNIEnv *env, jobject instance, jlong wrapper) {
    NlsRequestWarpper* pWrapper = (NlsRequestWarpper*)wrapper;
    SpeechSynthesizerRequest* request = gnlsClient->createSynthesizerRequest(AlibabaNls::TtsVersion::LongTts);

    request->setOnTaskFailed(OnTaskFailed, pWrapper);
    request->setOnSynthesisCompleted(OnSynthesisCompleted, pWrapper);
    request->setOnMetaInfo(OnMetaInfo, pWrapper);
    request->setOnBinaryDataReceived(OnBinaryReceived, pWrapper);
    request->setOnChannelClosed(OnChannelClosed, pWrapper);

    return (jlong) request;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_start__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->start();
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_stop__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    if (request != NULL) {
        int ret = request->stop();
        gnlsClient->releaseSynthesizerRequest(request);
        return ret;
    }

    return 0;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_cancel__J(JNIEnv *env, jobject instance, jlong id) {

    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    if (request != NULL) {
        int ret = request->cancel();
        gnlsClient->releaseSynthesizerRequest(request);
        return ret;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                           jstring token_) {
    if (token_ == NULL) {
        return -1;
    }

    const char *token = env->GetStringUTFChars(token_, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setToken(token);
    env->ReleaseStringUTFChars(token_, token);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                         jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setUrl(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                            jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setAppKey(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}


JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                            jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setFormat(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}


JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id,
                                                              jint _value) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    return request->setSampleRate(_value);
}


JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setText__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                                        jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setText(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setVoice__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring _value) {

    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setVoice(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setVolume__JI(JNIEnv *env, jobject instance, jlong id, jint _value) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    return request->setVolume(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setSpeechRate__JI(JNIEnv *env, jobject instance, jlong id, jint _value) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    return request->setSpeechRate(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setPitchRate__JI(JNIEnv *env, jobject instance, jlong id, jint _value) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    return request->setPitchRate(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setMethod__JI(JNIEnv *env, jobject instance, jlong id, jint _value) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    return request->setMethod(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setEnableSubtitle(JNIEnv *env, jobject instance, jlong id, jboolean _value) {
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    return request->setEnableSubtitle(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_) {
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setPayloadParam(value);

    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_setContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_) {
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->setContextParam(value);

    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_) {
    if (key_ == NULL || value_ == NULL) {
        return -1;
    }

    const char *key = env->GetStringUTFChars(key_, 0);
    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechSynthesizerRequest* request = (SpeechSynthesizerRequest*) id;
    int ret = request->AppendHttpHeaderParam(key, value);
    env->ReleaseStringUTFChars(value_, value);
    env->ReleaseStringUTFChars(key_, key);
    return ret;
}

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_SpeechSynthesizer_releaseCallback(JNIEnv *env, jobject instance, jlong id) {
    NlsRequestWarpper* wrapper = (NlsRequestWarpper*) id;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(wrapper) != NlsRequestWarpper::_requestMap.end()) {
        NlsRequestWarpper::_requestMap.erase(wrapper);
        LOG_DEBUG("Set request: %p false, size: %d", wrapper, NlsRequestWarpper::_requestMap.size());
    }

    if (wrapper != NULL) {
        LOG_DEBUG("Notify release tts callback.");
        delete wrapper;
        wrapper = NULL;
    }
    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}
