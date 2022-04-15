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
        public IntPtr native_request;

        public FileTransferRequest CreateFileTransferRequest()
        {
            IntPtr request = NativeMethods.NlsCreateFileTransferRequest();
            FileTransferRequest FTrequest = new FileTransferRequest();
            FTrequest.native_request = request;
            return FTrequest;
        }

        public void ReleaseFileTransferRequest(FileTransferRequest request)
        {
            NativeMethods.NlsReleaseTranscriberRequest(request.native_request);
            return;
        }

        public int ApplyFileTrans(FileTransferRequest request)
        {
            return NativeMethods.FTapplyFileTrans(request.native_request);
        }

        public string GetErrorMsg(FileTransferRequest request)
        {
            IntPtr get = NativeMethods.FTgetErrorMsg(request.native_request);
            string error = Marshal.PtrToStringAnsi(get);
            return error;
        }

        public string GetResult(FileTransferRequest request)
        {
            IntPtr get = NativeMethods.FTgetResult(request.native_request);
            string result = Marshal.PtrToStringAnsi(get);
            return result;
        }

        public void SetKeySecret(FileTransferRequest request, string KeySecret)
        {
            NativeMethods.FTsetKeySecret(request.native_request, KeySecret);
            return;
        }

        public void SetAccessKeyId(FileTransferRequest request, string accessKeyId)
        {
            NativeMethods.FTsetAccessKeyId(request.native_request, accessKeyId);
            return;
        }

        public void SetAppKey(FileTransferRequest request, string appKey)
        {
            NativeMethods.FTsetAppKey(request.native_request, appKey);
            return;
        }

        public void SetFileLinkUrl(FileTransferRequest request, string url)
        {
            NativeMethods.FTsetFileLinkUrl(request.native_request, url);
            return;
        }

        public void SetRegionId(FileTransferRequest request, string regionId)
        {
            NativeMethods.FTsetRegionId(request.native_request, regionId);
            return;
        }

        public void SetAction(FileTransferRequest request, string action)
        {
            NativeMethods.FTsetAction(request.native_request, action);
            return;
        }

        public void SetDomain(FileTransferRequest request, string domain)
        {
            NativeMethods.FTsetDomain(request.native_request, domain);
            return;
        }

        public void SetServerVersion(FileTransferRequest request, string serverVersion)
        {
            NativeMethods.FTSetServerVersion(request.native_request, serverVersion);
            return;
        }

        public void SetCustomParam(FileTransferRequest request, string customJsonString)
        {
            NativeMethods.FTSetCustomParam(request.native_request, customJsonString);
            return;
        }
    }
}
