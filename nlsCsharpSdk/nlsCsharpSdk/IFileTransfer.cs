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
    /// 录音文件识别
    /// </summary>
    public interface IFileTransfer
    {

        /// <summary>
        /// 调用文件转写. 调用之前, 请先设置请求参数.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回0, 否则返回-1.</returns>
        int ApplyFileTrans(FileTransferRequest request);

        /// <summary>
        /// 当ApplyFileTrans返回失败时, 获取错误信息.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回错误信息; 失败返回NULL.</returns>
        string GetErrorMsg(FileTransferRequest request);

        /// <summary>
        /// 当ApplyFileTrans返回成功时, 获取json string格式结果.
        /// </summary>
        /// <param name="request">
        /// CreateFileTransferRequest所建立的request对象.
        /// </param>
        /// <returns>成功则返回json格式字符串; 失败返回NULL.</returns>
        string GetResult(FileTransferRequest request);

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
        void SetKeySecret(FileTransferRequest request, string KeySecret);

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
        void SetAccessKeyId(FileTransferRequest request, string accessKeyId);

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
        void SetAppKey(FileTransferRequest request, string appKey);

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
        void SetFileLinkUrl(FileTransferRequest request, string url);

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
        void SetRegionId(FileTransferRequest request, string regionId);

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
        void SetAction(FileTransferRequest request, string action);

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
        void SetDomain(FileTransferRequest request, string domain);

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
        void SetServerVersion(FileTransferRequest request, string serverVersion);

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
        void SetCustomParam(FileTransferRequest request, string customJsonString);

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
        void SetOutputFormat(FileTransferRequest request, string textFormat);
    }
}
