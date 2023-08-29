namespace nlsCsharpSdkDemo
{
    partial class nlsCsharpSdkDemo
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            this.btnOpenLog = new System.Windows.Forms.Button();
            this.stResult = new System.Windows.Forms.Label();
            this.btnGetVersion = new System.Windows.Forms.Button();
            this.btnInitNls = new System.Windows.Forms.Button();
            this.btnDeinitNls = new System.Windows.Forms.Button();
            this.btnSTcreate = new System.Windows.Forms.Button();
            this.btnSTrelease = new System.Windows.Forms.Button();
            this.btnSTstart = new System.Windows.Forms.Button();
            this.btnSTstop = new System.Windows.Forms.Button();
            this.tAkId = new System.Windows.Forms.TextBox();
            this.tAkSecret = new System.Windows.Forms.TextBox();
            this.tAppKey = new System.Windows.Forms.TextBox();
            this.tToken = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.tUrl = new System.Windows.Forms.TextBox();
            this.btnCreateToken = new System.Windows.Forms.Button();
            this.btnReleaseToken = new System.Windows.Forms.Button();
            this.btnSRstop = new System.Windows.Forms.Button();
            this.btnSRstart = new System.Windows.Forms.Button();
            this.btnSRrelease = new System.Windows.Forms.Button();
            this.btnSRcreate = new System.Windows.Forms.Button();
            this.btnSYcancel = new System.Windows.Forms.Button();
            this.btnSYstart = new System.Windows.Forms.Button();
            this.btnSYrelease = new System.Windows.Forms.Button();
            this.btnSYcreate = new System.Windows.Forms.Button();
            this.stCompleted = new System.Windows.Forms.Label();
            this.stClosed = new System.Windows.Forms.Label();
            this.label8 = new System.Windows.Forms.Label();
            this.label9 = new System.Windows.Forms.Label();
            this.label10 = new System.Windows.Forms.Label();
            this.label11 = new System.Windows.Forms.Label();
            this.label12 = new System.Windows.Forms.Label();
            this.label13 = new System.Windows.Forms.Label();
            this.srClosed = new System.Windows.Forms.Label();
            this.srCompleted = new System.Windows.Forms.Label();
            this.srResult = new System.Windows.Forms.Label();
            this.label17 = new System.Windows.Forms.Label();
            this.label18 = new System.Windows.Forms.Label();
            this.syClosed = new System.Windows.Forms.Label();
            this.syCompleted = new System.Windows.Forms.Label();
            this.nlsResult = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.textBox2 = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.textBox3 = new System.Windows.Forms.TextBox();
            this.label14 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.tFileLinkUrl = new System.Windows.Forms.TextBox();
            this.label15 = new System.Windows.Forms.Label();
            this.ftResult = new System.Windows.Forms.Label();
            this.label16 = new System.Windows.Forms.Label();
            this.tInput1 = new System.Windows.Forms.TextBox();
            this.tinput2 = new System.Windows.Forms.TextBox();
            this.label19 = new System.Windows.Forms.Label();
            this.label20 = new System.Windows.Forms.Label();
            this.syOutput = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // btnOpenLog
            // 
            this.btnOpenLog.Location = new System.Drawing.Point(58, 5);
            this.btnOpenLog.Name = "btnOpenLog";
            this.btnOpenLog.Size = new System.Drawing.Size(118, 47);
            this.btnOpenLog.TabIndex = 0;
            this.btnOpenLog.Text = "OpenLog";
            this.btnOpenLog.UseVisualStyleBackColor = true;
            this.btnOpenLog.Click += new System.EventHandler(this.button1_Click);
            // 
            // stResult
            // 
            this.stResult.BackColor = System.Drawing.SystemColors.ControlLight;
            this.stResult.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.stResult.Location = new System.Drawing.Point(220, 210);
            this.stResult.Name = "stResult";
            this.stResult.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.stResult.Size = new System.Drawing.Size(785, 55);
            this.stResult.TabIndex = 1;
            this.stResult.Text = "null";
            this.stResult.Click += new System.EventHandler(this.label1_Click);
            // 
            // btnGetVersion
            // 
            this.btnGetVersion.Location = new System.Drawing.Point(306, 58);
            this.btnGetVersion.Name = "btnGetVersion";
            this.btnGetVersion.Size = new System.Drawing.Size(114, 47);
            this.btnGetVersion.TabIndex = 2;
            this.btnGetVersion.Text = "GetVersion";
            this.btnGetVersion.UseVisualStyleBackColor = true;
            this.btnGetVersion.Click += new System.EventHandler(this.button1_Click_1);
            // 
            // btnInitNls
            // 
            this.btnInitNls.Location = new System.Drawing.Point(182, 5);
            this.btnInitNls.Name = "btnInitNls";
            this.btnInitNls.Size = new System.Drawing.Size(114, 47);
            this.btnInitNls.TabIndex = 3;
            this.btnInitNls.Text = "InitNls";
            this.btnInitNls.UseVisualStyleBackColor = true;
            this.btnInitNls.Click += new System.EventHandler(this.button1_Click_2);
            // 
            // btnDeinitNls
            // 
            this.btnDeinitNls.Enabled = false;
            this.btnDeinitNls.Location = new System.Drawing.Point(306, 5);
            this.btnDeinitNls.Name = "btnDeinitNls";
            this.btnDeinitNls.Size = new System.Drawing.Size(114, 47);
            this.btnDeinitNls.TabIndex = 4;
            this.btnDeinitNls.Text = "deinitNls";
            this.btnDeinitNls.UseVisualStyleBackColor = true;
            this.btnDeinitNls.Click += new System.EventHandler(this.button1_Click_3);
            // 
            // btnSTcreate
            // 
            this.btnSTcreate.Enabled = false;
            this.btnSTcreate.Location = new System.Drawing.Point(58, 156);
            this.btnSTcreate.Name = "btnSTcreate";
            this.btnSTcreate.Size = new System.Drawing.Size(153, 47);
            this.btnSTcreate.TabIndex = 5;
            this.btnSTcreate.Text = "CreateTranscriber";
            this.btnSTcreate.UseVisualStyleBackColor = true;
            this.btnSTcreate.Click += new System.EventHandler(this.button1_Click_4);
            // 
            // btnSTrelease
            // 
            this.btnSTrelease.Enabled = false;
            this.btnSTrelease.Location = new System.Drawing.Point(415, 156);
            this.btnSTrelease.Name = "btnSTrelease";
            this.btnSTrelease.Size = new System.Drawing.Size(168, 47);
            this.btnSTrelease.TabIndex = 6;
            this.btnSTrelease.Text = "ReleaseTranscriber";
            this.btnSTrelease.UseVisualStyleBackColor = true;
            this.btnSTrelease.Click += new System.EventHandler(this.button2_Click);
            // 
            // btnSTstart
            // 
            this.btnSTstart.Enabled = false;
            this.btnSTstart.Location = new System.Drawing.Point(217, 156);
            this.btnSTstart.Name = "btnSTstart";
            this.btnSTstart.Size = new System.Drawing.Size(93, 47);
            this.btnSTstart.TabIndex = 7;
            this.btnSTstart.Text = "Start";
            this.btnSTstart.UseVisualStyleBackColor = true;
            this.btnSTstart.Click += new System.EventHandler(this.button3_Click);
            // 
            // btnSTstop
            // 
            this.btnSTstop.Enabled = false;
            this.btnSTstop.Location = new System.Drawing.Point(316, 156);
            this.btnSTstop.Name = "btnSTstop";
            this.btnSTstop.Size = new System.Drawing.Size(93, 47);
            this.btnSTstop.TabIndex = 8;
            this.btnSTstop.Text = "Stop";
            this.btnSTstop.UseVisualStyleBackColor = true;
            this.btnSTstop.Click += new System.EventHandler(this.btnSTstop_Click);
            // 
            // tAkId
            // 
            this.tAkId.Location = new System.Drawing.Point(572, 36);
            this.tAkId.Name = "tAkId";
            this.tAkId.Size = new System.Drawing.Size(339, 25);
            this.tAkId.TabIndex = 9;
            this.tAkId.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
            // 
            // tAkSecret
            // 
            this.tAkSecret.Location = new System.Drawing.Point(1011, 36);
            this.tAkSecret.Name = "tAkSecret";
            this.tAkSecret.Size = new System.Drawing.Size(338, 25);
            this.tAkSecret.TabIndex = 10;
            this.tAkSecret.TextChanged += new System.EventHandler(this.tAkSecret_TextChanged);
            // 
            // tAppKey
            // 
            this.tAppKey.Location = new System.Drawing.Point(572, 5);
            this.tAppKey.Name = "tAppKey";
            this.tAppKey.Size = new System.Drawing.Size(339, 25);
            this.tAppKey.TabIndex = 11;
            this.tAppKey.TextChanged += new System.EventHandler(this.tAppKey_TextChanged);
            // 
            // tToken
            // 
            this.tToken.Location = new System.Drawing.Point(572, 67);
            this.tToken.Name = "tToken";
            this.tToken.Size = new System.Drawing.Size(339, 25);
            this.tToken.TabIndex = 12;
            this.tToken.TextChanged += new System.EventHandler(this.tToken_TextChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(509, 8);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(55, 15);
            this.label1.TabIndex = 13;
            this.label1.Text = "Appkey";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(509, 39);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(39, 15);
            this.label2.TabIndex = 14;
            this.label2.Text = "AkId";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(934, 39);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(71, 15);
            this.label3.TabIndex = 15;
            this.label3.Text = "AkSecret";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(509, 70);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(47, 15);
            this.label4.TabIndex = 16;
            this.label4.Text = "Token";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(934, 74);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(31, 15);
            this.label5.TabIndex = 18;
            this.label5.Text = "Url";
            // 
            // tUrl
            // 
            this.tUrl.Location = new System.Drawing.Point(1011, 67);
            this.tUrl.Name = "tUrl";
            this.tUrl.Size = new System.Drawing.Size(338, 25);
            this.tUrl.TabIndex = 17;
            this.tUrl.TextChanged += new System.EventHandler(this.tUrl_TextChanged);
            // 
            // btnCreateToken
            // 
            this.btnCreateToken.Enabled = false;
            this.btnCreateToken.Location = new System.Drawing.Point(58, 58);
            this.btnCreateToken.Name = "btnCreateToken";
            this.btnCreateToken.Size = new System.Drawing.Size(118, 47);
            this.btnCreateToken.TabIndex = 19;
            this.btnCreateToken.Text = "CreateToken";
            this.btnCreateToken.UseVisualStyleBackColor = true;
            this.btnCreateToken.Click += new System.EventHandler(this.button3_Click_1);
            // 
            // btnReleaseToken
            // 
            this.btnReleaseToken.Enabled = false;
            this.btnReleaseToken.Location = new System.Drawing.Point(182, 58);
            this.btnReleaseToken.Name = "btnReleaseToken";
            this.btnReleaseToken.Size = new System.Drawing.Size(114, 47);
            this.btnReleaseToken.TabIndex = 20;
            this.btnReleaseToken.Text = "ReleaseToken";
            this.btnReleaseToken.UseVisualStyleBackColor = true;
            this.btnReleaseToken.Click += new System.EventHandler(this.button4_Click);
            // 
            // btnSRstop
            // 
            this.btnSRstop.Enabled = false;
            this.btnSRstop.Location = new System.Drawing.Point(319, 417);
            this.btnSRstop.Name = "btnSRstop";
            this.btnSRstop.Size = new System.Drawing.Size(93, 47);
            this.btnSRstop.TabIndex = 24;
            this.btnSRstop.Text = "Stop";
            this.btnSRstop.UseVisualStyleBackColor = true;
            this.btnSRstop.Click += new System.EventHandler(this.button5_Click);
            // 
            // btnSRstart
            // 
            this.btnSRstart.Enabled = false;
            this.btnSRstart.Location = new System.Drawing.Point(220, 417);
            this.btnSRstart.Name = "btnSRstart";
            this.btnSRstart.Size = new System.Drawing.Size(93, 47);
            this.btnSRstart.TabIndex = 23;
            this.btnSRstart.Text = "Start";
            this.btnSRstart.UseVisualStyleBackColor = true;
            this.btnSRstart.Click += new System.EventHandler(this.button6_Click);
            // 
            // btnSRrelease
            // 
            this.btnSRrelease.Enabled = false;
            this.btnSRrelease.Location = new System.Drawing.Point(418, 417);
            this.btnSRrelease.Name = "btnSRrelease";
            this.btnSRrelease.Size = new System.Drawing.Size(168, 47);
            this.btnSRrelease.TabIndex = 22;
            this.btnSRrelease.Text = "ReleaseRecognizer";
            this.btnSRrelease.UseVisualStyleBackColor = true;
            this.btnSRrelease.Click += new System.EventHandler(this.button7_Click);
            // 
            // btnSRcreate
            // 
            this.btnSRcreate.Enabled = false;
            this.btnSRcreate.Location = new System.Drawing.Point(61, 417);
            this.btnSRcreate.Name = "btnSRcreate";
            this.btnSRcreate.Size = new System.Drawing.Size(153, 47);
            this.btnSRcreate.TabIndex = 21;
            this.btnSRcreate.Text = "CreateRecognizer";
            this.btnSRcreate.UseVisualStyleBackColor = true;
            this.btnSRcreate.Click += new System.EventHandler(this.button8_Click);
            // 
            // btnSYcancel
            // 
            this.btnSYcancel.Enabled = false;
            this.btnSYcancel.Location = new System.Drawing.Point(319, 650);
            this.btnSYcancel.Name = "btnSYcancel";
            this.btnSYcancel.Size = new System.Drawing.Size(93, 47);
            this.btnSYcancel.TabIndex = 28;
            this.btnSYcancel.Text = "Cancel";
            this.btnSYcancel.UseVisualStyleBackColor = true;
            this.btnSYcancel.Click += new System.EventHandler(this.button9_Click);
            // 
            // btnSYstart
            // 
            this.btnSYstart.Enabled = false;
            this.btnSYstart.Location = new System.Drawing.Point(220, 650);
            this.btnSYstart.Name = "btnSYstart";
            this.btnSYstart.Size = new System.Drawing.Size(93, 47);
            this.btnSYstart.TabIndex = 27;
            this.btnSYstart.Text = "Start";
            this.btnSYstart.UseVisualStyleBackColor = true;
            this.btnSYstart.Click += new System.EventHandler(this.button10_Click);
            // 
            // btnSYrelease
            // 
            this.btnSYrelease.Enabled = false;
            this.btnSYrelease.Location = new System.Drawing.Point(418, 650);
            this.btnSYrelease.Name = "btnSYrelease";
            this.btnSYrelease.Size = new System.Drawing.Size(168, 47);
            this.btnSYrelease.TabIndex = 26;
            this.btnSYrelease.Text = "ReleaseSynthesizer";
            this.btnSYrelease.UseVisualStyleBackColor = true;
            this.btnSYrelease.Click += new System.EventHandler(this.btnSYrelease_Click);
            // 
            // btnSYcreate
            // 
            this.btnSYcreate.Enabled = false;
            this.btnSYcreate.Location = new System.Drawing.Point(61, 650);
            this.btnSYcreate.Name = "btnSYcreate";
            this.btnSYcreate.Size = new System.Drawing.Size(153, 47);
            this.btnSYcreate.TabIndex = 25;
            this.btnSYcreate.Text = "CreateSynthesizer";
            this.btnSYcreate.UseVisualStyleBackColor = true;
            this.btnSYcreate.Click += new System.EventHandler(this.button12_Click);
            // 
            // stCompleted
            // 
            this.stCompleted.BackColor = System.Drawing.SystemColors.ControlLight;
            this.stCompleted.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.stCompleted.Location = new System.Drawing.Point(220, 285);
            this.stCompleted.Name = "stCompleted";
            this.stCompleted.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.stCompleted.Size = new System.Drawing.Size(785, 72);
            this.stCompleted.TabIndex = 29;
            this.stCompleted.Text = "null";
            this.stCompleted.Click += new System.EventHandler(this.label6_Click);
            // 
            // stClosed
            // 
            this.stClosed.BackColor = System.Drawing.SystemColors.ControlLight;
            this.stClosed.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.stClosed.Location = new System.Drawing.Point(220, 371);
            this.stClosed.Name = "stClosed";
            this.stClosed.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.stClosed.Size = new System.Drawing.Size(785, 34);
            this.stClosed.TabIndex = 30;
            this.stClosed.Text = "null";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(55, 209);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(159, 15);
            this.label8.TabIndex = 31;
            this.label8.Text = "Transcriber result:";
            // 
            // label9
            // 
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(55, 285);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(143, 15);
            this.label9.TabIndex = 32;
            this.label9.Text = "Completed result:";
            // 
            // label10
            // 
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(55, 371);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(119, 15);
            this.label10.TabIndex = 33;
            this.label10.Text = "Closed result:";
            // 
            // label11
            // 
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(55, 615);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(119, 15);
            this.label11.TabIndex = 39;
            this.label11.Text = "Closed result:";
            // 
            // label12
            // 
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(55, 541);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(143, 15);
            this.label12.TabIndex = 38;
            this.label12.Text = "Completed result:";
            // 
            // label13
            // 
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(55, 478);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(151, 15);
            this.label13.TabIndex = 37;
            this.label13.Text = "Recognizer result:";
            // 
            // srClosed
            // 
            this.srClosed.BackColor = System.Drawing.SystemColors.ControlLight;
            this.srClosed.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.srClosed.Location = new System.Drawing.Point(220, 615);
            this.srClosed.Name = "srClosed";
            this.srClosed.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.srClosed.Size = new System.Drawing.Size(780, 24);
            this.srClosed.TabIndex = 36;
            this.srClosed.Text = "null";
            this.srClosed.Click += new System.EventHandler(this.label14_Click);
            // 
            // srCompleted
            // 
            this.srCompleted.BackColor = System.Drawing.SystemColors.ControlLight;
            this.srCompleted.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.srCompleted.Location = new System.Drawing.Point(220, 541);
            this.srCompleted.Name = "srCompleted";
            this.srCompleted.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.srCompleted.Size = new System.Drawing.Size(780, 60);
            this.srCompleted.TabIndex = 35;
            this.srCompleted.Text = "null";
            this.srCompleted.Click += new System.EventHandler(this.srCompleted_Click);
            // 
            // srResult
            // 
            this.srResult.BackColor = System.Drawing.SystemColors.ControlLight;
            this.srResult.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.srResult.Location = new System.Drawing.Point(220, 478);
            this.srResult.Name = "srResult";
            this.srResult.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.srResult.Size = new System.Drawing.Size(780, 43);
            this.srResult.TabIndex = 34;
            this.srResult.Text = "null";
            // 
            // label17
            // 
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(55, 796);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(119, 15);
            this.label17.TabIndex = 43;
            this.label17.Text = "Closed result:";
            // 
            // label18
            // 
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(55, 710);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(143, 15);
            this.label18.TabIndex = 42;
            this.label18.Text = "Completed result:";
            // 
            // syClosed
            // 
            this.syClosed.BackColor = System.Drawing.SystemColors.ControlLight;
            this.syClosed.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.syClosed.Location = new System.Drawing.Point(220, 796);
            this.syClosed.Name = "syClosed";
            this.syClosed.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.syClosed.Size = new System.Drawing.Size(780, 31);
            this.syClosed.TabIndex = 41;
            this.syClosed.Text = "null";
            // 
            // syCompleted
            // 
            this.syCompleted.BackColor = System.Drawing.SystemColors.ControlLight;
            this.syCompleted.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.syCompleted.Location = new System.Drawing.Point(220, 711);
            this.syCompleted.Name = "syCompleted";
            this.syCompleted.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.syCompleted.Size = new System.Drawing.Size(780, 74);
            this.syCompleted.TabIndex = 40;
            this.syCompleted.Text = "null";
            this.syCompleted.Click += new System.EventHandler(this.syCompleted_Click);
            // 
            // nlsResult
            // 
            this.nlsResult.BackColor = System.Drawing.SystemColors.ControlLight;
            this.nlsResult.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.nlsResult.Location = new System.Drawing.Point(55, 123);
            this.nlsResult.Name = "nlsResult";
            this.nlsResult.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.nlsResult.Size = new System.Drawing.Size(528, 24);
            this.nlsResult.TabIndex = 44;
            this.nlsResult.Text = "nls...";
            this.nlsResult.Click += new System.EventHandler(this.label21_Click);
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(610, 172);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(159, 15);
            this.label6.TabIndex = 45;
            this.label6.Text = "concurrency number:";
            this.label6.Click += new System.EventHandler(this.label6_Click_1);
            // 
            // textBox1
            // 
            this.textBox1.Location = new System.Drawing.Point(775, 169);
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(93, 25);
            this.textBox1.TabIndex = 46;
            this.textBox1.Text = "1";
            this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged_1);
            // 
            // textBox2
            // 
            this.textBox2.Location = new System.Drawing.Point(772, 430);
            this.textBox2.Name = "textBox2";
            this.textBox2.Size = new System.Drawing.Size(93, 25);
            this.textBox2.TabIndex = 48;
            this.textBox2.Text = "1";
            this.textBox2.TextChanged += new System.EventHandler(this.textBox2_TextChanged);
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(610, 433);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(159, 15);
            this.label7.TabIndex = 47;
            this.label7.Text = "concurrency number:";
            // 
            // textBox3
            // 
            this.textBox3.Location = new System.Drawing.Point(775, 663);
            this.textBox3.Name = "textBox3";
            this.textBox3.Size = new System.Drawing.Size(93, 25);
            this.textBox3.TabIndex = 50;
            this.textBox3.Text = "1";
            this.textBox3.TextChanged += new System.EventHandler(this.textBox3_TextChanged);
            // 
            // label14
            // 
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(610, 666);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(159, 15);
            this.label14.TabIndex = 49;
            this.label14.Text = "concurrency number:";
            // 
            // button1
            // 
            this.button1.Enabled = false;
            this.button1.Location = new System.Drawing.Point(61, 838);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(153, 47);
            this.button1.TabIndex = 51;
            this.button1.Text = "FileTransfer";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click_5);
            // 
            // tFileLinkUrl
            // 
            this.tFileLinkUrl.Location = new System.Drawing.Point(223, 838);
            this.tFileLinkUrl.Name = "tFileLinkUrl";
            this.tFileLinkUrl.Size = new System.Drawing.Size(721, 25);
            this.tFileLinkUrl.TabIndex = 52;
            this.tFileLinkUrl.Text = "https://gw.alipayobjects.com/os/bmw-prod/0574ee2e-f494-45a5-820f-63aee583045a.wav" +
    "";
            this.tFileLinkUrl.TextChanged += new System.EventHandler(this.textBox4_TextChanged);
            // 
            // label15
            // 
            this.label15.AutoSize = true;
            this.label15.Location = new System.Drawing.Point(55, 897);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(135, 15);
            this.label15.TabIndex = 54;
            this.label15.Text = "transfer result:";
            // 
            // ftResult
            // 
            this.ftResult.BackColor = System.Drawing.SystemColors.ControlLight;
            this.ftResult.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.ftResult.Location = new System.Drawing.Point(220, 896);
            this.ftResult.Name = "ftResult";
            this.ftResult.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.ftResult.Size = new System.Drawing.Size(1129, 72);
            this.ftResult.TabIndex = 53;
            this.ftResult.Text = "null";
            this.ftResult.Click += new System.EventHandler(this.ftResult_Click);
            // 
            // label16
            // 
            this.label16.AutoSize = true;
            this.label16.Location = new System.Drawing.Point(897, 172);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(103, 15);
            this.label16.TabIndex = 55;
            this.label16.Text = "input audio:";
            this.label16.Click += new System.EventHandler(this.label16_Click);
            // 
            // tInput1
            // 
            this.tInput1.Location = new System.Drawing.Point(1011, 169);
            this.tInput1.Multiline = true;
            this.tInput1.Name = "tInput1";
            this.tInput1.Size = new System.Drawing.Size(338, 69);
            this.tInput1.TabIndex = 56;
            this.tInput1.Text = "audio_files\\test3.wav";
            this.tInput1.TextChanged += new System.EventHandler(this.tInput1_TextChanged);
            // 
            // tinput2
            // 
            this.tinput2.Location = new System.Drawing.Point(1011, 430);
            this.tinput2.Multiline = true;
            this.tinput2.Name = "tinput2";
            this.tinput2.Size = new System.Drawing.Size(338, 69);
            this.tinput2.TabIndex = 58;
            this.tinput2.Text = "audio_files\\test2.wav";
            this.tinput2.TextChanged += new System.EventHandler(this.tinput2_TextChanged);
            // 
            // label19
            // 
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(902, 433);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(103, 15);
            this.label19.TabIndex = 57;
            this.label19.Text = "input audio:";
            // 
            // label20
            // 
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(902, 666);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(103, 15);
            this.label20.TabIndex = 59;
            this.label20.Text = "output file:";
            // 
            // syOutput
            // 
            this.syOutput.BackColor = System.Drawing.SystemColors.ControlLight;
            this.syOutput.ImageAlign = System.Drawing.ContentAlignment.MiddleLeft;
            this.syOutput.Location = new System.Drawing.Point(1011, 666);
            this.syOutput.Name = "syOutput";
            this.syOutput.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.syOutput.Size = new System.Drawing.Size(338, 85);
            this.syOutput.TabIndex = 60;
            this.syOutput.Text = "null";
            this.syOutput.Click += new System.EventHandler(this.label21_Click_1);
            // 
            // nlsCsharpSdkDemo
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1427, 990);
            this.Controls.Add(this.syOutput);
            this.Controls.Add(this.label20);
            this.Controls.Add(this.tinput2);
            this.Controls.Add(this.label19);
            this.Controls.Add(this.tInput1);
            this.Controls.Add(this.label16);
            this.Controls.Add(this.label15);
            this.Controls.Add(this.ftResult);
            this.Controls.Add(this.tFileLinkUrl);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.textBox3);
            this.Controls.Add(this.label14);
            this.Controls.Add(this.textBox2);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.nlsResult);
            this.Controls.Add(this.label17);
            this.Controls.Add(this.label18);
            this.Controls.Add(this.syClosed);
            this.Controls.Add(this.syCompleted);
            this.Controls.Add(this.label11);
            this.Controls.Add(this.label12);
            this.Controls.Add(this.label13);
            this.Controls.Add(this.srClosed);
            this.Controls.Add(this.srCompleted);
            this.Controls.Add(this.srResult);
            this.Controls.Add(this.label10);
            this.Controls.Add(this.label9);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.stClosed);
            this.Controls.Add(this.stCompleted);
            this.Controls.Add(this.btnSYcancel);
            this.Controls.Add(this.btnSYstart);
            this.Controls.Add(this.btnSYrelease);
            this.Controls.Add(this.btnSYcreate);
            this.Controls.Add(this.btnSRstop);
            this.Controls.Add(this.btnSRstart);
            this.Controls.Add(this.btnSRrelease);
            this.Controls.Add(this.btnSRcreate);
            this.Controls.Add(this.btnReleaseToken);
            this.Controls.Add(this.btnCreateToken);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.tUrl);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.tToken);
            this.Controls.Add(this.tAppKey);
            this.Controls.Add(this.tAkSecret);
            this.Controls.Add(this.tAkId);
            this.Controls.Add(this.btnSTstop);
            this.Controls.Add(this.btnSTstart);
            this.Controls.Add(this.btnSTrelease);
            this.Controls.Add(this.btnSTcreate);
            this.Controls.Add(this.btnDeinitNls);
            this.Controls.Add(this.btnInitNls);
            this.Controls.Add(this.btnGetVersion);
            this.Controls.Add(this.stResult);
            this.Controls.Add(this.btnOpenLog);
            this.Name = "nlsCsharpSdkDemo";
            this.Text = "NlsCsharpSdkDemo";
            this.Load += new System.EventHandler(this.nlsCsharpSdkDemo_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btnOpenLog;
        private System.Windows.Forms.Label stResult;
        private System.Windows.Forms.Button btnGetVersion;
        private System.Windows.Forms.Button btnInitNls;
        private System.Windows.Forms.Button btnDeinitNls;
        private System.Windows.Forms.Button btnSTcreate;
        private System.Windows.Forms.Button btnSTrelease;
        private System.Windows.Forms.Button btnSTstart;
        private System.Windows.Forms.Button btnSTstop;
        private System.Windows.Forms.TextBox tAkId;
        private System.Windows.Forms.TextBox tAkSecret;
        private System.Windows.Forms.TextBox tAppKey;
        private System.Windows.Forms.TextBox tToken;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.TextBox tUrl;
        private System.Windows.Forms.Button btnCreateToken;
        private System.Windows.Forms.Button btnReleaseToken;
        private System.Windows.Forms.Button btnSRstop;
        private System.Windows.Forms.Button btnSRstart;
        private System.Windows.Forms.Button btnSRrelease;
        private System.Windows.Forms.Button btnSRcreate;
        private System.Windows.Forms.Button btnSYcancel;
        private System.Windows.Forms.Button btnSYstart;
        private System.Windows.Forms.Button btnSYrelease;
        private System.Windows.Forms.Button btnSYcreate;
        private System.Windows.Forms.Label stCompleted;
        private System.Windows.Forms.Label stClosed;
        private System.Windows.Forms.Label label8;
        private System.Windows.Forms.Label label9;
        private System.Windows.Forms.Label label10;
        private System.Windows.Forms.Label label11;
        private System.Windows.Forms.Label label12;
        private System.Windows.Forms.Label label13;
        private System.Windows.Forms.Label srClosed;
        private System.Windows.Forms.Label srCompleted;
        private System.Windows.Forms.Label srResult;
        private System.Windows.Forms.Label label17;
        private System.Windows.Forms.Label label18;
        private System.Windows.Forms.Label syClosed;
        private System.Windows.Forms.Label syCompleted;
        private System.Windows.Forms.Label nlsResult;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.TextBox textBox2;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.TextBox textBox3;
        private System.Windows.Forms.Label label14;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.TextBox tFileLinkUrl;
        private System.Windows.Forms.Label label15;
        private System.Windows.Forms.Label ftResult;
        private System.Windows.Forms.Label label16;
        private System.Windows.Forms.TextBox tInput1;
        private System.Windows.Forms.TextBox tinput2;
        private System.Windows.Forms.Label label19;
        private System.Windows.Forms.Label label20;
        private System.Windows.Forms.Label syOutput;
    }
}

