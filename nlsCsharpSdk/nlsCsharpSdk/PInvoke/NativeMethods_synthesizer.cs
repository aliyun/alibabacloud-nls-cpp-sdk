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
        [DllImport(DllExtern, EntryPoint = "SYsetUrl", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetUrl(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYsetAppKey", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetAppKey(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYsetToken", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetToken(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYsetFormat", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetFormat(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYsetSampleRate", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetSampleRate(IntPtr request, int value);

        //[DllImport(DllExtern, EntryPoint = "SYsetText", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        [DllImport(DllExtern, EntryPoint = "SYsetText", CallingConvention = CallingConvention.Cdecl)]
        //public extern static int SYsetText(IntPtr request, string value);
        public extern static int SYsetText(IntPtr request, byte[] text, UInt32 textSize);


        [DllImport(DllExtern, EntryPoint = "SYsetVoice", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetVoice(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYsetVolume", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYsetVolume(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SYsetSpeechRate", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYsetSpeechRate(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SYsetPitchRate", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYsetPitchRate(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SYsetMethod", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYsetMethod(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SYsetEnableSubtitle", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYsetEnableSubtitle(IntPtr request, bool value);

        [DllImport(DllExtern, EntryPoint = "SYsetTimeout", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYsetTimeout(IntPtr request, int value);

        [DllImport(DllExtern, EntryPoint = "SYsetOutputFormat", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetOutputFormat(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYgetOutputFormat", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr SYgetOutputFormat(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "SYsetPayloadParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetPayloadParam(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYsetContextParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYsetContextParam(IntPtr request, string value);

        [DllImport(DllExtern, EntryPoint = "SYappendHttpHeaderParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static int SYappendHttpHeaderParam(IntPtr request, string key, string value);


        [DllImport(DllExtern, EntryPoint = "SYGetNlsEvent", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYGetNlsEvent(out NLS_EVENT_STRUCT e);

        #region Callback
        [DllImport(DllExtern, EntryPoint = "SYOnTaskFailed", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYOnTaskFailed(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SYOnChannelClosed", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYOnChannelClosed(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SYOnBinaryDataReceived", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYOnBinaryDataReceived(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SYOnSynthesisCompleted", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYOnSynthesisCompleted(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);

        [DllImport(DllExtern, EntryPoint = "SYOnMetaInfo", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYOnMetaInfo(
            IntPtr request, [MarshalAs(UnmanagedType.FunctionPtr)] NlsCallbackDelegate c, IntPtr user);
        #endregion


        [DllImport(DllExtern, EntryPoint = "SYstart", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYstart(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "SYstop", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYstop(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "SYcancel", CallingConvention = CallingConvention.Cdecl)]
        public extern static int SYcancel(IntPtr request);

    }
}
