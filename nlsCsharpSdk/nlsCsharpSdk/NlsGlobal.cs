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

using System.Runtime.InteropServices;

namespace nlsCsharpSdk
{

    public enum EncoderType
    {
        ENCODER_PCM = 0,
        ENCODER_OPUS = 1,
        ENCODER_OPU = 2,
    }

    public enum LogLevel
    {
        LogError = 1,
        LogWarning,
        LogInfo,
        LogDebug
    };

    public enum TtsVersion
    {
        ShortTts = 0,
        LongTts,
    };

    // 声明关于事件的委托
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void NlsCallbackDelegate(int status);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void CallbackDelegate(ref NLS_EVENT_STRUCT e);
}
