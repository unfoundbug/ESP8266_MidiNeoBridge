using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace SerialCommand
{
    
    public partial class Form1 : Form
    {
        System.Net.Sockets.UdpClient udpReciever;
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            serialPort1 = new System.IO.Ports.SerialPort("COM4", 115200);
            udpReciever = new System.Net.Sockets.UdpClient(8282);
            backgroundWorker1.RunWorkerAsync();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                if (textBox1.Lines.Count() > textBox1.Height / textBox1.Font.Height)
                    textBox1.Lines = textBox1.Lines.Skip(1).ToArray();
                while (serialPort1.BytesToRead > 0)
                {
                    char c = (char)serialPort1.ReadChar();
                    textBox1.Text += c;
                    if (c == '\n') return;
                }
            }
            else
                try
                {
                    serialPort1.Open();
                }
                catch (Exception ex)
                {
                };
        }

        private void button1_Click(object sender, EventArgs e)
        {
            timer1.Enabled = false;
            serialPort1.Close();
            System.Diagnostics.Process np = new System.Diagnostics.Process();
            System.Diagnostics.ProcessStartInfo pi = new System.Diagnostics.ProcessStartInfo("cmd.exe");
            np.StartInfo = pi;
            np.StartInfo.FileName = "cmd.exe";
            np.StartInfo.Arguments = "/C flash.bat";
            np.StartInfo.WorkingDirectory = "C:\\SVN\\ESP\\trunk\\";
            np.StartInfo.UseShellExecute = false;
            np.StartInfo.RedirectStandardOutput = true;
            np.Start();
            textBox1.Text += np.StandardOutput.ReadToEnd();
            np.WaitForExit();
            serialPort1.Open();
            timer1.Enabled = true ;
        }

        private void timer2_Tick(object sender, EventArgs e)
        {
            
        }

        private void backgroundWorker1_DoWork(object sender, DoWorkEventArgs e)
        {
            System.Net.IPEndPoint ipRemote = new System.Net.IPEndPoint(System.Net.IPAddress.Any, 8282);
            while (true)
            {
                broadcastPacket bPacket = new broadcastPacket();
                bPacket.bData = udpReciever.Receive(ref ipRemote);
                bPacket.ipRemote = ipRemote;
                bPacket.dt = DateTime.Now;
                backgroundWorker1.ReportProgress(0, bPacket);
            }
        }

        private void backgroundWorker1_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            broadcastPacket bPacket = (broadcastPacket)(e.UserState);
            label1.Text = bPacket.ipRemote.ToString();
            label2.Text = bPacket.dt.ToString();
            string strDataRecieved = "";
            foreach (byte b in bPacket.bData)
            {
                strDataRecieved += ((char)b).ToString();
            }
            label3.Text = strDataRecieved;
        }

    }
    public class broadcastPacket
    {
        public DateTime dt;
        public byte[] bData;
        public System.Net.IPEndPoint ipRemote;
    };
}
