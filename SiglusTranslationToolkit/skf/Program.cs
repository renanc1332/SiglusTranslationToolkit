using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace skf
{
    class Program
    {
        static int GetPID()
        {
            while (true)
            {
                string Name="";
                Process[] Procs = Process.GetProcesses();
                foreach (Process Proc in Procs)
                {
                    Name = Proc.ProcessName;
                    if (Name.Contains("SiglusEngine"))
                    {
                        return Proc.Id;
                    }
                }
            }
        }

        static void Main(string[] args)
        {
            System.Console.WriteLine("Please run a siglus engine exe. The process name must contain 'SiglusEngine'.");
            System.Console.WriteLine("Process Finding...");
            int PID = GetPID();
            if (PID != -1)
            {
                System.Console.WriteLine("Process Found.");
                System.Console.WriteLine("Keys Finding...");
                try
                {
                    SiglusKeyFinder.KeyFinder.Key[] Keys = SiglusKeyFinder.KeyFinder.ReadProcess(PID);
                    string MSG = "Keys found:\n";
                    foreach (SiglusKeyFinder.KeyFinder.Key Key in Keys)
                        MSG += Key.KeyStr + "\n---------------------------\n";
                    System.Console.WriteLine(MSG);

                    StreamReader sr = new StreamReader(new FileStream("stt.cfg", FileMode.Open));

                    List<string> fileLines = new List<string>();
                    while (sr.EndOfStream == false)
                    {
                        fileLines.Add(sr.ReadLine());
                    }

                    sr.Close();

                    for (int i = 0; i < fileLines.Count; ++i)
                    {
                        fileLines[i].TrimStart(' ');
                        if (fileLines[i].Length > 3 && fileLines[i].Substring(0, 3) == "key")
                            fileLines[i] = "key=" + Keys[0].KeyStr;
                    }

                    if (Keys.Length > 1)
                    {
                        for (int i = 1; i < Keys.Length; ++i)
                        {
                            fileLines.Add("// Alternate key" + i + ": " + Keys[i].KeyStr);
                        }
                    }

                    StreamWriter sw = new StreamWriter(new FileStream("stt.cfg", FileMode.Create));

                    foreach (string line in fileLines)
                    {
                        sw.WriteLine(line);
                    }

                    sw.Close();

                    System.Console.WriteLine("keys wrote in stt.cfg.");
                }
                catch
                {
                    System.Console.WriteLine("Error. Process crashed or stt.cfg is corrupted.");
                }
            }


            System.Console.WriteLine("\n==Press Any Key==");
            Console.ReadKey(false);
        }
    }
}
