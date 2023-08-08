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

using System;
using System.Runtime.InteropServices;
using System.Security;

namespace nlsCsharpSdk.CPlusPlus
{
    static partial class NativeMethods
    {
        [DllImport(DllExtern, EntryPoint = "NlsGetVersion", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsGetVersion();

        [DllImport(DllExtern,EntryPoint = "NlsSetLogConfig", CallingConvention = CallingConvention.Cdecl)]
        public extern static int NlsSetLogConfig(string logFileName, int logLevel, int logFileSize, int logFileNum);

        [DllImport(DllExtern, EntryPoint = "NlsSetAddrInFamily", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsSetAddrInFamily(string aiFamily);

        [DllImport(DllExtern, EntryPoint = "NlsSetDirectHost", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsSetDirectHost(string ip);

        [DllImport(DllExtern, EntryPoint = "NlsCalculateUtf8Chars", CallingConvention = CallingConvention.Cdecl)]
        public extern static int NlsCalculateUtf8Chars(string text);


        [DllImport(DllExtern, EntryPoint = "NlsStartWorkThread", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsStartWorkThread(int threadsNumber);

        [DllImport(DllExtern, EntryPoint = "NlsReleaseInstance", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsReleaseInstance();


        [DllImport(DllExtern, EntryPoint = "NlsCreateTranscriberRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsCreateTranscriberRequest();

        [DllImport(DllExtern, EntryPoint = "NlsReleaseTranscriberRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsReleaseTranscriberRequest(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "NlsCreateRecognizerRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsCreateRecognizerRequest();

        [DllImport(DllExtern, EntryPoint = "NlsReleaseRecognizerRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsReleaseRecognizerRequest(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "NlsCreateSynthesizerRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsCreateSynthesizerRequest(int type);

        [DllImport(DllExtern, EntryPoint = "NlsReleaseSynthesizerRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsReleaseSynthesizerRequest(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "NlsCreateFileTransferRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsCreateFileTransferRequest();

        [DllImport(DllExtern, EntryPoint = "NlsReleaseFileTransferRequest", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsReleaseFileTransferRequest(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "NlsCreateNlsToken", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsCreateNlsToken();

        [DllImport(DllExtern, EntryPoint = "NlsReleaseNlsToken", CallingConvention = CallingConvention.Cdecl)]
        public extern static void NlsReleaseNlsToken(IntPtr request);


    }
}
