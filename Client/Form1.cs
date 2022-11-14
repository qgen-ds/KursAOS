using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Threading.Tasks;
using System.Net;

namespace Client
{
    public partial class Form1 : Form
    {
        private readonly ConnectDialog cd;
        private event EventHandler NetworkEvent;
        public RECVPARAM RecvBuf;
        public Form1()
        {
            InitializeComponent();
            cd = new ConnectDialog();
            ChatBox.GotFocus += new EventHandler(ChatBox_GotFocus);
            NetworkEvent += new EventHandler(HandleNetworkEvent);
            RecvBuf = new RECVPARAM
            {
                EventRaiser = Marshal.GetFunctionPointerForDelegate(new EventRaiserDelegate(OnNetworkEvent)),
                OnDisconnect = Marshal.GetFunctionPointerForDelegate(new EventRaiserDelegate(OnDisconnect)),
                Buf = new WSABUF(),
                MarkForDelete = 0
            };
        }

        public void OnNetworkEvent()
        {
            NetworkEvent?.Invoke(this, EventArgs.Empty);
        }

        private void HandleNetworkEvent(object sender, EventArgs e)
        {
            Packet pkt = new Packet();
            int[] data = new int[1];
            if (RecvBuf.Buf.buf != IntPtr.Zero)
            {
                IntPtr ptr = RecvBuf.Buf.buf;
                Marshal.Copy(RecvBuf.Buf.buf, data, 0, 1);
                pkt.Code = (Command)IPAddress.NetworkToHostOrder(data[0]);
                ptr += 4;
                switch (pkt.Code)
                {
                    case Command.COMMAND_COMMON_MESSAGE:
                        {
                            data = new int[3];
                            Marshal.Copy(ptr, data, 0, 3);
                            ptr += 12;
                            pkt.NameLen = IPAddress.NetworkToHostOrder(data[0]);
                            uint ID = Convert.ToUInt32(IPAddress.NetworkToHostOrder(data[1]));
                            int addrlen = IPAddress.NetworkToHostOrder(data[2]);
                            string addr = Marshal.PtrToStringAuto(ptr, addrlen);
                            ptr += addrlen * sizeof(char);
                            pkt.Name = Marshal.PtrToStringAuto(ptr, pkt.NameLen);
                            ptr += pkt.NameLen * sizeof(char);
                            int msglen = (Convert.ToInt32(RecvBuf.Buf.len) - (ptr.ToInt32() - RecvBuf.Buf.buf.ToInt32())) / sizeof(char);
                            pkt.Message = Marshal.PtrToStringAuto(ptr, msglen);
                            Invoke(new Action(() =>
                            {
                                ChatBox.AppendText(pkt.Name + '(' + addr + ')' + "(ID " + ID.ToString() + "): " + pkt.Message + Environment.NewLine);
                            }));
                            break;
                        }
                    case Command.COMMAND_PRIVATE_MESSAGE:
                        {
                            data = new int[3];
                            Marshal.Copy(ptr, data, 0, 3);
                            ptr += 12;
                            uint ID = Convert.ToUInt32(IPAddress.NetworkToHostOrder(data[0]));
                            int addrlen = IPAddress.NetworkToHostOrder(data[1]);
                            pkt.NameLen = IPAddress.NetworkToHostOrder(data[2]);
                            IntPtr name = RecvBuf.Buf.buf + Convert.ToInt32(RecvBuf.Buf.len) - pkt.NameLen * sizeof(char);
                            pkt.Name = Marshal.PtrToStringAuto(name, pkt.NameLen);
                            string addr = Marshal.PtrToStringAuto(ptr, addrlen);
                            ptr += addrlen * sizeof(char);
                            int msglen = (name.ToInt32() - ptr.ToInt32()) / sizeof(char);
                            pkt.Message = Marshal.PtrToStringAuto(ptr, msglen);
                            Invoke(new Action(() =>
                            {
                                ChatBox.AppendText("Сообщение от " + pkt.Name + '(' + addr + ')' + " (ID " + ID.ToString() + "): " + pkt.Message + Environment.NewLine);
                            }));
                            break;
                        }
                    case Command.COMMAND_PM_RETURN:
                        {
                            data = new int[2];
                            Marshal.Copy(ptr, data, 0, 2);
                            ptr += 12;
                            uint ID = Convert.ToUInt32(IPAddress.NetworkToHostOrder(data[0]));
                            int addrlen = IPAddress.NetworkToHostOrder(data[1]);
                            string addr = Marshal.PtrToStringAuto(ptr, addrlen);
                            ptr += addrlen * sizeof(char);
                            int msglen = (Convert.ToInt32(RecvBuf.Buf.len) - (ptr.ToInt32() - RecvBuf.Buf.buf.ToInt32())) / sizeof(char);
                            pkt.Message = Marshal.PtrToStringAuto(ptr, msglen);
                            Invoke(new Action(() =>
                            {
                                ChatBox.AppendText("Сообщение доставлено " + '(' + addr + ')' + " (ID " + ID.ToString() + "): " + pkt.Message + Environment.NewLine);
                            }));
                        }
                        break;
                    case Command.COMMAND_ERROR:
                        {
                            pkt.Message = Marshal.PtrToStringAuto(ptr, (Convert.ToInt32(RecvBuf.Buf.len) - 4) / sizeof(char));
                            Invoke(new Action(() =>
                            {
                                ChatBox.AppendText("Ошибка: " + pkt.Message + Environment.NewLine);
                            }));
                        }
                        break;
                }
                if (RecvBuf.MarkForDelete != 0)
                {
                    StaticMethods.FreeBlock(RecvBuf.Buf.buf);
                }
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
            Invoke(new Action(() =>
            {
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
                ChatBox.Clear();
            }));
        }
        private void Form1_Load(object sender, EventArgs e)
        {
            MsgBox.SetWatermark("Enter message here");
            NameBox.SetWatermark("Name");
            ActiveControl = MsgBox;
        }
        private void MessageBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode != Keys.Return)
                return;
            string lName = ObtainChatName();
            Packet Contents = new Packet
            {
                Name = lName,
                Message = MsgBox.Text,
                NameLen = lName.Length
            };
            new Task(() =>
            {
                // Отправляем текст на сервер
                try
                {
                    if(Contents.Message[0] == '/')
                    {
                        int pos = Contents.Message.IndexOf(' ');
                        string cmd = Contents.Message.Substring(1, pos - 1);
                        switch(cmd)
                        {
                            case "pm":
                                Contents.Code = Command.COMMAND_PRIVATE_MESSAGE;
                                Contents.Message = Contents.Message.Substring(pos + 1);
                                break;
                        }
                    }
                    else
                    {
                        Contents.Code = Command.COMMAND_COMMON_MESSAGE;
                    }
                    StaticMethods.Send(ref Contents);
                }
                catch (Exception ex)
                {
                    Invoke(new Action(() =>
                    {
                        MessageBox.Show("Error sending the message. Exception: " + ex.Message, "Error");
                    }));
                }
            }).Start();
            MsgBox.Clear();
            e.SuppressKeyPress = true;
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
            return string.IsNullOrWhiteSpace(NameBox.Text) ? "Anonymous" : NameBox.Text;
        }
        private void Form1_Shown(object sender, EventArgs e)
        {
            OnDisconnect();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            StaticMethods.Disconnect();
            OnDisconnect();
        }
    }
}
