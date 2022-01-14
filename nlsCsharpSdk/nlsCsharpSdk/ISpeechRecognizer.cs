/*
 * Copyright 2021 Alibaba Group Holding Limited
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

namespace nlsCsharpSdk
{
    public interface ISpeechRecognizer
    {

        int SetUrl(SpeechRecognizerRequest request, string value);

        int SetAppKey(SpeechRecognizerRequest request, string value);

        int SetToken(SpeechRecognizerRequest request, string value);

        int SetFormat(SpeechRecognizerRequest request, string value);

        int SetSampleRate(SpeechRecognizerRequest request, int value);

        int SetIntermediateResult(SpeechRecognizerRequest request, bool value);

        int SetPunctuationPrediction(SpeechRecognizerRequest request, bool value);

        int SetInverseTextNormalization(SpeechRecognizerRequest request, bool value);

        int SetEnableVoiceDetection(SpeechRecognizerRequest request, bool value);

        int SetMaxStartSilence(SpeechRecognizerRequest request, int value);

        int SetMaxEndSilence(SpeechRecognizerRequest request, int value);

        int SetCustomizationId(SpeechRecognizerRequest request, string value);

        int SetVocabularyId(SpeechRecognizerRequest request, string value);

        int SetTimeout(SpeechRecognizerRequest request, int value);

        int SetOutputFormat(SpeechRecognizerRequest request, string value);

        int SetPayloadParam(SpeechRecognizerRequest request, string value);

        int SetContextParam(SpeechRecognizerRequest request, string value);

        int AppendHttpHeaderParam(SpeechRecognizerRequest request, string key, string value);


        int Start(SpeechRecognizerRequest request);

        int Stop(SpeechRecognizerRequest request);

        int Cancel(SpeechRecognizerRequest request);

        int SendAudio(SpeechRecognizerRequest request, byte[] data, UInt64 dataSize, EncoderType type);


        void SetOnTaskFailed(SpeechRecognizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnRecognitionStarted(SpeechRecognizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnRecognitionResultChanged(SpeechRecognizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnRecognitionCompleted(SpeechRecognizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnChannelClosed(SpeechRecognizerRequest request, CallbackDelegate callback, object para = null);
    }
}
