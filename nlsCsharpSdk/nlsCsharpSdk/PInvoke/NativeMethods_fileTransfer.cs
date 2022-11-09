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

namespace nlsCsharpSdk.CPlusPlus
{
    static partial class NativeMethods
    {
        [DllImport(DllExtern, EntryPoint = "FTapplyFileTrans", CallingConvention = CallingConvention.Cdecl)]
        public extern static int FTapplyFileTrans(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "FTgetErrorMsg", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr FTgetErrorMsg(IntPtr request);

        [DllImport(DllExtern, EntryPoint = "FTgetResult", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr FTgetResult(IntPtr request);


        [DllImport(DllExtern, EntryPoint = "FTsetKeySecret", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetKeySecret(IntPtr request, string KeySecret);

        [DllImport(DllExtern, EntryPoint = "FTsetAccessKeyId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetAccessKeyId(IntPtr request, string accessKeyId);

        [DllImport(DllExtern, EntryPoint = "FTsetAppKey", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetAppKey(IntPtr request, string appkey);

        [DllImport(DllExtern, EntryPoint = "FTsetFileLinkUrl", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetFileLinkUrl(IntPtr request, string url);

        [DllImport(DllExtern, EntryPoint = "FTsetRegionId", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetRegionId(IntPtr request, string regionId);

        [DllImport(DllExtern, EntryPoint = "FTsetAction", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetAction(IntPtr request, string action);

        [DllImport(DllExtern, EntryPoint = "FTsetDomain", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetDomain(IntPtr request, string domain);

        [DllImport(DllExtern, EntryPoint = "FTSetServerVersion", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTSetServerVersion(IntPtr request, string serverVersion);

        [DllImport(DllExtern, EntryPoint = "FTSetCustomParam", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTSetCustomParam(IntPtr request, string customJsonString);

        [DllImport(DllExtern, EntryPoint = "FTsetOutputFormat", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public extern static void FTsetOutputFormat(IntPtr request, string textFormat);
    }
}
