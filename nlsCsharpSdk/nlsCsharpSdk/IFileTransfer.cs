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
    public interface IFileTransfer
    {
        FileTransferRequest CreateFileTransferRequest();

        void ReleaseFileTransferRequest(FileTransferRequest request);

        int ApplyFileTrans(FileTransferRequest request);

        string GetErrorMsg(FileTransferRequest request);

        string GetResult(FileTransferRequest request);

        void SetKeySecret(FileTransferRequest request, string KeySecret);

        void SetAccessKeyId(FileTransferRequest request, string accessKeyId);

        void SetAppKey(FileTransferRequest request, string appKey);

        void SetFileLinkUrl(FileTransferRequest request, string url);

        void SetRegionId(FileTransferRequest request, string regionId);

        void SetAction(FileTransferRequest request, string action);

        void SetDomain(FileTransferRequest request, string domain);

        void SetServerVersion(FileTransferRequest request, string serverVersion);

        void SetCustomParam(FileTransferRequest request, string customJsonString);
    }
}
