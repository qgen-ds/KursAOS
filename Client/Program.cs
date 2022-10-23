using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace Client
{
    static class Program
    {
        /// <summary>
        /// Главная точка входа для приложения.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new Form1());
        }
    }
    [StructLayout(LayoutKind.Sequential)]
    public struct WSABUF
    {
        [MarshalAs(UnmanagedType.U4)]
        public uint len;
        public IntPtr buf;
    }
    [StructLayout(LayoutKind.Sequential)]
    public struct RECVPARAM
    {
        public IntPtr EventRaiser;
        public IntPtr OnDisconnect;
        public WSABUF Buf;
        [MarshalAs(UnmanagedType.I1)]
        public byte MarkForDelete;
    }
    public delegate void EventRaiserDelegate();
}
