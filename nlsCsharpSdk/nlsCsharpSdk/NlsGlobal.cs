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
    /// <summary>
    /// 音频编码格式
    /// </summary>
    public enum EncoderType
    {
        /// <summary>
        /// 音频数据PCM编码格式, 即不进行编解码.
        /// </summary>
        ENCODER_PCM = 0,

        /// <summary>
        /// 音频数据OPUS编码格式, 即对传入音频压缩成OPUS格式进行传递.
        /// </summary>
        ENCODER_OPUS = 1,

        /// <summary>
        /// 音频数据OPU编码格式, 即对传入音频压缩成OPU格式进行传递.
        /// OPU格式为阿里内部特殊格式, 不推荐使用.
        /// </summary>
        ENCODER_OPU = 2,
    }

    /// <summary>
    /// 记录日志的等级
    /// </summary>
    public enum LogLevel
    {
        /// <summary>
        /// 只打印和保存Error级别日志
        /// </summary>
        LogError = 1,

        /// <summary>
        /// 打印和保存Error、Warning级别日志
        /// </summary>
        LogWarning,

        /// <summary>
        /// 打印和保存Error、Warning、Info级别日志
        /// </summary>
        LogInfo,

        /// <summary>
        /// 打印和保存Error、Warning、Info、Debug级别日志, Verbose级别也属于Debug.
        /// </summary>
        LogDebug
    };

    /// <summary>
    /// 语音合成服务的版本. 各版本语音合成服务收费不同, 详细请看官网说明.
    /// </summary>
    public enum TtsVersion
    {
        /// <summary>
        /// 选择短文本(300字符内)语音合成服务
        /// </summary>
        ShortTts = 0,

        /// <summary>
        /// 选择长文本(300字符以上)语音合成服务
        /// </summary>
        LongTts,
    };

    /// <summary>
    /// 回调设置的中转参数
    /// </summary>
    public struct SpeechParamStruct
    {
        /// <summary>
        /// 设置回调时传入的用户参数
        /// </summary>
        public string user;

        /// <summary>
        /// 设置回调时传入的回调函数
        /// </summary>
        public CallbackDelegate callback;

        /// <summary>
        /// 用于回调时获取完整事件信息
        /// </summary>
        public NLS_EVENT_STRUCT nlsEvent;
    };

    /// <summary>
    /// 错误码, 负值.
    /// </summary>
    public enum NlsResultCode
    {
        // 900 - 998 reserved for C#

        /// <summary>
        /// 最大错误码
        /// </summary>
        NlsMaxErrorCode = -999,

        /// <summary>
        /// C#层请求为空, 请检查是否已经创建或已经释放.
        /// </summary>
        NativeRequestEmpty = -990,

        /// <summary>
        /// 暂时未实现的接口
        /// </summary>
        InvalidAPI,
    };

    /// <summary>
    /// 声明关于从底层收到回调事件的委托
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void NlsCallbackDelegate(IntPtr handler);

    /// <summary>
    /// 声明关于用户回调的委托
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void CallbackDelegate(ref NLS_EVENT_STRUCT e, ref string s);
}
