namespace SGCTRemote
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.statusStrip1 = new System.Windows.Forms.StatusStrip();
            this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.connectButton = new System.Windows.Forms.Button();
            this.ipTextBox = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.PropertiesGroupBox = new System.Windows.Forms.GroupBox();
            this.GraphCheckBox = new System.Windows.Forms.CheckBox();
            this.StatsCheckBox = new System.Windows.Forms.CheckBox();
            this.WireframeCheckBox = new System.Windows.Forms.CheckBox();
            this.SizeTrackBar = new System.Windows.Forms.TrackBar();
            this.SizeLabel = new System.Windows.Forms.Label();
            this.statusStrip1.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.PropertiesGroupBox.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.SizeTrackBar)).BeginInit();
            this.SuspendLayout();
            // 
            // statusStrip1
            // 
            this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1});
            this.statusStrip1.Location = new System.Drawing.Point(0, 240);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new System.Drawing.Size(284, 22);
            this.statusStrip1.TabIndex = 0;
            this.statusStrip1.Text = "statusStrip1";
            // 
            // toolStripStatusLabel1
            // 
            this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
            this.toolStripStatusLabel1.Size = new System.Drawing.Size(118, 17);
            this.toolStripStatusLabel1.Text = "toolStripStatusLabel1";
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.connectButton);
            this.groupBox1.Controls.Add(this.ipTextBox);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Location = new System.Drawing.Point(13, 13);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(259, 48);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Network";
            // 
            // connectButton
            // 
            this.connectButton.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.connectButton.Location = new System.Drawing.Point(151, 17);
            this.connectButton.Name = "connectButton";
            this.connectButton.Size = new System.Drawing.Size(102, 20);
            this.connectButton.TabIndex = 3;
            this.connectButton.Text = "Connect";
            this.connectButton.UseVisualStyleBackColor = true;
            this.connectButton.Click += new System.EventHandler(this.connectButton_Click);
            // 
            // ipTextBox
            // 
            this.ipTextBox.Location = new System.Drawing.Point(57, 17);
            this.ipTextBox.MaxLength = 16;
            this.ipTextBox.Name = "ipTextBox";
            this.ipTextBox.Size = new System.Drawing.Size(88, 20);
            this.ipTextBox.TabIndex = 2;
            this.ipTextBox.Text = "127.0.0.1";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(8, 20);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(48, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Address:";
            // 
            // PropertiesGroupBox
            // 
            this.PropertiesGroupBox.Controls.Add(this.SizeLabel);
            this.PropertiesGroupBox.Controls.Add(this.SizeTrackBar);
            this.PropertiesGroupBox.Controls.Add(this.WireframeCheckBox);
            this.PropertiesGroupBox.Controls.Add(this.GraphCheckBox);
            this.PropertiesGroupBox.Controls.Add(this.StatsCheckBox);
            this.PropertiesGroupBox.Location = new System.Drawing.Point(13, 68);
            this.PropertiesGroupBox.Name = "PropertiesGroupBox";
            this.PropertiesGroupBox.Size = new System.Drawing.Size(259, 160);
            this.PropertiesGroupBox.TabIndex = 2;
            this.PropertiesGroupBox.TabStop = false;
            this.PropertiesGroupBox.Text = "Properties";
            // 
            // GraphCheckBox
            // 
            this.GraphCheckBox.AutoSize = true;
            this.GraphCheckBox.Location = new System.Drawing.Point(16, 51);
            this.GraphCheckBox.Name = "GraphCheckBox";
            this.GraphCheckBox.Size = new System.Drawing.Size(145, 17);
            this.GraphCheckBox.TabIndex = 1;
            this.GraphCheckBox.Text = "Show performance graph";
            this.GraphCheckBox.UseVisualStyleBackColor = true;
            this.GraphCheckBox.CheckedChanged += new System.EventHandler(this.GraphCheckBox_CheckedChanged);
            // 
            // StatsCheckBox
            // 
            this.StatsCheckBox.AutoSize = true;
            this.StatsCheckBox.Location = new System.Drawing.Point(16, 27);
            this.StatsCheckBox.Name = "StatsCheckBox";
            this.StatsCheckBox.Size = new System.Drawing.Size(96, 17);
            this.StatsCheckBox.TabIndex = 0;
            this.StatsCheckBox.Text = "Show statistics";
            this.StatsCheckBox.UseVisualStyleBackColor = true;
            this.StatsCheckBox.CheckedChanged += new System.EventHandler(this.StatsCheckBox_CheckedChanged);
            // 
            // WireframeCheckBox
            // 
            this.WireframeCheckBox.AutoSize = true;
            this.WireframeCheckBox.Location = new System.Drawing.Point(16, 75);
            this.WireframeCheckBox.Name = "WireframeCheckBox";
            this.WireframeCheckBox.Size = new System.Drawing.Size(101, 17);
            this.WireframeCheckBox.TabIndex = 2;
            this.WireframeCheckBox.Text = "Show wireframe";
            this.WireframeCheckBox.UseVisualStyleBackColor = true;
            this.WireframeCheckBox.CheckedChanged += new System.EventHandler(this.WireframeCheckBox_CheckedChanged);
            // 
            // SizeTrackBar
            // 
            this.SizeTrackBar.AutoSize = false;
            this.SizeTrackBar.Location = new System.Drawing.Point(11, 123);
            this.SizeTrackBar.Maximum = 100;
            this.SizeTrackBar.Name = "SizeTrackBar";
            this.SizeTrackBar.Size = new System.Drawing.Size(242, 28);
            this.SizeTrackBar.TabIndex = 3;
            this.SizeTrackBar.TickFrequency = 5;
            this.SizeTrackBar.Scroll += new System.EventHandler(this.SizeTrackBar_Scroll);
            // 
            // SizeLabel
            // 
            this.SizeLabel.AutoSize = true;
            this.SizeLabel.Location = new System.Drawing.Point(99, 109);
            this.SizeLabel.Name = "SizeLabel";
            this.SizeLabel.Size = new System.Drawing.Size(62, 13);
            this.SizeLabel.TabIndex = 4;
            this.SizeLabel.Text = "Size = 50 %";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(284, 262);
            this.Controls.Add(this.PropertiesGroupBox);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.statusStrip1);
            this.Name = "Form1";
            this.Text = "SGCT Remote";
            this.Closed += new System.EventHandler(this.MainForm_Closed);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.PropertiesGroupBox.ResumeLayout(false);
            this.PropertiesGroupBox.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.SizeTrackBar)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.StatusStrip statusStrip1;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button connectButton;
        private System.Windows.Forms.TextBox ipTextBox;
        private System.Windows.Forms.GroupBox PropertiesGroupBox;
        private System.Windows.Forms.CheckBox StatsCheckBox;
        private System.Windows.Forms.CheckBox GraphCheckBox;
        private System.Windows.Forms.TrackBar SizeTrackBar;
        private System.Windows.Forms.CheckBox WireframeCheckBox;
        private System.Windows.Forms.Label SizeLabel;
    }
}

