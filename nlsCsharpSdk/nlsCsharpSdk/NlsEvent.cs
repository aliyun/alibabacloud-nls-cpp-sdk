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
//	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
	public struct NLS_EVENT_STRUCT
	{
		public int statusCode;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 8192)]
        public string msg;
        public int msgType;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string taskId;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 8192)]
        public string result;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 8192)]
        public string displayText;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 8192)]
        public string spokenText;
        public int sentenceTimeOutStatus;
        public int sentenceIndex;
        public int sentenceTime;
        public int sentenceBeginTime;
        public double sentenceConfidence;
        public bool wakeWordAccepted;
        public bool wakeWordKnown;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        public string wakeWordUserId;
        public int wakeWordGender;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 16384)]
        public byte[] binaryData;
        public int binaryDataSize;

        public int stashResultSentenceId;
        public int stashResultBeginTime;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 8192)]
        public string stashResultText;
        public int stashResultCurrentTime;

        public int isValid;
    }
}
