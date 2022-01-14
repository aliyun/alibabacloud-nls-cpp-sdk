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
    public interface ISpeechTranscriber
    {
        /*
         * @brief 设置实时音频流识别服务URL地址
         * @note 必填参数. 默认为公网服务URL地址.
         * @param value 服务url字符串
         * @return 成功则返回0，否则返回-1
         */
        int SetUrl(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置appKey
         * @note 请参照官网
         * @param value appKey字符串
         * @return 成功则返回0，否则返回-1
         */
        int SetAppKey(SpeechTranscriberRequest request, string value);

        /*
         * @brief 口令认证。所有的请求都必须通过SetToken方法认证通过，才可以使用
         * @note token需要申请获取
         * @param value	申请的token字符串
         * @return 成功则返回0，否则返回-1
         */
        int SetToken(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置音频数据编码格式
         * @note  可选参数，目前支持pcm, opu. 默认是pcm
         * @param value	音频数据编码字符串
         * @return 成功则返回0，否则返回-1
         */
        int SetFormat(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置音频数据采样率sample_rate
         * @note 可选参数，目前支持16000, 8000. 默认是1600
         * @param value
         * @return 成功则返回0，否则返回-1
         */
        int SetSampleRate(SpeechTranscriberRequest request, int value);

        /*
         * @brief 设置是否返回中间识别结果
         * @note 可选参数. 默认false
         * @param value true 或 false
         * @return 成功则返回0，否则返回-1
         */
        int SetIntermediateResult(SpeechTranscriberRequest request, bool value);

        /*
         * @brief 设置是否在后处理中添加标点
         * @note 可选参数. 默认false
         * @param value true 或 false
         * @return 成功则返回0，否则返回-1
         */
        int SetPunctuationPrediction(SpeechTranscriberRequest request, bool value);

        /*
         * @brief 设置是否在后处理中执行数字转换
         * @note 可选参数. 默认false
         * @param value true 或 false
         * @return 成功则返回0，否则返回-1
         */
        int SetInverseTextNormalization(SpeechTranscriberRequest request, bool value);

        /*
         * @brief 设置是否使用语义断句
         * @note 可选参数. 默认false. 如果使用语义断句, 则vad断句设置不会生效.
         *                            两者为互斥关系.
         * @param value true 或 false
         * @return 成功则返回0，否则返回-1
         */
        //int SetSemanticSentenceDetection(bool value);

        /*
         * @brief 设置vad阀值
         * @note 可选参数. 静音时长超过该阈值会被认为断句,
         *                 合法参数范围200～2000(ms), 默认值800ms.
         *                 vad断句与语义断句为互斥关系, 不能同时使用.
         *                 调用此设置前,
         *                 请将语义断句setSemanticSentenceDetection设置为false.
         * @param value vad阀值
         * @return 成功则返回0，否则返回-1
         */
        int SetMaxSentenceSilence(SpeechTranscriberRequest request, int value);

        /*
         * @brief 设置定制模型
         * @param value 定制模型id字符串
         * @return 成功则返回0，否则返回-1
         */
        //int SetCustomizationId(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置泛热词
         * @param value 定制泛热词id字符串
         * @return 成功则返回0，否则返回-1
         */
        //int SetVocabularyId(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置Socket接收超时时间
         * @note
         * @param value 超时时间
         * @return 成功则返回0，否则返回-1
         */
        //int SetTimeout(SpeechTranscriberRequest request, int value);

        /*
         * @brief 设置是否开启nlp服务
         * @param value 编码格式 UTF-8 or GBK
         * @return 成功则返回0，否则返回-1
         */
        //int SetEnableNlp(SpeechTranscriberRequest request, bool enable);

        /*
         * @brief 设置nlp模型名称，开启NLP服务后必填
         * @param value 编码格式 UTF-8 or GBK
         * @return 成功则返回0，否则返回-1
         */
        //int SetNlpModel(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置session id
         * @note  用于请求异常断开重连时，服务端识别是同一个会话。
         * @param value session id 字符串
         * @return 成功则返回0，否则返回-1
         */
        //int SetSessionId(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置输出文本的编码格式
         * @note
         * @param value 编码格式 UTF-8 or GBK
         * @return 成功则返回0，否则返回-1
         */
        //int SetOutputFormat(SpeechTranscriberRequest request, string value);

        /*
         * @brief 参数设置
         * @note  暂不对外开放
         * @param value 参数
         * @return 成功则返回0，否则返回-1
         */
        //int SetPayloadParam(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置用户自定义参数
         * @param value 参数
         * @return 成功则返回0，否则返回-1
         */
        //int SetContextParam(SpeechTranscriberRequest request, string value);

        /*
         * @brief 设置用户自定义ws阶段http header参数
         * @param key 参数名称
         * @param value 参数内容
         * @return 成功则返回0，否则返回-1
         */
        //int AppendHttpHeaderParam(SpeechTranscriberRequest request, string key, string value);

        /*
         * @brief 启动实时音频流识别
         * @note 异步操作。成功返回started事件。失败返回TaskFailed事件。
         * @return 成功则返回0，否则返回-1
         */
        int Start(SpeechTranscriberRequest request);

        /*
         * @brief 要求服务端更新识别参数
         * @note 异步操作。失败返回TaskFailed。
         * @return 成功则返回0，否则返回-1
         */
        //int Control(SpeechTranscriberRequest request, string message);

        /*
         * @brief 会与服务端确认关闭，正常停止实时音频流识别操作
         * @note 异步操作。失败返回TaskFailed。
         * @return 成功则返回0，否则返回-1
         */
        int Stop(SpeechTranscriberRequest request);

        /*
         * @brief 直接关闭实时音频流识别过程
         * @note 调用cancel之后不会在上报任何回调事件
         * @return 成功则返回0，否则返回-1
         */
        int Cancel(SpeechTranscriberRequest request);

        /*
         * @brief 发送语音数据
         * @note 异步操作。request
         * @param data 语音数据
         * @param dataSize 语音数据长度(建议每次100ms左右数据)
         * @param type ENCODER_NONE 表示原始音频进行传递,
                                    建议每次100ms音频数据,支持16K和8K;
                       ENCODER_OPU 表示以定制OPUS压缩后进行传递,
                                   只支持20ms 16K16b1c
                       ENCODER_OPUS 表示以OPUS压缩后进行传递,
                                    只支持20ms, 支持16K16b1c和8K16b1c
         * @return 成功则返回0，失败返回-1。
                   由于音频格式不确定，传入音频字节数和传出音频字节数
                   无法通过比较判断成功与否，故成功返回0。
         */
        int SendAudio(SpeechTranscriberRequest request, byte[] data, UInt64 dataSize, EncoderType type);

        /*
         * @brief 设置错误回调函数
         * @note 在请求过程中出现异常错误时，sdk内部线程上报该回调。
         *       用户可以在事件的消息头中检查状态码和状态消息，以确认失败的具体原因.
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        void SetOnTaskFailed(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);

        /*
         * @brief 设置实时音频流识别开始回调函数
         * @note 服务端就绪、可以开始识别时，sdk内部线程上报该回调.
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        void SetOnTranscriptionStarted(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);

        /*
         * @brief 设置一句话开始回调
         * @note 检测到一句话的开始时, sdk内部线程上报该回调.
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        void SetOnSentenceBegin(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);

        /*
         * @brief 设置实时音频流识别中间结果回调函数
         * @note 设置enable_intermediate_result字段为true，才会有中间结果.
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        void SetOnTranscriptionResultChanged(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);

        /*
         * @brief 设置一句话结束回调函数
         * @note 检测到了一句话的结束时, sdk内部线程上报该回调.
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        void SetOnSentenceEnd(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);

        /*
         * @brief 设置服务端结束服务回调函数
         * @note 云端结束实时音频流识别服务时, sdk内部线程上报该回调.
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        void SetOnTranscriptionCompleted(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);

        /*
         * @brief 设置通道关闭回调函数
         * @note 在请求过程中通道关闭时，sdk内部线程上报该回调.
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        void SetOnChannelClosed(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);

        /*
         * @brief 设置二次处理结果回调函数
         * @note 表示对实时转写的原始结果进行处理后的结果, 开启enable_nlp后返回
         * @param _event 回调方法
         * @param para 用户传入参数, 默认为NULL
         * @return void
         */
        //void SetOnSentenceSemantics(SpeechTranscriberRequest request, CallbackDelegate callback, object para = null);
    }
}
