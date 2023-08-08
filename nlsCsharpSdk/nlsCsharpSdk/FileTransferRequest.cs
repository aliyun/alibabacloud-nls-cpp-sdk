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
using System.Text;

namespace nlsCsharpSdk
{
    /// <summary>
    /// 录音文件识别
    /// </summary>

    public class FileTransferRequest : IFileTransfer
    {
        /// <summary>
        /// 录音文件识别请求的Native指针.
        /// </summary>
        public IntPtr native_request;


        /// <summary>
        /// 调用文件转写. 调用之前, 请先设置请求参数.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        public int ApplyFileTrans(FileTransferRequest request)
        {
            return NativeMethods.FTapplyFileTrans(request.native_request);
        }

        /// <summary>
        /// 当ApplyFileTrans返回失败时, 获取错误信息.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回错误信息; 失败返回NULL.</returns>
        public string GetErrorMsg(FileTransferRequest request)
        {
            IntPtr get = NativeMethods.FTgetErrorMsg(request.native_request);
            string error = Marshal.PtrToStringAnsi(get);
            return error;
        }

        /// <summary>
        /// 当ApplyFileTrans返回成功时, 获取json string格式结果.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回json格式字符串; 失败返回NULL.</returns>
        public string GetResult(FileTransferRequest request)
        {
            IntPtr get = NativeMethods.FTgetResult(request.native_request);
            string result = Marshal.PtrToStringAnsi(get);
            return result;
        }

        /// <summary>
        /// 设置阿里云账号的KeySecret. 如何获取请查看官网文档说明.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="KeySecret">
        /// AccessKeySecret字符串.
        /// </param>
        /// <returns></returns>
        public void SetKeySecret(FileTransferRequest request, string KeySecret)
        {
            NativeMethods.FTsetKeySecret(request.native_request, KeySecret);
            return;
        }

        /// <summary>
        /// 设置阿里云账号的KeyId. 如何获取请查看官网文档说明.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="accessKeyId">
        /// AccessKeyId字符串.
        /// </param>
        /// <returns></returns>
        public void SetAccessKeyId(FileTransferRequest request, string accessKeyId)
        {
            NativeMethods.FTsetAccessKeyId(request.native_request, accessKeyId);
            return;
        }

        /// <summary>
        /// 设置APPKEY. 如何获取请查看官网文档说明.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="appKey">
        /// appKey字符串.
        /// </param>
        /// <returns></returns>
        public void SetAppKey(FileTransferRequest request, string appKey)
        {
            NativeMethods.FTsetAppKey(request.native_request, appKey);
            return;
        }

        /// <summary>
        /// 设置音频文件连接地址.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="url">
        /// 音频文件URL地址.
        /// </param>
        /// <returns></returns>
        public void SetFileLinkUrl(FileTransferRequest request, string url)
        {
            NativeMethods.FTsetFileLinkUrl(request.native_request, url);
            return;
        }

        /// <summary>
        /// 设置地域ID.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="regionId">
        /// regionId 服务地区.
        /// </param>
        /// <returns></returns>
        public void SetRegionId(FileTransferRequest request, string regionId)
        {
            NativeMethods.FTsetRegionId(request.native_request, regionId);
            return;
        }

        /// <summary>
        /// 设置功能.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="action">
        /// action 功能.
        /// </param>
        /// <returns></returns>
        public void SetAction(FileTransferRequest request, string action)
        {
            NativeMethods.FTsetAction(request.native_request, action);
            return;
        }

        /// <summary>
        /// 设置域名.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="domain">
        /// domain 域名字符串.
        /// </param>
        /// <returns></returns>
        public void SetDomain(FileTransferRequest request, string domain)
        {
            NativeMethods.FTsetDomain(request.native_request, domain);
            return;
        }

        /// <summary>
        /// 设置API版本.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="serverVersion">
        /// serverVersion API版本字符串.
        /// </param>
        /// <returns></returns>
        public void SetServerVersion(FileTransferRequest request, string serverVersion)
        {
            NativeMethods.FTSetServerVersion(request.native_request, serverVersion);
            return;
        }

        /// <summary>
        /// 输入参数.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="customJsonString">
        /// customJsonString json字符串.
        /// </param>
        /// <returns></returns>
        public void SetCustomParam(FileTransferRequest request, string customJsonString)
        {
            NativeMethods.FTSetCustomParam(request.native_request, customJsonString);
            return;
        }

        /// <summary>
        /// 设置输出文本的编码格式. 默认GBK.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <param name="textFormat">
        /// textFormat 编码格式 UTF-8 or GBK.
        /// </param>
        /// <returns></returns>
        public void SetOutputFormat(FileTransferRequest request, string textFormat)
        {
            NativeMethods.FTsetOutputFormat(request.native_request, textFormat);
            return;
        }
    }
}
