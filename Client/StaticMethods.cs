using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Net;

namespace Client
{
    public static class StaticMethods
    {
        private const uint ECM_FIRST = 0x1500;
        private const uint EM_SETCUEBANNER = ECM_FIRST + 1;

        [DllImport("ClientDLL.dll", CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool Connect([MarshalAs(UnmanagedType.LPWStr)] string address, ushort port, ref RECVPARAM param);

        [DllImport("ClientDLL.dll", CharSet = CharSet.Auto)]
        public static extern void Send(ref Packet packet);

        [DllImport("ClientDLL.dll")]
        public static extern void Disconnect();

        [DllImport("ClientDLL.dll")]
        public static extern void FreeBlock(IntPtr Block);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.U4)]
        private static extern uint HideCaret(IntPtr hWnd);
        public static void HideCaret(this TextBox textBox)
        {
            HideCaret(textBox.Handle);
        }

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = false)]
        private static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, uint wParam, [MarshalAs(UnmanagedType.LPWStr)] string lParam);

        public static void SetWatermark(this TextBox textBox, string watermarkText)
        {
            SendMessage(textBox.Handle, EM_SETCUEBANNER, 1, watermarkText);
        }
        //public static PMPacket PMParseCommon(ref IntPtr ptr, ref IntPtr name, ref RECVPARAM RecvBuf, ref Packet pkt)
        //{
        //    PMPacket ret = new PMPacket();
        //    int[] data = new int[3];
        //    ret.pkt = pkt;
        //    Marshal.Copy(ptr, data, 0, 3);
        //    ret.ID = Convert.ToUInt32(IPAddress.NetworkToHostOrder(data[0]));
        //    int addrlen = IPAddress.NetworkToHostOrder(data[1]);
        //    ret.pkt.NameLen = IPAddress.NetworkToHostOrder(data[2]);
        //    name = RecvBuf.Buf.buf + Convert.ToInt32(RecvBuf.Buf.len) - ret.pkt.NameLen * sizeof(char);
        //    ptr += 12;
        //    ret.addr = Marshal.PtrToStringAuto(ptr, addrlen);
        //    ptr += addrlen * sizeof(char);
        //    int msglen = name.ToInt32() > ptr.ToInt32() ? (name.ToInt32() - ptr.ToInt32()) / sizeof(char) :
        //        (Convert.ToInt32(RecvBuf.Buf.len) - ptr.ToInt32()) / sizeof(char);
        //    ret.pkt.Message = Marshal.PtrToStringAuto(ptr, msglen);
        //    return ret;
        //}
    }
}