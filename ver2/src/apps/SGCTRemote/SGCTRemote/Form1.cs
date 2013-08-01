using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace SGCTRemote
{
    struct clientData
    {
        public NetworkManager connection;
        public string ip;
        public Int32 port;
        public int bufferSize;
    }
    
    public partial class Form1 : Form
    {
        private clientData c;

        public Form1()
        {
            InitializeComponent();
            init();
        }

        private void init()
        {
            c = new clientData();
            c.connection = new NetworkManager();
            c.port = 20500;
            c.ip = "127.0.0.1"; //default ip
            c.bufferSize = 1024;

            this.toolStripStatusLabel1.Text = "Disconnected";
            this.ipTextBox.Text = c.ip;
            this.SizeTrackBar.Value = 50;
            this.SizeLabel.Text = "Size = 50 %";

            componentVisibility(false);
        }

        private void exit()
        {
            disconnect();
            System.Environment.Exit(0);
        }

        /*
         * Enable or disable items depending on connection status
         */
        private void componentVisibility(bool status)
        {
            this.PropertiesGroupBox.Enabled = status;
        }

        #region Network

        private void connect()
        {
            //if connection successfull
            if (c.connection.ConnectIP(c.ip, c.port, c.bufferSize))
            {
                componentVisibility(true);
                this.connectButton.Text = "Disconnect";
                this.toolStripStatusLabel1.Text = "Connected";
                
                //send defaults
                c.connection.Send("stats=0\r\ngraph=0\r\nwire=0\r\nsize=50");
            }
            else
            {
                componentVisibility(false);
                this.connectButton.Text = "Connect";
                this.toolStripStatusLabel1.Text = "Disconnected";
            }
        }

        private void disconnect()
        {
            componentVisibility(false);
            this.connectButton.Text = "Connect";
            this.toolStripStatusLabel1.Text = "Disconnected";

            if (c.connection != null)
            {
                c.connection.valid = false;
                c.connection.Disconnect();
            }
        }

        #endregion

        #region callbacks

        private void MainForm_Closed(object sender, System.EventArgs e)
        {
            exit();
        }

        private void connectButton_Click(object sender, EventArgs e)
        {
            if (!c.connection.valid)
            {
                //get the ip address string from the textbox 
                c.ip = this.ipTextBox.Text;

                connect();
            }
            else
            {
                disconnect();
            }
        }

        private void StatsCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (c.connection.valid)
            {
                CheckBox cb = (CheckBox)sender;

                if (cb.Checked)
                    c.connection.Send("stats=1");
                else
                    c.connection.Send("stats=0");
            }
        }

        private void GraphCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (c.connection.valid)
            {
                CheckBox cb = (CheckBox)sender;

                if (cb.Checked)
                    c.connection.Send("graph=1");
                else
                    c.connection.Send("graph=0");
            }
        }

        private void WireframeCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            if (c.connection.valid)
            {
                CheckBox cb = (CheckBox)sender;

                if (cb.Checked)
                    c.connection.Send("wire=1");
                else
                    c.connection.Send("wire=0");
            }
        }

        private void SizeTrackBar_Scroll(object sender, EventArgs e)
        {
            TrackBar tb = (TrackBar)sender;
            this.SizeLabel.Text = "Size: " + tb.Value.ToString() + " %";

            if (c.connection.valid)
            {
                c.connection.Send("size=" + tb.Value.ToString());
            }
        }

        #endregion
    }
}
