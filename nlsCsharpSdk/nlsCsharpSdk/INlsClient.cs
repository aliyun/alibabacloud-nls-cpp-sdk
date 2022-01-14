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
    public interface INlsClient
    {

        int SetLogConfig(string logOutputFile, LogLevel logLevel, int logFileSize = 10, int logFileNum = 10);

        string GetVersion();

        void StartWorkThread(int threadsNumber = 1);

        void ReleaseInstance();


        SpeechTranscriberRequest CreateTranscriberRequest();

        void ReleaseTranscriberRequest(SpeechTranscriberRequest request);


        SpeechRecognizerRequest CreateRecognizerRequest();

        void ReleaseRecognizerRequest(SpeechRecognizerRequest request);


        SpeechSynthesizerRequest CreateSynthesizerRequest(TtsVersion version = TtsVersion.ShortTts);

        void ReleaseSynthesizerRequest(SpeechSynthesizerRequest request);


        NlsToken CreateNlsToken();

        void ReleaseNlsToken(NlsToken request);
    }
}
