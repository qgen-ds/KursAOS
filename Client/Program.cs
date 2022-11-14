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
    [StructLayout(LayoutKind.Sequential)]
    public struct Packet
    {
        [MarshalAs(UnmanagedType.I4)]
        public Command Code;
        [MarshalAs(UnmanagedType.I4)]
        public int NameLen;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string Name;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string Message;
    }

    //public struct PMPacket
    //{
    //    public Packet pkt;
    //    public uint ID;
    //    public string addr;
    //}
    public enum Command
    {
        COMMAND_COMMON_MESSAGE = 1,
        COMMAND_PRIVATE_MESSAGE = 2,
        COMMAND_PM_RETURN = 3,
        COMMAND_ERROR = -1
    }
    public delegate void EventRaiserDelegate();
}