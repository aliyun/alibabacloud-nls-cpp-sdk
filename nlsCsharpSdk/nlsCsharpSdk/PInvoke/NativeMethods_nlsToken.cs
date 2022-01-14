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
        [DllImport(DllExtern, EntryPoint = "NlsApplyNlsToken", CallingConvention = CallingConvention.Cdecl)]
        public extern static int NlsApplyNlsToken(IntPtr token);

        [DllImport(DllExtern, EntryPoint = "NlsGetErrorMsg", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsGetErrorMsg(IntPtr token);

        [DllImport(DllExtern, EntryPoint = "NlsGetToken", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr NlsGetToken(IntPtr token);

        [DllImport(DllExtern, EntryPoint = "NlsGetExpireTime", CallingConvention = CallingConvention.Cdecl)]
        public extern static UInt32 NlsGetExpireTime(IntPtr token);


        [DllImport(DllExtern, EntryPoint = "NlsSetKeySecret", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void NlsSetKeySecret(IntPtr token, string KeySecret);

        [DllImport(DllExtern, EntryPoint = "NlsSetAccessKeyId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void NlsSetAccessKeyId(IntPtr token, string accessKeyId);

        [DllImport(DllExtern, EntryPoint = "NlsSetDomain", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void NlsSetDomain(IntPtr token, string domain);

        [DllImport(DllExtern, EntryPoint = "NlsSetServerVersion", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void NlsSetServerVersion(IntPtr token, string serverVersion);

        [DllImport(DllExtern, EntryPoint = "NlsSetServerResourcePath", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void NlsSetServerResourcePath(IntPtr token, string serverResourcePath);

        [DllImport(DllExtern, EntryPoint = "NlsSetRegionId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void NlsSetRegionId(IntPtr token, string regionId);

        [DllImport(DllExtern, EntryPoint = "NlsSetAction", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void NlsSetAction(IntPtr token, string action);
    }
}
