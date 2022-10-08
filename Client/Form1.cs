using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace Client
{
    public partial class Form1 : Form
    {
        private readonly ConnectDialog cd;
        private string LocalAddress;
        public Form1()
        {
            InitializeComponent();
            cd = new ConnectDialog();
            ChatBox.GotFocus += new EventHandler(ChatBox_GotFocus);
        }

        private void ChatBox_GotFocus(object sender, EventArgs e)
        {
            ChatBox.HideCaret();
        }

        public void OnDisconnect()
        {
            StringBuilder sb = new StringBuilder(100);
            bool connected = false;
            while (!connected)
            {
                cd.textBox1.Clear();
                cd.ShowDialog();
                if (cd.DialogResult == DialogResult.OK)
                {
                    try
                    {
                        var buf = cd.textBox1.Text.Split(':');
                        connected = StaticMethods.Connect(buf[0], Convert.ToUInt16(buf[1]));
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show("Connecting exception: " + ex.Message, "Error");
                    }
                }
                else
                {
                    Application.Exit();
                    return;
                }
            }
            StaticMethods.GetLocalIP(sb);
            LocalAddress = sb.ToString();
        }
        private void Form1_Load(object sender, EventArgs e)
        {
            MsgBox.SetWatermark("Enter message here");
            NameBox.SetWatermark("Name");
            ActiveControl = MsgBox;
        }
        private void MessageBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
            {
                //TODO: Отправляем текст на сервер, дисконнект по нажатию на кнопку
                try
                {
                    StaticMethods.Send(NameBox.Text + "#" + MsgBox.Text);
                    ChatBox.Text += ObtainChatName() + ": " + MsgBox.Text + Environment.NewLine;
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Error sending the message. Exception: " + ex.Message, "Error");
                }
                MsgBox.Clear();
                e.SuppressKeyPress = true;
            }
        }
        private void NameBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
            {
                e.SuppressKeyPress = true;
            }
        }
        private string ObtainChatName()
        {
            return (string.IsNullOrWhiteSpace(NameBox.Text) ? "Anonymous" : NameBox.Text) + '(' + LocalAddress + ')';
        }

        private void Form1_Shown(object sender, EventArgs e)
        {
            OnDisconnect();
            //NameInternal = ObtainChatName();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            StaticMethods.Disonnect();
            OnDisconnect();
        }
    }
}
