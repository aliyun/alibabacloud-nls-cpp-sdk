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

using nlsCsharpSdk.CPlusPlus;
using System.Runtime.InteropServices;

namespace nlsCsharpSdk
{
    /// <summary>
    /// NLS TOKEN 操作入口.
    /// </summary>

    public class NlsToken : INlsToken
    {
        /// <summary>
        /// NLS TOKEN请求的Native指针.
        /// </summary>
        public IntPtr native_token;

        /// <summary>
        /// 申请获取token.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns></returns>
        public int ApplyNlsToken(NlsToken token)
        {
            return NativeMethods.NlsApplyNlsToken(token.native_token);
        }

        /// <summary>
        /// 获取错误信息.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns>成功则返回错误信息; 失败返回NULL.</returns>
        public string GetErrorMsg(NlsToken token)
        {
            IntPtr get = NativeMethods.NlsGetErrorMsg(token.native_token);
            string error = Marshal.PtrToStringAnsi(get);
            return error;
        }

        /// <summary>
        /// 获取token id.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns>返回 token 字符串.</returns>
        public string GetToken(NlsToken token)
        {
            IntPtr get = NativeMethods.NlsGetToken(token.native_token);
            string token_str = Marshal.PtrToStringAnsi(get);
            return token_str;
        }

        /// <summary>
        /// 获得token有效期时间戳(秒).
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns>成功则返回有效期时间戳, 失败返回0.</returns>
        public UInt32 GetExpireTime(NlsToken token)
        {
            return NativeMethods.NlsGetExpireTime(token.native_token);
        }

        /// <summary>
        /// 设置阿里云账号的KeySecret.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <param name="KeySecret">
        /// Secret字符串.
        /// </param>
        /// <returns></returns>
        public void SetKeySecret(NlsToken token, string KeySecret)
        {
            NativeMethods.NlsSetKeySecret(token.native_token, KeySecret);
            return;
        }

        /// <summary>
        /// 设置阿里云账号的KeyId.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <param name="accessKeyId">
        /// Access Key Id 字符串.
        /// </param>
        /// <returns></returns>
        public void SetAccessKeyId(NlsToken token, string accessKeyId)
        {
            NativeMethods.NlsSetAccessKeyId(token.native_token, accessKeyId);
            return;
        }

        /// <summary>
        /// 设置域名.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <param name="domain">
        /// 域名url字符串.
        /// </param>
        /// <returns></returns>
        public void SetDomain(NlsToken token, string domain)
        {
            NativeMethods.NlsSetDomain(token.native_token, domain);
            return;
        }

        /// <summary>
        /// 设置API版本.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <param name="serverVersion">
        /// API版本字符串.
        /// </param>
        /// <returns></returns>
        public void SetServerVersion(NlsToken token, string serverVersion)
        {
            NativeMethods.NlsSetServerVersion(token.native_token, serverVersion);
            return;
        }

        /// <summary>
        /// 设置服务路径.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <param name="serverResourcePath">
        /// 服务路径字符串.
        /// </param>
        /// <returns></returns>
        public void SetServerResourcePath(NlsToken token, string serverResourcePath)
        {
            NativeMethods.NlsSetServerResourcePath(token.native_token, serverResourcePath);
            return;
        }

        /// <summary>
        /// 设置RegionId.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <param name="regionId">
        /// 服务地区.
        /// </param>
        /// <returns></returns>
        public void SetRegionId(NlsToken token, string regionId)
        {
            NativeMethods.NlsSetRegionId(token.native_token, regionId);
            return;
        }

        /// <summary>
        /// 设置功能.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <param name="action">
        /// 功能.
        /// </param>
        /// <returns></returns>
        public void SetAction(NlsToken token, string action)
        {
            NativeMethods.NlsSetAction(token.native_token, action);
            return;
        }
    }
}
