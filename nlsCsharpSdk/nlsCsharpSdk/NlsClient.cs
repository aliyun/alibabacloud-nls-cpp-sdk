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
    /// NLS SDK 操作入口.
    /// </summary>

    public class NlsClient : INlsClient
    {
        #region Get the version of NLS SDK
        /// <summary>
        /// NLS SDK 当前版本信息.
        /// </summary>
        /// <returns></returns>
        public string GetVersion() 
        {
            IntPtr get = NativeMethods.NlsGetVersion();
            string version = "unknown version";
            if (get != IntPtr.Zero)
            {
                version = Marshal.PtrToStringAnsi(get);
            }
            return version;
        }
        #endregion

        #region Set the configure of log system
        /// <summary>
        /// 设置日志文件与存储路径.
        /// </summary>
        /// <param name="logOutputFile">
        /// 日志文件, 绝对路径或者相对路径均可. 若填写"log-transcriber", 则会在程序当前目录生成log-transcriber.log;
        /// 若填写"/home/XXX/NlsCppSdk/log-transcriber", 则会在/home/XXX/NlsCppSdk/下生成log-transcriber.log.
        /// </param>
        /// <param name="logLevel">
        /// 日志级别, 默认1 (LogError:1, LogWarning:2, LogInfo:3, LogDebug:4).
        /// </param>
        /// <param name="logFileSize">
        /// 日志文件的大小, 以MB为单位, 默认为10MB;如果日志文件内容的大小超过这个值,
        /// SDK会自动备份当前的日志文件, 超过后会循环覆盖已有文件.
        /// </param>
        /// <param name="logFileNum">
        /// 日志文件循环存储最大数, 默认10个文件.
        /// </param>
        /// <returns></returns>
        public int SetLogConfig(string logOutputFile, LogLevel logLevel, int logFileSize, int logFileNum)
        {
            return NativeMethods.NlsSetLogConfig(logOutputFile, (int)logLevel, logFileSize, logFileNum);
        }
        #endregion

        #region Start WorkThread (Init NLS SDK)
        /// <summary>
        /// 启动工作线程数量, 同时也是NLS SDK的初始化步骤.
        /// </summary>
        /// <param name="threadsNumber">
        /// 启动工作线程数量, 默认设置值为 1.
        /// threadsNumber 小于 0 则启动工作线程数量等于CPU核数.
        /// </param>
        /// <returns></returns>
        public void StartWorkThread(int threadsNumber = 1)
        {
            NativeMethods.NlsStartWorkThread(threadsNumber);
            return;
        }
        #endregion

        #region Release NLS SDK
        /// <summary>
        /// 销毁NlsClient对象实例, 即释放NLS SDK.
        /// </summary>
        /// <returns></returns>
        public void ReleaseInstance()
        {
            NativeMethods.NlsReleaseInstance();
            return;
        }
        #endregion

        #region Creata a request of speech transcriber
        /// <summary>
        /// 创建实时音频流识别对象.
        /// </summary>
        /// <returns>成功返回SpeechTranscriberRequest对象, 否则返回NULL.</returns>
        public SpeechTranscriberRequest CreateTranscriberRequest()
        {
            IntPtr request = NativeMethods.NlsCreateTranscriberRequest();
            SpeechTranscriberRequest STrequest = new SpeechTranscriberRequest();
            STrequest.native_request = request;
            return STrequest;
        }
        #endregion

        #region Release the request of speech transcriber
        /// <summary>
        /// 销毁实时音频流识别对象.
        /// </summary>
        /// <param name="request">
        /// CreateTranscriberRequest所建立的request对象.
        /// </param>
        /// <returns></returns>
        public void ReleaseTranscriberRequest(SpeechTranscriberRequest request)
        {
            NativeMethods.NlsReleaseTranscriberRequest(request.native_request);
            return;
        }
        #endregion

        #region Creata a request of speech recognizer
        /// <summary>
        /// 创建一句话识别对象.
        /// </summary>
        /// <returns>成功返回speechRecognizerRequest对象, 否则返回NULL.</returns>
        public SpeechRecognizerRequest CreateRecognizerRequest()
        {
            IntPtr request = NativeMethods.NlsCreateRecognizerRequest();
            SpeechRecognizerRequest SRrequest = new SpeechRecognizerRequest();
            SRrequest.native_request = request;
            return SRrequest;
        }
        #endregion

        #region Release the request of speech recognizer
        /// <summary>
        /// 销毁一句话识别对象.
        /// </summary>
        /// <param name="request">
        /// CreateRecognizerRequest所建立的request对象.
        /// </param>
        /// <returns></returns>
        public void ReleaseRecognizerRequest(SpeechRecognizerRequest request)
        {
            NativeMethods.NlsReleaseRecognizerRequest(request.native_request);
            return;
        }
        #endregion

        #region Creata a request of speech synthesizer
        /// <summary>
        /// 创建语音合成对象.
        /// </summary>
        /// <param name="version">
        /// tts类型, 短文本或长文本.
        /// </param>
        /// <returns></returns>
        public SpeechSynthesizerRequest CreateSynthesizerRequest(TtsVersion version)
        {
            IntPtr request = NativeMethods.NlsCreateSynthesizerRequest((int)version);
            SpeechSynthesizerRequest SYrequest = new SpeechSynthesizerRequest();
            SYrequest.native_request = request;
            return SYrequest;
        }
        #endregion

        #region Release the request of speech synthesizer
        /// <summary>
        /// 销毁语音合成对象.
        /// </summary>
        /// <param name="request">
        /// CreateSynthesizerRequest所建立的request对象.
        /// </param>
        /// <returns></returns>
        public void ReleaseSynthesizerRequest(SpeechSynthesizerRequest request)
        {
            NativeMethods.NlsReleaseSynthesizerRequest(request.native_request);
            return;
        }
        #endregion

        #region Create a NlsToken
        /// <summary>
        /// 创建Token获取对象.
        /// </summary>
        /// <returns>成功返回NlsToken对象, 否则返回NULL.</returns>
        public NlsToken CreateNlsToken()
        {
            IntPtr token = NativeMethods.NlsCreateNlsToken();
            NlsToken nlsToken = new NlsToken();
            nlsToken.native_token = token;
            return nlsToken;
        }
        #endregion

        #region Release the NlsToken
        /// <summary>
        /// 销毁Token获取对象.
        /// </summary>
        /// <param name="token">
        /// CreateNlsToken所建立的NlsToken对象.
        /// </param>
        /// <returns></returns>
        public void ReleaseNlsToken(NlsToken token)
        {
            NativeMethods.NlsReleaseNlsToken(token.native_token);
            return;
        }
        #endregion
    }
}
