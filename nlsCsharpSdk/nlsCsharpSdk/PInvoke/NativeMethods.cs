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
using System.Security;
using System.Security.Permissions;
using nlsCsharpSdk;

namespace nlsCsharpSdk.CPlusPlus
{
    [SuppressUnmanagedCodeSecurity]
    internal static partial class NativeMethods
    {
        /// <summary>
        /// DLL file name
        /// </summary>
        public const string DllExtern = "nlsCsharpSdkExtern.dll";

        /// <summary>
        /// Static constructor
        /// </summary>
        /// 
        [SecurityPermission(SecurityAction.Demand, Flags = SecurityPermissionFlag.UnmanagedCode)]

        static NativeMethods()
        {
            TryPInvoke();
        }

        private static void TryPInvoke()
        {
            if (tried)
                return;
            tried = true;
        }
        private static bool tried = false;
    }
}
