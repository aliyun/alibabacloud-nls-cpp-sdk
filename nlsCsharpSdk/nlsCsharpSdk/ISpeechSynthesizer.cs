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
    public interface ISpeechSynthesizer
    {
        int SetUrl(SpeechSynthesizerRequest request, string value);

        int SetAppKey(SpeechSynthesizerRequest request, string value);

        int SetToken(SpeechSynthesizerRequest request, string value);

        int SetFormat(SpeechSynthesizerRequest request, string value);

        int SetSampleRate(SpeechSynthesizerRequest request, int value);

        int SetText(SpeechSynthesizerRequest request, string value);

        int SetVoice(SpeechSynthesizerRequest request, string value);

        int SetVolume(SpeechSynthesizerRequest request, int value);

        int SetSpeechRate(SpeechSynthesizerRequest request, int value);

        int SetPitchRate(SpeechSynthesizerRequest request, int value);

        int SetMethod(SpeechSynthesizerRequest request, int value);


        int SetEnableSubtitle(SpeechSynthesizerRequest request, bool value);

        int SetPayloadParam(SpeechSynthesizerRequest request, string value);

        int SetOutputFormat(SpeechSynthesizerRequest request, string value);

        int SetTimeout(SpeechSynthesizerRequest request, int value);

        int SetContextParam(SpeechSynthesizerRequest request, string value);

        int AppendHttpHeaderParam(SpeechSynthesizerRequest request, string key, string value);


        int Start(SpeechSynthesizerRequest request);

        int Stop(SpeechSynthesizerRequest request);

        int Cancel(SpeechSynthesizerRequest request);


        void SetOnTaskFailed(SpeechSynthesizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnSynthesisCompleted(SpeechSynthesizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnChannelClosed(SpeechSynthesizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnBinaryDataReceived(SpeechSynthesizerRequest request, CallbackDelegate callback, object para = null);

        void SetOnMetaInfo(SpeechSynthesizerRequest request, CallbackDelegate callback, object para = null);
    }
}
