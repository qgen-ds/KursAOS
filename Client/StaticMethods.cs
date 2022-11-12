using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

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
    }
}