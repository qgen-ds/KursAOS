using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using System.Threading.Tasks;

namespace Client
{
    public partial class Form1 : Form
    {
        private readonly ConnectDialog cd;
        private event EventHandler NetworkEvent;
        private static EventRaiserDelegate EventRaiser;
        public RECVPARAM RecvBuf;
        public Form1()
        {
            InitializeComponent();
            cd = new ConnectDialog();
            ChatBox.GotFocus += new EventHandler(ChatBox_GotFocus);
            NetworkEvent += new EventHandler(HandleNetworkEvent);
            EventRaiser = new EventRaiserDelegate(OnNetworkEvent); // Store OnNetworkEvent to guard it against GC
            RecvBuf = new RECVPARAM
            {
                EventRaiser = Marshal.GetFunctionPointerForDelegate(EventRaiser),
                Buf = new WSABUF()
            };
        }

        public void OnNetworkEvent()
        {
            NetworkEvent?.Invoke(this, EventArgs.Empty);
        }

        private void HandleNetworkEvent(object sender, EventArgs e)
        {
            byte[] buf = new byte[RecvBuf.Buf.len];
            if (RecvBuf.Buf.buf != null)
            {
                var str = Marshal.PtrToStringAuto(RecvBuf.Buf.buf, Convert.ToInt32(RecvBuf.Buf.len / sizeof(char)));
                var arr = StaticMethods.Decode(str.Remove(str.Length - 2).Split('#'));          
                // 0 - Message
                // 1 - Name
                // 2 - IP address
                // 3 - ID
                if(arr[0][0] == '@')
                {
                    arr[0] = arr[0].Substring(arr[0].IndexOf(' ') + 1);
                    Invoke(new Action(() =>
                    {
                        ChatBox.AppendText("PM from " + arr[1] + '(' + arr[2] + ')' + "(ID " + arr[3] + "): " + arr[0] + Environment.NewLine);
                    }));
                    return;
                }
                Invoke(new Action(() =>
                {
                    ChatBox.AppendText(arr[1] + '(' + arr[2] + ')' + "(ID " + arr[3] + "): " + arr[0] + Environment.NewLine);
                }));
            }
            else
            {
                MessageBox.Show("Failed to recieve message from the server.");
            }
        }
        private void ChatBox_GotFocus(object sender, EventArgs e)
        {
            ChatBox.HideCaret();
        }

        public void OnDisconnect()
        {
            bool connected = false;
            cd.textBox1.Clear();
            while (!connected)
            {
                cd.ShowDialog();
                if (cd.DialogResult == DialogResult.OK)
                {
                    try
                    {
                        var buf = cd.textBox1.Text.Split(':');
                        connected = StaticMethods.Connect(buf[0], Convert.ToUInt16(buf[1]), ref RecvBuf);
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
                //Message#
                //Name#&
                List<string> Contents = new List<string>
                {
                    MsgBox.Text,
                    ObtainChatName()
                };
                new Task(() =>
                {
                    // Отправляем текст на сервер
                   try
                   {
                        if (Contents[0][0] == '@')
                        {
                            var id_str = Contents[0].Substring(1, Contents[0].IndexOf(' ') - 1);
                            var str = Contents[0].Substring(Contents[0].IndexOf(' ') + 1);
                            Invoke(new Action(() =>
                            {
                                ChatBox.AppendText("PM to ID " + '(' + id_str + "): " + str + Environment.NewLine);
                            }));
                        }
                        Contents.Encode();
                        StaticMethods.Send(string.Join("#", Contents.ToArray()) + "#&");
                    }
                   catch (Exception ex)
                   {
                       MessageBox.Show("Error sending the message. Exception: " + ex.Message, "Error");
                   }
                }).Start();
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
            return (string.IsNullOrWhiteSpace(NameBox.Text) ? "Anonymous" : NameBox.Text);
        }
        private void Form1_Shown(object sender, EventArgs e)
        {
            OnDisconnect();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            StaticMethods.Disonnect();
            OnDisconnect();
        }
    }
}
