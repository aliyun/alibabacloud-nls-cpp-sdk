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
    public interface INlsToken
    {

        int ApplyNlsToken(NlsToken token);

        string GetErrorMsg(NlsToken token);

        string GetToken(NlsToken token);

        UInt32 GetExpireTime(NlsToken token);

        void SetKeySecret(NlsToken token, string KeySecret);

        void SetAccessKeyId(NlsToken token, string accessKeyId);

        void SetDomain(NlsToken token, string domain);

        void SetServerVersion(NlsToken token, string serverVersion);

        void SetServerResourcePath(NlsToken token, string serverResourcePath);

        void SetRegionId(NlsToken token, string regionId);

        void SetAction(NlsToken token, string action);
    }
}
