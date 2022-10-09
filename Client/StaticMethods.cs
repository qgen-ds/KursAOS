using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;
using System.Collections.Generic;

namespace Client
{
    public static class StaticMethods
    {
        private const uint ECM_FIRST = 0x1500;
        private const uint EM_SETCUEBANNER = ECM_FIRST + 1;

        [DllImport("ClientDLL.dll", CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool Connect([MarshalAs(UnmanagedType.LPWStr)] string address, ushort port);

        [DllImport("ClientDLL.dll", CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.LPWStr)]
        public static extern void GetLocalIP([MarshalAs(UnmanagedType.LPWStr)] StringBuilder addr);

        [DllImport("ClientDLL.dll", CharSet = CharSet.Auto)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern void Send([MarshalAs(UnmanagedType.LPWStr)] string packet);

        [DllImport("ClientDLL.dll")]
        public static extern void Disonnect();

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool HideCaret(IntPtr hWnd);
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
        public static void Escape(string str)
        {
            str.Replace("#", "<num>").Replace("&", "<and>");
        }

        public static void Encode(this List<string> L)
        {
            L.ForEach((s) => { Escape(s); });
        }

        [DllImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.U4)]
        public static extern uint GetLastError();
    }
}
