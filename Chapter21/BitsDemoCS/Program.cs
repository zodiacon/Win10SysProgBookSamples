using bits;
using System;
using System.Threading;

namespace BitsDemoCS {
    class Program {
        static void Main(string[] args) {
            var mgr = new BackgroundCopyManager();
            mgr.CreateJob("My managed Job", BG_JOB_TYPE.BG_JOB_TYPE_DOWNLOAD, out var guid, out var job);
            job.AddFile("http://speedtest.ftp.otenet.gr/files/test10Mb.db", @"c:\temp\test.db");
            job.Resume();

            BG_JOB_STATE state;
            while(true) {
                job.GetState(out state);
                if (state == BG_JOB_STATE.BG_JOB_STATE_TRANSFERRED || state == BG_JOB_STATE.BG_JOB_STATE_ERROR)
                    break;
                Thread.Sleep(500);
                Console.Write(".");
            }
            Console.WriteLine();
            if (state == BG_JOB_STATE.BG_JOB_STATE_TRANSFERRED) {
                job.Complete();
                Console.WriteLine("Transferred successfully.");
            }
            else {
                Console.WriteLine("Error!");
            }
        }
    }
}
