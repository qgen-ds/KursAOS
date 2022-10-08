using System;
using System.Windows.Forms;

namespace Client
{
    class DialogEventArgs : EventArgs
    {
        public IWin32Window OwnerWindow { get; }
        public DialogEventArgs()
        {
            OwnerWindow = null;
        }
        public DialogEventArgs(IWin32Window owner)
        {
            OwnerWindow = owner;
        }
    }
}
