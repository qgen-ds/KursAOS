using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Collections.Generic;

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
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern void Send([MarshalAs(UnmanagedType.LPWStr)] string packet);

        [DllImport("ClientDLL.dll")]
        public static extern void Disconnect();

        [DllImport("ClientDLL.dll")]
        public static extern void FreeBlock(IntPtr Block);

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
        public static string Escape(string str)
        {
            return str.Replace("#", "<num>").Replace("&", "<and>");
        }

        public static string Unescape(string str)
        {
           return str.Replace("<num>", "#").Replace("<and>", "&");

        }
        public static void Encode(this List<string> L)
        {
            //L.ForEach((s) => { s = Escape(s); });
            for(int i = 0; i < L.Count; i++)
            {
                L[i] = Escape(L[i]);
            }
        }
        public static string[] Decode(string[] strings)
        {
            //Array.ForEach(strings, (string s) => { s = Unescape(s); });
            string[] ret = new string[strings.Length];
            for (int i = 0; i < strings.Length; i++)
            {
                ret[i] = Unescape(strings[i]);
            }
            return ret;
        }
    }
}
