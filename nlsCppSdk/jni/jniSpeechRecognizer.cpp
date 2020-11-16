#include <jni.h>
#include <string>
#include <cstdlib>
#include <vector>
#include "nlsClient.h"
#include "nlsEvent.h"
#include "sr/speechRecognizerRequest.h"
#include "log.h"
#include "NlsRequestWarpper.h"
#include "native-lib.h"
using namespace AlibabaNls;
using namespace AlibabaNls::utility;

extern "C" {
JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_createRecognizerCallback(JNIEnv *env, jobject instance, jobject _callback);

//JNIEXPORT jlong JNICALL
//Java_com_alibaba_idst_util_SpeechRecognizer_createRecognizerCallback(JNIEnv *env, jobject instance);

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_buildRecognizerRequest(JNIEnv *env, jobject instance, jlong wrapper);

//JNIEXPORT jlong JNICALL
//Java_com_alibaba_idst_util_SpeechRecognizer_buildRecognizerRequest(JNIEnv *env, jobject instance, jobject _callback);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_start__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_stop__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_cancel__J(JNIEnv *env, jobject instance, jlong id);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring token_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setIntermediateResult(JNIEnv *env, jobject instance, jlong id, jboolean value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setInverseTextNormalization(JNIEnv *env, jobject instance, jlong id, jboolean value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setPunctuationPrediction(JNIEnv *env, jobject instance, jlong id, jboolean value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_enableVoiceDetection(JNIEnv *env, jobject instance, jlong id, jboolean value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setMaxStartSilence(JNIEnv *env, jobject instance, jlong id, jint _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setMaxEndSilence(JNIEnv *env, jobject instance, jlong id, jint _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id, jint _value);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setCustomizationId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring customizationId_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setVocabularyId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring vocabularyId_);

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_sendAudio(JNIEnv *env, jobject instance, jlong id, jbyteArray data_, jint num_byte);

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_releaseCallback(JNIEnv *env, jobject instance, jlong id);
}

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_createRecognizerCallback(JNIEnv *env, jobject instance, jobject _callback){
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

JNIEXPORT jlong JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_buildRecognizerRequest(JNIEnv *env, jobject instance, jlong wrapper) {
    NlsRequestWarpper* pWrapper = (NlsRequestWarpper*)wrapper;
    SpeechRecognizerRequest* request = gnlsClient->createRecognizerRequest();

    request->setOnTaskFailed(OnTaskFailed, pWrapper);
    request->setOnRecognitionStarted(OnRecognizerStarted, pWrapper);
    request->setOnRecognitionCompleted(OnRecognizerCompleted, pWrapper);
    request->setOnRecognitionResultChanged(OnRecognizedResultChanged, pWrapper);
    request->setOnChannelClosed(OnChannelClosed, pWrapper);

    return (jlong) request;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_start__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->start();
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_stop__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    if (request != NULL) {
        int ret = request->stop();
        gnlsClient->releaseRecognizerRequest(request);
        return ret;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_cancel__J(JNIEnv *env, jobject instance, jlong id) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    if (request != NULL) {
        int ret = request->cancel();
        gnlsClient->releaseRecognizerRequest(request);
        return ret;
    }
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setToken__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id,
                                                                           jstring token_) {
    if (token_ == NULL) {
        return -1;
    }

    const char *token = env->GetStringUTFChars(token_, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setToken(token);
    env->ReleaseStringUTFChars(token_, token);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setCustomizationId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring customizationId_) {
    if (customizationId_ == NULL) {
        return -1;
    }

    const char *customizationId = env->GetStringUTFChars(customizationId_, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setCustomizationId(customizationId);
    env->ReleaseStringUTFChars(customizationId_, customizationId);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setVocabularyId__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring vocabularyId_) {
    if (vocabularyId_ == NULL) {
        return -1;
    }

    const char *vocabularyId = env->GetStringUTFChars(vocabularyId_, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setVocabularyId(vocabularyId);
    env->ReleaseStringUTFChars(vocabularyId_, vocabularyId);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setUrl__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setUrl(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setAppKey__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setAppKey(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setFormat__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring _value) {
    if (_value == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(_value, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setFormat(value);
    env->ReleaseStringUTFChars(_value, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setSampleRate__JI(JNIEnv *env, jobject instance, jlong id, jint _value) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    return request->setSampleRate(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setIntermediateResult(JNIEnv *env, jobject instance, jlong id, jboolean _value) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    return request->setIntermediateResult(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setPunctuationPrediction(JNIEnv *env, jobject instance, jlong id, jboolean _value) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    return request->setPunctuationPrediction(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setInverseTextNormalization(JNIEnv *env, jobject instance, jlong id, jboolean _value) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    return request->setInverseTextNormalization(_value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_enableVoiceDetection(JNIEnv *env, jobject instance, jlong id, jboolean value) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    return request->setEnableVoiceDetection(value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setMaxStartSilence(JNIEnv *env, jobject instance, jlong id, jint value) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    return request->setMaxStartSilence(value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setMaxEndSilence(JNIEnv *env, jobject instance, jlong id, jint value) {
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    return request->setMaxEndSilence(value);
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setParams__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_) {
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setPayloadParam(value);
    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_setContext__JLjava_lang_String_2(JNIEnv *env, jobject instance, jlong id, jstring value_) {
    if (value_ == NULL) {
        return -1;
    }

    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->setContextParam(value);
    env->ReleaseStringUTFChars(value_, value);
    return ret;
}

JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_addHttpHeader(JNIEnv *env, jobject instance, jlong id, jstring key_, jstring value_) {
    if (key_ == NULL || value_ == NULL) {
        return -1;
    }

    const char *key = env->GetStringUTFChars(key_, 0);
    const char *value = env->GetStringUTFChars(value_, 0);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->AppendHttpHeaderParam(key, value);
    env->ReleaseStringUTFChars(value_, value);
    env->ReleaseStringUTFChars(key_, key);
    return ret;
}


JNIEXPORT jint JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_sendAudio(JNIEnv *env, jobject instance, jlong id, jbyteArray data_, jint num_byte) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    SpeechRecognizerRequest* request = (SpeechRecognizerRequest*) id;
    int ret = request->sendAudio((uint8_t*)data, num_byte);
    env->ReleaseByteArrayElements(data_, data, 0);
    return ret;
}

JNIEXPORT void JNICALL
Java_com_alibaba_idst_util_SpeechRecognizer_releaseCallback(JNIEnv *env, jobject instance, jlong id) {
    NlsRequestWarpper* wrapper = (NlsRequestWarpper*) id;

    pthread_mutex_lock(&NlsRequestWarpper::_global_mtx);

    if (NlsRequestWarpper::_requestMap.find(wrapper) != NlsRequestWarpper::_requestMap.end()) {
        NlsRequestWarpper::_requestMap.erase(wrapper);
        LOG_DEBUG("Set request: %p false, size: %d", wrapper, NlsRequestWarpper::_requestMap.size());
    }

    if (wrapper != NULL) {
        LOG_DEBUG("Notify release sr callback.");
        delete wrapper;
        wrapper = NULL;
    }

    pthread_mutex_unlock(&NlsRequestWarpper::_global_mtx);
}
