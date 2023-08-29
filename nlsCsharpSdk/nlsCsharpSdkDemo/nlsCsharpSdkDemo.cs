using nlsCsharpSdk;
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Windows.Forms;

namespace nlsCsharpSdkDemo
{
    public struct RunParams
    {
        public bool send_audio_flag;
        public bool audio_loop_flag;
    };

    public struct DemoSpeechTranscriberStruct
    {
        public SpeechTranscriberRequest stPtr;
        public Thread st_send_audio;
        public string uuid;
    };

    public struct DemoSpeechRecognizerStruct
    {
        public SpeechRecognizerRequest srPtr;
        public Thread sr_send_audio;
        public string uuid;
    };

    public struct DemoSpeechSynthesizerStruct
    {
        public SpeechSynthesizerRequest syPtr;
        public Thread sy_send_audio;
        public string uuid;
    };

    public partial class nlsCsharpSdkDemo : Form
    {
        private NlsClient nlsClient;
        private static Dictionary<string, RunParams> globalRunParams = new Dictionary<string, RunParams>();
        private LinkedList<DemoSpeechTranscriberStruct> stList = null;
        private LinkedList<DemoSpeechRecognizerStruct> srList = null;
        private LinkedList<DemoSpeechSynthesizerStruct> syList = null;
        private NlsToken tokenPtr;
        private UInt64 expireTime;

        private string appKey;
        private string akId;
        private string akSecret;
        private string token;
        private string url;

        static int max_concurrency_num = 200;  /* 可设置的最大并发数 */
        static bool running;  /* 刷新Label的flag */

        static string cur_st_result;
        static string cur_st_completed;
        static string cur_st_closed;
        static int st_concurrency_number = 1;
        static string cur_st_file_path = System.Environment.CurrentDirectory + @"\audio_files\test3.wav";

        static string cur_sr_result;
        static string cur_sr_completed;
        static string cur_sr_closed;
        static int sr_concurrency_number = 1;
        static string cur_sr_file_path = System.Environment.CurrentDirectory + @"\audio_files\test1.wav";

        static string cur_sy_completed;
        static string cur_sy_closed;
        static int sy_concurrency_number = 1;
        static string cur_sy_output;

        static string fileLinkUrl = "https://gw.alipayobjects.com/os/bmw-prod/0574ee2e-f494-45a5-820f-63aee583045a.wav";

        public nlsCsharpSdkDemo()
        {
            InitializeComponent();
            System.Windows.Forms.Control.CheckForIllegalCrossThreadCalls = false;/* 设置该属性为false */

            nlsClient = new NlsClient();
        }

        #region ShowTextIntoLab
        private void FlushLab()
        {
            while (running)
            {
                if (cur_st_result != null && cur_st_result.Length > 0)
                {
                    stResult.Text = cur_st_result;
                }
                if (cur_st_completed != null && cur_st_completed.Length > 0)
                {
                    stCompleted.Text = cur_st_completed;
                }
                if (cur_st_closed != null && cur_st_closed.Length > 0)
                {
                    stClosed.Text = cur_st_closed;
                }

                if (cur_sr_result != null && cur_sr_result.Length > 0)
                {
                    srResult.Text = cur_sr_result;
                }
                if (cur_sr_completed != null && cur_sr_completed.Length > 0)
                {
                    srCompleted.Text = cur_sr_completed;
                }
                if (cur_sr_closed != null && cur_sr_closed.Length > 0)
                {
                    srClosed.Text = cur_sr_closed;
                }

                if (cur_sy_completed != null && cur_sy_completed.Length > 0)
                {
                    syCompleted.Text = cur_sy_completed;
                }
                if (cur_sy_closed != null && cur_sy_closed.Length > 0)
                {
                    syClosed.Text = cur_sy_closed;
                }
                if (cur_sy_output != null && cur_sy_output.Length > 0)
                {
                    syOutput.Text = cur_sy_output;
                }
                Thread.Sleep(200);
            }
        }
        #endregion

        #region SendAudioDataIntoNlsSDK
        /// <summary>
        /// 实时识别的音频推送线程.
        /// </summary>
        private void STAudioLab(object request)
        {
            DemoSpeechTranscriberStruct st_node = (DemoSpeechTranscriberStruct)request;
            System.Diagnostics.Debug.WriteLine("st audio file_name = {0}", cur_st_file_path);
            FileStream fs = new FileStream(cur_st_file_path, FileMode.Open, FileAccess.Read);
            if (fs.Length <= 0)
            {
                nlsResult.Text = "open" + cur_st_file_path + "failed";
                return;
            }
            BinaryReader br = new BinaryReader(fs);

            /*
             * 通过编号uuid获取对应的状态机，uuid在创建此语音请求时生成
             */
            bool cur_audio_loop_flag = globalRunParams[st_node.uuid].audio_loop_flag;
            bool cur_send_audio_flag = globalRunParams[st_node.uuid].send_audio_flag;

            while (cur_audio_loop_flag)
            {
                if (cur_send_audio_flag)
                {
                    byte[] byData = br.ReadBytes((int)3200);
                    if (byData.Length > 0)
                    {
                        /*
                         * 推送byData.Lenght字节PCM格式音频数据到SDK进行识别
                         */
                        st_node.stPtr.SendAudio(st_node.stPtr, byData, (UInt64)byData.Length, EncoderType.ENCODER_PCM);
                    }
                    else
                    {
                        /*
                         * 音频推送完成,重新打开循环继续
                         */
                        br.Close();
                        fs.Dispose();
                        fs = new FileStream(cur_st_file_path, FileMode.Open, FileAccess.Read);
                        br = new BinaryReader(fs);
                    }
                }
                else
                {
                }
                /*
                 * 上面推送3200字节音频数据，相当于模拟100MS的音频
                 */
                Thread.Sleep(100);

                /*
                 * 更新状态机
                 */
                if (globalRunParams.ContainsKey(st_node.uuid))
                {
                    cur_audio_loop_flag = globalRunParams[st_node.uuid].audio_loop_flag;
                    cur_send_audio_flag = globalRunParams[st_node.uuid].send_audio_flag;
                }
                else
                {
                    cur_audio_loop_flag = false;
                    cur_send_audio_flag = false;
                }
            }
            br.Close();
            fs.Dispose();
        }

        /// <summary>
        /// 一句话识别的音频推送线程.
        /// </summary>
        private void SRAudioLab(object request)
        {
            DemoSpeechRecognizerStruct sr_node = (DemoSpeechRecognizerStruct)request;
            System.Diagnostics.Debug.WriteLine("sr audio file_name = {0}", cur_sr_file_path);
            FileStream fs = new FileStream(cur_sr_file_path, FileMode.Open, FileAccess.Read);
            if (fs.Length <= 0)
            {
                nlsResult.Text = "open" + cur_sr_file_path + "failed";
                return;
            }
            BinaryReader br = new BinaryReader(fs);

            /*
             * 通过编号uuid获取对应的状态机，uuid在创建此语音请求时生成
             */
            bool cur_audio_loop_flag = globalRunParams[sr_node.uuid].audio_loop_flag;
            bool cur_send_audio_flag = globalRunParams[sr_node.uuid].send_audio_flag;

            while (cur_audio_loop_flag)
            {
                if (cur_send_audio_flag)
                {
                    byte[] byData = br.ReadBytes((int)3200);
                    if (byData.Length > 0)
                    {
                        /*
                         * 推送byData.Lenght字节PCM格式音频数据到SDK进行识别
                         */
                        sr_node.srPtr.SendAudio(sr_node.srPtr, byData, (UInt64)byData.Length, EncoderType.ENCODER_PCM);
                    }
                    else
                    {
                        /*
                         * 音频推送完成,重新打开循环继续
                         */
                        br.Close();
                        fs.Dispose();
                        fs = new FileStream(cur_sr_file_path, FileMode.Open, FileAccess.Read);
                        br = new BinaryReader(fs);
                    }
                }
                else
                {
                }
                /*
                 * 上面推送3200字节音频数据，相当于模拟100MS的音频
                 */
                Thread.Sleep(100);

                /*
                 * 更新状态机
                 */
                if (globalRunParams.ContainsKey(sr_node.uuid))
                {
                    cur_audio_loop_flag = globalRunParams[sr_node.uuid].audio_loop_flag;
                    cur_send_audio_flag = globalRunParams[sr_node.uuid].send_audio_flag;
                }
                else
                {
                    cur_audio_loop_flag = false;
                    cur_send_audio_flag = false;
                }
            }
            br.Close();
            fs.Dispose();
        }
        #endregion

        #region NlsSdkButton
        // open log
        private void button1_Click(object sender, EventArgs e)
        {
            int ret = nlsClient.SetLogConfig("nlsLog", LogLevel.LogDebug, 400, 10);
            if (ret == 0)
                nlsResult.Text = "OpenLog Success";
            else
                nlsResult.Text = "OpenLog Failed";
        }

        // get sdk version
        private void button1_Click_1(object sender, EventArgs e)
        {
            string version = nlsClient.GetVersion();
            nlsResult.Text = version;
        }

        // start sdk workThread
        private void button1_Click_2(object sender, EventArgs e)
        {
            /* 设置套接口地址类型, 需要在StartWorkThread前调用, 默认可不调用此接口 */
            //nlsClient.SetAddrInFamily("AF_INET4");

            /*
             * 启动1个事件池。在多并发(上百并发)情况下，建议选择 4 。若单并发，则建议填写 1 。
             */
            nlsClient.StartWorkThread(1);
            nlsResult.Text = "StartWorkThread and init NLS success.";
            running = true;
            /*
             * 启动线程FlushLab，用于将一些text显示到UI上
             */
            Thread t = new Thread(FlushLab);
            t.Start();

            btnInitNls.Enabled = false;
            btnDeinitNls.Enabled = true;
            btnCreateToken.Enabled = true;
            btnReleaseToken.Enabled = false;

            btnSTcreate.Enabled = true;
            btnSTstart.Enabled = false;
            btnSTstop.Enabled = false;
            btnSTrelease.Enabled = false;
            btnSRcreate.Enabled = true;
            btnSRstart.Enabled = false;
            btnSRstop.Enabled = false;
            btnSRrelease.Enabled = false;
            btnSYcreate.Enabled = true;
            btnSYstart.Enabled = false;
            btnSYcancel.Enabled = false;
            btnSYrelease.Enabled = false;
            button1.Enabled = true;
        }

        // release sdk
        private void button1_Click_3(object sender, EventArgs e)
        {
            nlsClient.ReleaseInstance();
            nlsResult.Text = "Release NLS success.";

            btnInitNls.Enabled = true;
            btnDeinitNls.Enabled = false;
            btnCreateToken.Enabled = false;
            btnReleaseToken.Enabled = false;

            btnSTcreate.Enabled = false;
            btnSTstart.Enabled = false;
            btnSTstop.Enabled = false;
            btnSTrelease.Enabled = false;
            btnSRcreate.Enabled = false;
            btnSRstart.Enabled = false;
            btnSRstop.Enabled = false;
            btnSRrelease.Enabled = false;
            btnSYcreate.Enabled = false;
            btnSYstart.Enabled = false;
            btnSYcancel.Enabled = false;
            btnSYrelease.Enabled = false;
            button1.Enabled = false;
        }
        #endregion

        #region FillInUserInfo
        private void textBox1_TextChanged(object sender, EventArgs e)
        {
            akId = tAkId.Text;
        }

        private void tAppKey_TextChanged(object sender, EventArgs e)
        {
            appKey = tAppKey.Text;
        }

        private void tAkSecret_TextChanged(object sender, EventArgs e)
        {
            akSecret = tAkSecret.Text;
        }

        private void tToken_TextChanged(object sender, EventArgs e)
        {
            token = tToken.Text;
        }

        private void tUrl_TextChanged(object sender, EventArgs e)
        {
            url = tUrl.Text;
        }

        private void tInput1_TextChanged(object sender, EventArgs e)
        {
            cur_st_file_path = tInput1.Text;
        }

        private void tinput2_TextChanged(object sender, EventArgs e)
        {
            cur_sr_file_path = tinput2.Text;
        }
        #endregion

        #region TokenButton
        // create token
        private void button3_Click_1(object sender, EventArgs e)
        {
            int ret = -1;
            tokenPtr = nlsClient.CreateNlsToken();
            if (tokenPtr.native_token != IntPtr.Zero)
            {
                if (akId == null || akId.Length == 0)
                {
                    akId = tAkId.Text;
                }
                if (akSecret == null || akSecret.Length == 0)
                {
                    akSecret = tAkSecret.Text;
                }

                if (akId != null && akSecret != null && akId.Length > 0 && akSecret.Length > 0)
                {
                    tokenPtr.SetAccessKeyId(tokenPtr, akId);
                    tokenPtr.SetKeySecret(tokenPtr, akSecret);

                    ret = tokenPtr.ApplyNlsToken(tokenPtr);
                    if (ret < 0)
                    {
                        System.Diagnostics.Debug.WriteLine("ApplyNlsToken failed");
                        nlsResult.Text = tokenPtr.GetErrorMsg(tokenPtr);
                    }
                    else
                    {
                        System.Diagnostics.Debug.WriteLine("ApplyNlsToken success");

                        btnCreateToken.Enabled = false;
                        btnReleaseToken.Enabled = true;

                        token = tokenPtr.GetToken(tokenPtr);
                        tToken.Text = token;
                        expireTime = tokenPtr.GetExpireTime(tokenPtr);
                        nlsResult.Text = "ExpireTime:" + expireTime.ToString();
                    }
                }
                else
                {
                    nlsResult.Text = "CreateToken Failed, akId or Secret is null";
                }
            }
            else
            {
                nlsResult.Text = "CreateToken Failed";
            }
        }

        // release token
        private void button4_Click(object sender, EventArgs e)
        {
            if (tokenPtr.native_token != IntPtr.Zero)
            {
                nlsClient.ReleaseNlsToken(tokenPtr);
                tokenPtr.native_token = IntPtr.Zero;
                nlsResult.Text = "ReleaseNlsToken Success";
            }
            else
            {
                nlsResult.Text = "ReleaseNlsToken is nullptr";
            }
        }
        #endregion

        #region FillInConcurrencyNumber
        private void textBox1_TextChanged_1(object sender, EventArgs e)
        {
            st_concurrency_number = Convert.ToInt32(textBox1.Text);
            if (st_concurrency_number < 1)
            {
                st_concurrency_number = 1;
                textBox1.Text = "1";
            }
            else if (st_concurrency_number > max_concurrency_num)
            {
                st_concurrency_number = max_concurrency_num;
                textBox1.Text = Convert.ToString(max_concurrency_num);
            }
        }

        private void textBox2_TextChanged(object sender, EventArgs e)
        {
            sr_concurrency_number = Convert.ToInt32(textBox2.Text);
            if (sr_concurrency_number < 1)
            {
                sr_concurrency_number = 1;
                textBox2.Text = "1";
            }
            else if (sr_concurrency_number > max_concurrency_num)
            {
                sr_concurrency_number = max_concurrency_num;
                textBox2.Text = Convert.ToString(max_concurrency_num);
            }
        }

        private void textBox3_TextChanged(object sender, EventArgs e)
        {
            sy_concurrency_number = Convert.ToInt32(textBox3.Text);
            if (sy_concurrency_number < 1)
            {
                sy_concurrency_number = 1;
                textBox3.Text = "1";
            }
            else if (sy_concurrency_number > max_concurrency_num)
            {
                sy_concurrency_number = max_concurrency_num;
                textBox3.Text = Convert.ToString(max_concurrency_num);
            }
        }
        #endregion

        #region TranscriberCallback
        public CallbackDelegate DemoOnTranscriptionStarted =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionStarted user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionStarted msg = {0}", msg);
                cur_st_completed = "msg : " + msg;

                RunParams demo_params = new RunParams();
                demo_params.send_audio_flag = true;
                demo_params.audio_loop_flag = globalRunParams[uuid].audio_loop_flag;

                globalRunParams.Remove(uuid);
                globalRunParams.Add(uuid, demo_params);
            };
        public CallbackDelegate DemoOnTranscriptionClosed =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionClosed user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionClosed msg = {0}", msg);
                cur_st_closed = "msg : " + msg;
            };
        public CallbackDelegate DemoOnTranscriptionTaskFailed =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionTaskFailed user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionTaskFailed msg = {0}", msg);
                cur_st_completed = "msg : " + msg;

                RunParams demo_params = new RunParams();
                demo_params.send_audio_flag = false;
                demo_params.audio_loop_flag = false;

                globalRunParams.Remove(uuid);
                globalRunParams.Add(uuid, demo_params);
            };
        private CallbackDelegate DemoOnTranscriptionResultChanged =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionResultChanged user uuid = {0}", uuid);
                string result = System.Text.Encoding.Default.GetString(e.result).TrimEnd('\0');
                //System.Diagnostics.Debug.WriteLine("DemoOnTranscriptionResultChanged result = {0}", result);
                cur_st_result = "middle result : " + result;
            };
        private CallbackDelegate DemoOnSentenceBegin =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSentenceBegin user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnSentenceBegin msg = {0}", msg);
                cur_st_completed = "sentenceBegin : " + msg;
            };
        private CallbackDelegate DemoOnSentenceEnd =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSentenceEnd user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnSentenceEnd msg = {0}", msg);
                cur_st_completed = "sentenceEnd : " + msg;
            };
        #endregion

        #region TranscriberButton
        // create transcriber
        private void button1_Click_4(object sender, EventArgs e)
        {
            if (stList == null)
            {
                stList = new LinkedList<DemoSpeechTranscriberStruct>();
            }
            else
            {
                nlsResult.Text = "transcriber list is existed, release first...";
            }

            for (int i = 0; i < st_concurrency_number; i++)
            {
                DemoSpeechTranscriberStruct stStruct;
                stStruct = new DemoSpeechTranscriberStruct();
                stStruct.stPtr = nlsClient.CreateTranscriberRequest();
                if (stStruct.stPtr.native_request != IntPtr.Zero)
                {
                    nlsResult.Text = "CreateTranscriberRequest Success";
                    btnSTcreate.Enabled = false;
                    btnSTstart.Enabled = true;
                    btnSTrelease.Enabled = true;

                    stStruct.uuid = System.Guid.NewGuid().ToString("N");
                    RunParams demo_params = new RunParams();
                    demo_params.send_audio_flag = false;
                    demo_params.audio_loop_flag = false;
                    globalRunParams[stStruct.uuid] = demo_params;
                }
                else
                {
                    nlsResult.Text = "CreateTranscriberRequest Failed";
                }
                stList.AddLast(stStruct);
            }

            cur_st_result = "null";
            cur_st_closed = "null";
            cur_st_completed = "null";
        }

        // release transcriber
        private void button2_Click(object sender, EventArgs e)
        {
            if (stList == null)
            {
                nlsResult.Text = "transcriber list is null, create first...";
                return;
            }
            else
            {
                int st_count = stList.Count;
                for (int i = 0; i < st_count; i++)
                {
                    LinkedListNode<DemoSpeechTranscriberStruct> stStruct = stList.Last;
                    DemoSpeechTranscriberStruct st = stStruct.Value;
                    if (st.stPtr.native_request != IntPtr.Zero)
                    {
                        nlsClient.ReleaseTranscriberRequest(st.stPtr);
                        st.stPtr.native_request = IntPtr.Zero;
                        globalRunParams.Remove(st.uuid);

                        nlsResult.Text = "ReleaseTranscriberRequest Success";
                        btnSTrelease.Enabled = false;
                        btnSTcreate.Enabled = true;
                        btnSTstart.Enabled = false;
                        btnSTstop.Enabled = false;
                    }
                    else
                    {
                        nlsResult.Text = "TranscriberRequest is nullptr";
                    }
                    stList.RemoveLast();
                }
                stList.Clear();
            }

            cur_st_result = "null";
            cur_st_closed = "null";
            cur_st_completed = "null";
        }

        // start transcriber
        private void button3_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (stList == null)
            {
                nlsResult.Text = "transcriber list is null, create first...";
                return;
            }
            else
            {
                if (File.Exists(cur_st_file_path))
                {
                }
                else
                {
                    cur_st_completed = "cannot open file: " + cur_st_file_path;
                    return;
                }

                LinkedListNode<DemoSpeechTranscriberStruct> stStruct = stList.First;
                int st_count = stList.Count;
                for (int i = 0; i < st_count; i++)
                {
                    DemoSpeechTranscriberStruct st = stStruct.Value;
                    if (st.stPtr.native_request != IntPtr.Zero)
                    {
                        if (appKey == null || appKey.Length == 0)
                        {
                            appKey = tAppKey.Text;
                        }
                        if (token == null || token.Length == 0)
                        {
                            token = tToken.Text;
                        }
                        if (appKey == null || token == null ||
                            appKey.Length == 0 || token.Length == 0)
                        {
                            nlsResult.Text = "Start failed, token or appkey is empty";
                            return;
                        }

                        st.stPtr.SetAppKey(st.stPtr, appKey);
                        st.stPtr.SetToken(st.stPtr, token);
                        st.stPtr.SetUrl(st.stPtr, url);
                        st.stPtr.SetFormat(st.stPtr, "pcm");
                        st.stPtr.SetSampleRate(st.stPtr, 16000);
                        st.stPtr.SetIntermediateResult(st.stPtr, true);
                        st.stPtr.SetPunctuationPrediction(st.stPtr, true);
                        st.stPtr.SetInverseTextNormalization(st.stPtr, true);

                        // 此处仅仅只是用unix时间戳作为每轮对话的session id
                        Int32 unixTimestamp = (Int32)(DateTime.UtcNow.Subtract(new DateTime(1970, 1, 1))).TotalSeconds;
                        st.stPtr.SetSessionId(st.stPtr, unixTimestamp.ToString());

                        st.stPtr.SetOnTranscriptionStarted(st.stPtr, DemoOnTranscriptionStarted, st.uuid);
                        st.stPtr.SetOnChannelClosed(st.stPtr, DemoOnTranscriptionClosed, st.uuid);
                        st.stPtr.SetOnTaskFailed(st.stPtr, DemoOnTranscriptionTaskFailed, st.uuid);
                        st.stPtr.SetOnSentenceBegin(st.stPtr, DemoOnSentenceBegin, st.uuid);
                        st.stPtr.SetOnSentenceEnd(st.stPtr, DemoOnSentenceEnd, st.uuid);
                        st.stPtr.SetOnTranscriptionResultChanged(st.stPtr, DemoOnTranscriptionResultChanged, st.uuid);

                        ret = st.stPtr.Start(st.stPtr);
                        if (ret != 0)
                        {
                            nlsResult.Text = "Transcriber Start failed";
                        }
                        else
                        {
                            if (globalRunParams[st.uuid].audio_loop_flag == false)
                            {
                                RunParams demo_params = new RunParams();
                                demo_params.audio_loop_flag = true;
                                demo_params.send_audio_flag = globalRunParams[st.uuid].send_audio_flag;

                                globalRunParams.Remove(st.uuid);
                                globalRunParams.Add(st.uuid, demo_params);

                                st.st_send_audio = new Thread(new ParameterizedThreadStart(STAudioLab));
                                st.st_send_audio.Start((object)st);

                                btnSTstart.Enabled = false;
                                btnSTstop.Enabled = true;
                            }

                            nlsResult.Text = "Transcriber Start success";
                        }
                    }
                    else
                    {
                    }
                    stStruct = stStruct.Next;
                    if (stStruct == null)
                    {
                        break;
                    }
                }
            }
        }

        // stop transcriber
        private void btnSTstop_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (stList == null)
            {
                nlsResult.Text = "transcriber list is null, create first...";
                return;
            }
            else
            {
                LinkedListNode<DemoSpeechTranscriberStruct> stStruct = stList.First;
                int st_count = stList.Count;
                for (int i = 0; i < st_count; i++)
                {
                    DemoSpeechTranscriberStruct st = stStruct.Value;
                    if (st.stPtr.native_request != IntPtr.Zero)
                    {
                        if (st.stPtr.native_request != IntPtr.Zero)
                        {
                            RunParams demo_params = new RunParams();
                            demo_params.audio_loop_flag = globalRunParams[st.uuid].audio_loop_flag;
                            demo_params.send_audio_flag = false;

                            globalRunParams.Remove(st.uuid);
                            globalRunParams.Add(st.uuid, demo_params);

                            ret = st.stPtr.Stop(st.stPtr);
                        }

                        if (ret != 0)
                        {
                            nlsResult.Text = "Transcriber Stop failed";
                        }
                        else
                        {
                            btnSTstop.Enabled = false;
                            nlsResult.Text = "Transcriber Stop success";
                        }
                    }
                    else
                    {
                    }
                    stStruct = stStruct.Next;
                    if (stStruct == null)
                    {
                        break;
                    }
                }
            }
        }
        #endregion

        #region SynthesizerCallback
        private CallbackDelegate DemoOnBinaryDataReceived =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnBinaryDataReceived uuid = {0}", uuid);
                System.Diagnostics.Debug.WriteLine("DemoOnBinaryDataReceived taskId = {0}", e.taskId);
                System.Diagnostics.Debug.WriteLine("DemoOnBinaryDataReceived dataSize = {0}", e.binaryDataSize);
                //cur_sy_completed = e.taskId + ", binaryDataSize : " + e.binaryDataSize;

                /*
                 * 特殊注意：此回调频率较高，需要谨慎处理对收到音频数据的转存。
                 * 如下写入音频，容易出现多次回调同一时间同时操作同一个fs的问题。
                 */
#if TRUE
                FileStream fs;
                string filename = e.taskId + ".wav";
                cur_sy_output = System.Environment.CurrentDirectory + @"\" + filename;
                System.Diagnostics.Debug.WriteLine("DemoOnBinaryDataReceived current filename = {0}", filename);
                if (File.Exists(filename))
                {
                    fs = new FileStream(filename, FileMode.Append, FileAccess.Write);
                }
                else
                {
                    fs = new FileStream(filename, FileMode.Create, FileAccess.Write);
                }
                fs.Write(e.binaryData, 0, e.binaryDataSize);
                fs.Close();
#endif
            };
        private CallbackDelegate DemoOnSynthesisClosed =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisClosed user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisClosed msg = {0}", msg);
                cur_sy_closed = "msg : " + msg;
            };
        private CallbackDelegate DemoOnSynthesisTaskFailed =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisTaskFailed user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisTaskFailed msg = {0}", msg);
                cur_sy_completed = "msg : " + msg;
            };
        private CallbackDelegate DemoOnSynthesisCompleted =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisCompleted user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnSynthesisCompleted msg = {0}", msg);
                cur_sy_completed = "result : " + msg;
            };
        private CallbackDelegate DemoOnMetaInfo =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnMetaInfo user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnMetaInfo msg = {0}", msg);
                //cur_sy_completed = "metaInfo : " + e.msg;
            };
        #endregion

        #region SynthesizerButton
        // create synthesizer
        private void button12_Click(object sender, EventArgs e)
        {
            if (syList == null)
            {
                syList = new LinkedList<DemoSpeechSynthesizerStruct>();
            }
            else
            {
                nlsResult.Text = "synthesizer list is existed, release first...";
            }

            for (int i = 0; i < sy_concurrency_number; i++)
            {
                /*
                 * 创建一个用户定义的结构体，传入SDK，后续可在回调中获取
                 */
                DemoSpeechSynthesizerStruct syStruct;
                syStruct = new DemoSpeechSynthesizerStruct();
                syStruct.syPtr = nlsClient.CreateSynthesizerRequest(TtsVersion.ShortTts);
                if (syStruct.syPtr.native_request != IntPtr.Zero)
                {
                    nlsResult.Text = "CreateSynthesizerRequest Success";
                    btnSYcreate.Enabled = false;
                    btnSYstart.Enabled = true;
                    btnSYrelease.Enabled = true;

                    /*
                     * 结构体RunParams中创建uuid，用于状态机与此次请求的一一对应
                     */
                    syStruct.uuid = System.Guid.NewGuid().ToString("N");
                    RunParams demo_params = new RunParams();
                    demo_params.send_audio_flag = false;
                    demo_params.audio_loop_flag = false;
                    globalRunParams[syStruct.uuid] = demo_params;
                }
                else
                {
                    nlsResult.Text = "CreateSynthesizerRequest Failed";
                }
                syList.AddLast(syStruct);
            }

            cur_sy_closed = "null";
            cur_sy_completed = "null";
        }

        // start synthesizer
        private void button10_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (syList == null)
            {
                nlsResult.Text = "synthesizer list is null, create first...";
                return;
            }
            else
            {
                LinkedListNode<DemoSpeechSynthesizerStruct> syStruct = syList.First;
                int sy_count = syList.Count;
                for (int i = 0; i < sy_count; i++)
                {
                    DemoSpeechSynthesizerStruct sy = syStruct.Value;
                    if (sy.syPtr.native_request != IntPtr.Zero)
                    {
                        if (appKey == null || appKey.Length == 0)
                        {
                            appKey = tAppKey.Text;
                        }
                        if (token == null || token.Length == 0)
                        {
                            token = tToken.Text;
                        }
                        if (appKey == null || token == null ||
                            appKey.Length == 0 || token.Length == 0)
                        {
                            nlsResult.Text = "Start failed, token or appkey is empty";
                            return;
                        }

                        string text = "今天天气真不错，我想去操场踢足球。";
                        sy.syPtr.SetAppKey(sy.syPtr, appKey);
                        sy.syPtr.SetToken(sy.syPtr, token);
                        sy.syPtr.SetUrl(sy.syPtr, url);
                        sy.syPtr.SetText(sy.syPtr, text);
                        sy.syPtr.SetVoice(sy.syPtr, "siqi");
                        sy.syPtr.SetVolume(sy.syPtr, 50);
                        sy.syPtr.SetFormat(sy.syPtr, "wav");
                        sy.syPtr.SetSampleRate(sy.syPtr, 16000);
                        sy.syPtr.SetSpeechRate(sy.syPtr, 0);
                        sy.syPtr.SetPitchRate(sy.syPtr, 0);
                        sy.syPtr.SetEnableSubtitle(sy.syPtr, true);

                        sy.syPtr.SetOnSynthesisCompleted(sy.syPtr, DemoOnSynthesisCompleted, sy.uuid);
                        sy.syPtr.SetOnBinaryDataReceived(sy.syPtr, DemoOnBinaryDataReceived, sy.uuid);
                        sy.syPtr.SetOnTaskFailed(sy.syPtr, DemoOnSynthesisTaskFailed, sy.uuid);
                        sy.syPtr.SetOnChannelClosed(sy.syPtr, DemoOnSynthesisClosed, sy.uuid);
                        sy.syPtr.SetOnMetaInfo(sy.syPtr, DemoOnMetaInfo, sy.uuid);

                        ret = sy.syPtr.Start(sy.syPtr);
                        if (ret != 0)
                        {
                            nlsResult.Text = "Synthesizer Start failed.";
                        }
                        else
                        {
                            nlsResult.Text = "Transcriber Start success.";
                            btnSYstart.Enabled = false;
                            btnSYcancel.Enabled = true;
                        }
                    }
                    else
                    {
                    }
                    syStruct = syStruct.Next;
                    if (syStruct == null)
                    {
                        break;
                    }
                }
            }
        }

        // cancel synthesizer
        private void button9_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (syList == null)
            {
                nlsResult.Text = "synthesizer list is null, create first...";
                return;
            }
            else
            {
                LinkedListNode<DemoSpeechSynthesizerStruct> syStruct = syList.First;
                int sy_count = syList.Count;
                for (int i = 0; i < sy_count; i++)
                {
                    DemoSpeechSynthesizerStruct sy = syStruct.Value;
                    if (sy.syPtr.native_request != IntPtr.Zero)
                    {
                        if (sy.syPtr.native_request != IntPtr.Zero)
                        {
                            ret = sy.syPtr.Cancel(sy.syPtr);
                        }

                        if (ret != 0)
                        {
                            nlsResult.Text = "Synthesizer Cancel failed";
                        }
                        else
                        {
                            btnSYcancel.Enabled = false;
                            nlsResult.Text = "Synthesizer Cancel success";
                        }
                    }
                    else
                    {
                    }
                    syStruct = syStruct.Next;
                    if (syStruct == null)
                    {
                        break;
                    }
                }
            }
        }

        // release synthesizer
        private void btnSYrelease_Click(object sender, EventArgs e)
        {
            if (syList == null)
            {
                nlsResult.Text = "synthesizer list is null, create first...";
                return;
            }
            else
            {
                int sy_count = syList.Count;
                for (int i = 0; i < sy_count; i++)
                {
                    LinkedListNode<DemoSpeechSynthesizerStruct> syStruct = syList.Last;
                    DemoSpeechSynthesizerStruct sy = syStruct.Value;
                    if (sy.syPtr.native_request != IntPtr.Zero)
                    {
                        nlsClient.ReleaseSynthesizerRequest(sy.syPtr);
                        sy.syPtr.native_request = IntPtr.Zero;
                        globalRunParams.Remove(sy.uuid);

                        nlsResult.Text = "ReleaseSynthesizerRequest Success";
                        btnSYrelease.Enabled = false;
                        btnSYcreate.Enabled = true;
                        btnSYstart.Enabled = false;
                        btnSYcancel.Enabled = false;
                    }
                    else
                    {
                        nlsResult.Text = "ReleaseSynthesizerRequest is nullptr";
                    }
                    syList.RemoveLast();
                }
                syList.Clear();
            }

            cur_sy_closed = "null";
            cur_sy_completed = "null";
        }
        #endregion

        #region RecognizerCallback
        private CallbackDelegate DemoOnRecognitionStarted =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionStarted user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionStarted msg = {0}", msg);

                cur_sr_completed = "msg : " + msg;

                /*
                 * 更新状态机send_audio_flag为true，表示请求成功，可以开始传送音频
                 */
                RunParams demo_params = new RunParams();
                demo_params.send_audio_flag = true;
                demo_params.audio_loop_flag = globalRunParams[uuid].audio_loop_flag;

                globalRunParams.Remove(uuid);
                globalRunParams.Add(uuid, demo_params);
            };
        private CallbackDelegate DemoOnRecognitionClosed =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionClosed = {0}", msg);
                cur_sr_closed = "msg : " + msg;

                /*
                 * 这里可更新状态机为false，表示请求完成，可以停止传递音频和推出传递音频的线程
                 * 此处demo为循环运行，没有做停止此次请求的处理
                 */
            };
        private CallbackDelegate DemoOnRecognitionTaskFailed =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionTaskFailed user uuid = {0}", uuid);
                string msg = System.Text.Encoding.Default.GetString(e.msg).TrimEnd('\0');
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionTaskFailed = {0}", msg);
                cur_sr_completed = "msg : " + msg;

                /*
                 * 更新状态机为false，表示请求完成，可以停止传递音频和推出传递音频的线程
                 */
                RunParams demo_params = new RunParams();
                demo_params.send_audio_flag = false;
                demo_params.audio_loop_flag = false;

                globalRunParams.Remove(uuid);
                globalRunParams.Add(uuid, demo_params);
            };
        private CallbackDelegate DemoOnRecognitionResultChanged =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionResultChanged user uuid = {0}", uuid);
                string result = System.Text.Encoding.Default.GetString(e.result).TrimEnd('\0');
                //System.Diagnostics.Debug.WriteLine("DemoOnRecognitionResultChanged result = {0}", result);
                cur_sr_result = "middle result : " + result;
            };
        private CallbackDelegate DemoOnRecognitionCompleted =
            (ref NLS_EVENT_STRUCT e, ref string uuid) =>
            {
                System.Diagnostics.Debug.WriteLine("DemoOnRecognitionCompleted user uuid = {0}", uuid);
                string result = System.Text.Encoding.Default.GetString(e.result).TrimEnd('\0');
                //System.Diagnostics.Debug.WriteLine("DemoOnRecognitionCompleted result = {0}", result);
                cur_sr_completed = "final result : " + result;
            };
        #endregion

        #region RecognizerButton
        // create recognizer
        private void button8_Click(object sender, EventArgs e)
        {
            if (srList == null)
            {
                srList = new LinkedList<DemoSpeechRecognizerStruct>();
            }
            else
            {
                nlsResult.Text = "recognizer list is existed, release first...";
            }

            for (int i = 0; i < sr_concurrency_number; i++)
            {
                DemoSpeechRecognizerStruct srStruct;
                srStruct = new DemoSpeechRecognizerStruct();
                srStruct.srPtr = nlsClient.CreateRecognizerRequest();
                if (srStruct.srPtr.native_request != IntPtr.Zero)
                {
                    nlsResult.Text = "CreateRecognizerRequest Success";
                    btnSRcreate.Enabled = false;
                    btnSRstart.Enabled = true;
                    btnSRrelease.Enabled = true;

                    srStruct.uuid = System.Guid.NewGuid().ToString("N");
                    RunParams demo_params = new RunParams();
                    demo_params.send_audio_flag = false;
                    demo_params.audio_loop_flag = false;
                    globalRunParams[srStruct.uuid] = demo_params;
                }
                else
                {
                    nlsResult.Text = "CreateRecognizerRequest Failed";
                }
                srList.AddLast(srStruct);
            }
            cur_sr_result = "null";
            cur_sr_closed = "null";
            cur_sr_completed = "null";
        }

        // start recognizer
        private void button6_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (srList == null)
            {
                nlsResult.Text = "recognizer list is null, create first...";
                return;
            }
            else
            {
                if (File.Exists(cur_sr_file_path))
                {
                }
                else
                {
                    cur_sr_completed = "cannot open file: " + cur_sr_file_path;
                    return;
                }

                LinkedListNode<DemoSpeechRecognizerStruct> srStruct = srList.First;
                int sr_count = srList.Count;
                for (int i = 0; i < sr_count; i++)
                {
                    DemoSpeechRecognizerStruct sr = srStruct.Value;
                    if (sr.srPtr.native_request != IntPtr.Zero)
                    {
                        if (appKey == null || appKey.Length == 0)
                        {
                            appKey = tAppKey.Text;
                        }
                        if (token == null || token.Length == 0)
                        {
                            token = tToken.Text;
                        }
                        if (appKey == null || token == null ||
                            appKey.Length == 0 || token.Length == 0)
                        {
                            nlsResult.Text = "Start failed, token or appkey is empty";
                            return;
                        }

                        sr.srPtr.SetAppKey(sr.srPtr, appKey);
                        sr.srPtr.SetToken(sr.srPtr, token);
                        sr.srPtr.SetUrl(sr.srPtr, url);
                        sr.srPtr.SetFormat(sr.srPtr, "pcm");
                        sr.srPtr.SetSampleRate(sr.srPtr, 16000);
                        sr.srPtr.SetIntermediateResult(sr.srPtr, true);
                        sr.srPtr.SetPunctuationPrediction(sr.srPtr, true);
                        sr.srPtr.SetInverseTextNormalization(sr.srPtr, true);

                        sr.srPtr.SetOnRecognitionStarted(sr.srPtr, DemoOnRecognitionStarted, sr.uuid);
                        sr.srPtr.SetOnChannelClosed(sr.srPtr, DemoOnRecognitionClosed, sr.uuid);
                        sr.srPtr.SetOnTaskFailed(sr.srPtr, DemoOnRecognitionTaskFailed, sr.uuid);
                        sr.srPtr.SetOnRecognitionResultChanged(sr.srPtr, DemoOnRecognitionResultChanged, sr.uuid);
                        sr.srPtr.SetOnRecognitionCompleted(sr.srPtr, DemoOnRecognitionCompleted, sr.uuid);

                        ret = sr.srPtr.Start(sr.srPtr);
                        if (ret != 0)
                        {
                            nlsResult.Text = "recognizer Start failed";
                        }
                        else
                        {
                            if (globalRunParams[sr.uuid].audio_loop_flag == false)
                            {
                                RunParams demo_params = new RunParams();
                                demo_params.audio_loop_flag = true;
                                demo_params.send_audio_flag = globalRunParams[sr.uuid].send_audio_flag;

                                globalRunParams.Remove(sr.uuid);
                                globalRunParams.Add(sr.uuid, demo_params);

                                sr.sr_send_audio = new Thread(new ParameterizedThreadStart(SRAudioLab));
                                sr.sr_send_audio.Start((object)sr);

                                btnSRstart.Enabled = false;
                                btnSRstop.Enabled = true;
                            }

                            nlsResult.Text = "Recognizer Start success";
                        }
                    }
                    else
                    {
                    }
                    srStruct = srStruct.Next;
                    if (srStruct == null)
                    {
                        break;
                    }
                }
            }
        }

        // stop recognizer
        private void button5_Click(object sender, EventArgs e)
        {
            int ret = -1;
            if (srList == null)
            {
                nlsResult.Text = "recognizer list is null, create first...";
                return;
            }
            else
            {
                LinkedListNode<DemoSpeechRecognizerStruct> srStruct = srList.First;
                int sr_count = srList.Count;
                for (int i = 0; i < sr_count; i++)
                {
                    DemoSpeechRecognizerStruct sr = srStruct.Value;
                    if (sr.srPtr.native_request != IntPtr.Zero)
                    {
                        if (sr.srPtr.native_request != IntPtr.Zero)
                        {
                            RunParams demo_params = new RunParams();
                            demo_params.audio_loop_flag = globalRunParams[sr.uuid].audio_loop_flag;
                            demo_params.send_audio_flag = false;

                            globalRunParams.Remove(sr.uuid);
                            globalRunParams.Add(sr.uuid, demo_params);

                            ret = sr.srPtr.Stop(sr.srPtr);
                        }

                        if (ret != 0)
                        {
                            nlsResult.Text = "Recognizer Stop failed";
                        }
                        else
                        {
                            btnSRstop.Enabled = false;
                            nlsResult.Text = "Recognizer Stop success";
                        }
                    }
                    else
                    {
                    }
                    srStruct = srStruct.Next;
                    if (srStruct == null)
                    {
                        break;
                    }
                }
            }
        }

        // release recognizer
        private void button7_Click(object sender, EventArgs e)
        {
            if (srList == null)
            {
                nlsResult.Text = "recognizer list is null, create first...";
                return;
            }
            else
            {
                int sr_count = srList.Count;
                for (int i = 0; i < sr_count; i++)
                {
                    LinkedListNode<DemoSpeechRecognizerStruct> srStruct = srList.Last;
                    DemoSpeechRecognizerStruct sr = srStruct.Value;
                    if (sr.srPtr.native_request != IntPtr.Zero)
                    {
                        nlsClient.ReleaseRecognizerRequest(sr.srPtr);
                        sr.srPtr.native_request = IntPtr.Zero;
                        globalRunParams.Remove(sr.uuid);

                        nlsResult.Text = "ReleaseRecognizerRequest Success";
                        btnSRrelease.Enabled = false;
                        btnSRcreate.Enabled = true;
                        btnSRstart.Enabled = false;
                        btnSRstop.Enabled = false;
                    }
                    else
                    {
                        nlsResult.Text = "RecognizerRequest is nullptr";
                    }
                    srList.RemoveLast();
                }
                srList.Clear();
            }

            cur_sr_result = "null";
            cur_sr_closed = "null";
            cur_sr_completed = "null";
        }
        #endregion

        #region FileTransferButton
        private void button1_Click_5(object sender, EventArgs e)
        {
            FileTransferRequest request = nlsClient.CreateFileTransferRequest();
            request.SetAccessKeyId(request, akId);
            request.SetKeySecret(request, akSecret);
            request.SetAppKey(request, appKey);
            request.SetFileLinkUrl(request, fileLinkUrl);

            int ret = request.ApplyFileTrans(request);
            if (ret < 0)
            {
                string ft_error_msg = request.GetErrorMsg(request);
                System.Diagnostics.Debug.WriteLine("FileTransfer get error msg = {0}", ft_error_msg);
                ftResult.Text = ft_error_msg;
            }
            else
            {
                string ft_result_msg = request.GetResult(request);
                System.Diagnostics.Debug.WriteLine("FileTransfer get result msg = {0}", ft_result_msg);
                ftResult.Text = ft_result_msg;
            }
            nlsClient.ReleaseFileTransferRequest(request);
        }

        private void textBox4_TextChanged(object sender, EventArgs e)
        {
            fileLinkUrl = tFileLinkUrl.Text;
        }
        #endregion

        #region Useless
        private void label1_Click(object sender, EventArgs e) { }
        private void nlsCsharpSdkDemo_Load(object sender, EventArgs e) { }
        private void label6_Click(object sender, EventArgs e) { }
        private void label21_Click(object sender, EventArgs e) { }
        private void label14_Click(object sender, EventArgs e) { }
        private void syCompleted_Click(object sender, EventArgs e) { }
        private void srCompleted_Click(object sender, EventArgs e) { }
        private void label6_Click_1(object sender, EventArgs e) { }
        private void ftResult_Click(object sender, EventArgs e) { }
        private void label16_Click(object sender, EventArgs e) { }
        private void label21_Click_1(object sender, EventArgs e) { }
        #endregion
    }
}
