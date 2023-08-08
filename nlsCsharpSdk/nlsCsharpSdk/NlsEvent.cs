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

namespace nlsCsharpSdk
{
    static class NlsBufferSize
    {
        public const int NlsEventIdSize = 128;
        public const int NlsEventBinarySize = 16384;
        public const int NlsEventResponseSize = 34816;
        public const int NlsEventResultSize = 10240;
        public const int NlsEventTextSize = 1024;
    }

    /// <summary>
    /// Csharp传递的NLS事件信息.
    /// </summary>
    public struct NLS_EVENT_STRUCT
	{
        /// <summary>
        /// 语音合成的音频数据长度(字节数).
        /// </summary>
        public int binaryDataSize;
        /// <summary>
        /// 语音合成的音频数据.
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = NlsBufferSize.NlsEventBinarySize)]
        public byte[] binaryData;

        /// <summary>
        /// 状态码, 正常情况为0或20000000, 失败时对应失败的错误码, 具体请查看官网.
        /// </summary>
        public int statusCode;

        /// <summary>
        /// 此次事件的完整信息, json string格式.
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = NlsBufferSize.NlsEventResponseSize)]
        public byte[] msg;

        /// <summary>
        /// 此次事件的类型.
        /// </summary>
        public int msgType;

        /// <summary>
        /// 每轮事件对应对应一个独一无二的task id
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = NlsBufferSize.NlsEventIdSize)]
        public string taskId;

        /// <summary>
        /// 识别内容.
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = NlsBufferSize.NlsEventResultSize)]
        public byte[] result;

        /// <summary>
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = NlsBufferSize.NlsEventTextSize)]
        public byte[] displayText;

        /// <summary>
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = NlsBufferSize.NlsEventTextSize)]
        public byte[] spokenText;

        /// <summary>
        /// </summary>
        public int sentenceTimeOutStatus;
        /// <summary>
        /// </summary>
        public int sentenceIndex;
        /// <summary>
        /// </summary>
        public int sentenceTime;
        /// <summary>
        /// </summary>
        public int sentenceBeginTime;
        /// <summary>
        /// </summary>
        public double sentenceConfidence;

        /// <summary>
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool wakeWordAccepted;
        /// <summary>
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool wakeWordKnown;

        /// <summary>
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = NlsBufferSize.NlsEventIdSize)]
        public byte[] wakeWordUserId;

        /// <summary>
        /// </summary>
        public int wakeWordGender;

        /// <summary>
        /// </summary>
        public int stashResultSentenceId;
        /// <summary>
        /// </summary>
        public int stashResultBeginTime;

        /// <summary>
        /// </summary>
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = NlsBufferSize.NlsEventTextSize)]
        public byte[] stashResultText;

        /// <summary>
        /// </summary>
        public int stashResultCurrentTime;

        /// <summary>
        /// </summary>
        [MarshalAs(UnmanagedType.I1)]
        public bool isValid;

        /// <summary>
        /// </summary>
        public IntPtr user;

        /// <summary>
        /// </summary>
        public IntPtr eventMtx;
    }
}
