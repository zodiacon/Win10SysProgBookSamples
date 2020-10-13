using System;
using System.Runtime.InteropServices;

namespace CalcCS {
    [ComImport, Guid("7020B8D9-CEFE-46DC-89D7-F0261C3CDE66")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IRPNCalculator {
        void Push(double value);
        double Pop();
        void Add();
        void Subtract();
    };

    [ComImport, Guid("FA523D4E-DB35-4D0B-BD0A-002281FE3F31")]
    class RPNCalculator { }

    static class Program {
        [STAThread]
        static void Main(string[] args) {
            IRPNCalculator calc = (IRPNCalculator)new RPNCalculator();
            calc.Push(10);
            calc.Push(20);
            calc.Add();
            Console.WriteLine(calc.Pop());
        }
    }
}
