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
        [DllImport(DllExtern, EntryPoint = "STsetUrl", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetUrl(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetAppKey", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetAppKey(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetToken", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetToken(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetFormat", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetFormat(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetSampleRate", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetSampleRate(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "STsetIntermediateResult", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsetIntermediateResult(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "STsetPunctuationPrediction", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsetPunctuationPrediction(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "STsetInverseTextNormalization", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsetInverseTextNormalization(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "STsetSemanticSentenceDetection", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsetSemanticSentenceDetection(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "STsetMaxSentenceSilence", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsetMaxSentenceSilence(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "STsetCustomizationId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetCustomizationId(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetVocabularyId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetVocabularyId(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetTimeout", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsetTimeout(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "STsetEnableNlp", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsetEnableNlp(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "STsetNlpModel", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetNlpModel(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetSessionId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetSessionId(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetOutputFormat", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetOutputFormat(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STgetOutputFormat", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr STgetOutputFormat(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "STsetPayloadParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetPayloadParam(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STsetContextParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STsetContextParam(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "STappendHttpHeaderParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STappendHttpHeaderParam(IntPtr request, string key, string value);


        [DllImport(DllExtern, EntryPoint = "STGetNlsEvent", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STGetNlsEvent(out NLS_EVENT_STRUCT e);

        #region Callback
        [DllImport(DllExtern, EntryPoint = "STOnTaskFailed", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnTaskFailed(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "STOnTranscriptionStarted", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnTranscriptionStarted(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "STOnChannelClosed", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnChannelClosed(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "STOnSentenceBegin", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnSentenceBegin(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "STOnSentenceEnd", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnSentenceEnd(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "STOnTranscriptionResultChanged", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnTranscriptionResultChanged(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "STOnTranscriptionCompleted", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnTranscriptionCompleted(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "STsetOnSentenceSemantics", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STOnSentenceSemantics(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);
        #endregion


        [DllImport(DllExtern, EntryPoint = "STstart", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STstart(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "STcontrol", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int STcontrol(IntPtr request, string message);

        [DllImport(DllExtern, EntryPoint = "STstop", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STstop(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "STcancel", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STcancel(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "STsendAudio", CallingConvention = CallingConvention.Cdecl)]
        public extern static int STsendAudio(IntPtr request, byte[] data, UInt64 dataSize, int type);

    }
}
