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
    /// <summary>
    /// NLS TOKEN 操作入口.
    /// </summary>
    public interface INlsToken
    {
        /// <summary>
        /// 申请获取token.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns></returns>
        int ApplyNlsToken(NlsToken token);

        /// <summary>
        /// 获取错误信息.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns>成功则返回错误信息; 失败返回NULL.</returns>
        string GetErrorMsg(NlsToken token);

        /// <summary>
        /// 获取token id.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns>返回 token 字符串.</returns>
        string GetToken(NlsToken token);

        /// <summary>
        /// 获得token有效期时间戳(秒).
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns>成功则返回有效期时间戳, 失败返回0.</returns>
        UInt32 GetExpireTime(NlsToken token);

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
        void SetKeySecret(NlsToken token, string KeySecret);

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
        void SetAccessKeyId(NlsToken token, string accessKeyId);

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
        void SetDomain(NlsToken token, string domain);

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
        void SetServerVersion(NlsToken token, string serverVersion);

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
        void SetServerResourcePath(NlsToken token, string serverResourcePath);

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
        void SetRegionId(NlsToken token, string regionId);

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
        void SetAction(NlsToken token, string action);
    }
}
