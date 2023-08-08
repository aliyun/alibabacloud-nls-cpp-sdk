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
        [DllImport(DllExtern, EntryPoint = "SRsetUrl", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetUrl(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRsetAppKey", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetAppKey(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRsetToken", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetToken(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRsetFormat", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetFormat(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRsetSampleRate", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetSampleRate(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SRsetIntermediateResult", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsetIntermediateResult(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "SRsetPunctuationPrediction", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsetPunctuationPrediction(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "SRsetInverseTextNormalization", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsetInverseTextNormalization(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "SRsetEnableVoiceDetection", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsetEnableVoiceDetection(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "SRsetMaxStartSilence", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsetMaxStartSilence(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SRsetMaxEndSilence", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsetMaxEndSilence(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SRsetCustomizationId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetCustomizationId(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRsetVocabularyId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetVocabularyId(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRsetTimeout", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsetTimeout(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SRsetOutputFormat", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetOutputFormat(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRgetOutputFormat", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr SRgetOutputFormat(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "SRsetPayloadParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetPayloadParam(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRsetContextParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRsetContextParam(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SRappendHttpHeaderParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SRappendHttpHeaderParam(IntPtr request, string key, string value);


        [DllImport(DllExtern, EntryPoint = "SRGetNlsEvent", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRGetNlsEvent(out NLS_EVENT_STRUCT e);

        #region Callback
        [DllImport(DllExtern, EntryPoint = "SROnTaskFailed", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SROnTaskFailed(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SROnRecognitionStarted", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SROnRecognitionStarted(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SROnChannelClosed", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SROnChannelClosed(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SROnRecognitionResultChanged", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SROnRecognitionResultChanged(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SROnRecognitionCompleted", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SROnRecognitionCompleted(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        #endregion


        [DllImport(DllExtern, EntryPoint = "SRstart", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRstart(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "SRstop", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRstop(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "SRcancel", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRcancel(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "SRsendAudio", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SRsendAudio(IntPtr request, byte[] data, UInt64 dataSize, int type);

    }
}
