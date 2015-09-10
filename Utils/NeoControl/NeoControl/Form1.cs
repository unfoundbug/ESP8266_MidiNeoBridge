using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
namespace SerialCommand
{
    
    public partial class Form1 : Form
    {

        byte[] bExpectedHeader = { 0x4d, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06 };
        byte[] bExpectedTrack = { 0x4d, 0x54, 0x72, 0x6B };
        int uiTimeScale;
        int uiTrackCount;
        byte[][] lbTrackData;
        List<List<MidiEvent>> llMidiEvents;
		List<int> liMidiPlayTime;
		public bool memcmp(byte[] A, byte[] B, int iLen)
		{
			int iCurPoint = 0;
			while (iCurPoint < iLen)
			{
				if (A[iCurPoint] != B[iCurPoint])
					return false;
				++iCurPoint;
			}
			return true;
		}
        System.Net.Sockets.UdpClient udpReciever;
        public Form1()
        {
            InitializeComponent();

            
        }
        System.Threading.ThreadStart m_tsNetwork;
        System.Threading.Thread m_nTNetwork;
        System.Threading.ThreadStart m_tsSerial;
        System.Threading.Thread m_nTSerial;
        
        private void Form1_Load(object sender, EventArgs e)
        {
            serialPort1 = new System.IO.Ports.SerialPort("COM4", 78400);
            udpReciever = new System.Net.Sockets.UdpClient(8282);
            backgroundWorker1.RunWorkerAsync();
            m_tsNetwork = new System.Threading.ThreadStart(this.ProcessNetwork);
            m_nTNetwork = new System.Threading.Thread(m_tsNetwork);
            m_nTNetwork.Start();
            m_tsSerial = new System.Threading.ThreadStart(this.ProcessNetwork);
            m_nTSerial = new System.Threading.Thread(m_tsNetwork);
            m_nTSerial.Start();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (bSerialOK)
                pictureBox1.BackColor = Color.Green;
            else
                pictureBox1.BackColor = Color.Red;
            if (bNetworkOK)
                pictureBox2.BackColor = Color.Green;
            else
                pictureBox2.BackColor = Color.Red;
            if (m_iNetworkState == 0)
                pictureBox3.BackColor = Color.Red;
            else if (m_iNetworkState == 1)
                pictureBox3.BackColor = Color.Yellow;
            else
                pictureBox3.BackColor = Color.Green;
            if (serialPort1.IsOpen)
            {
                int iBytesRead = 0;
                if (textBox1.Lines.Count() > textBox1.Height / textBox1.Font.Height)
                    textBox1.Lines = textBox1.Lines.Skip(1).ToArray();
                while (serialPort1.BytesToRead > 0 && iBytesRead < 20)
                {
                    char c = (char)serialPort1.ReadChar();
                    textBox1.Text += c;
                    if (c == '\n') return;
                    ++iBytesRead;
                }
            }
            else
                try
                {
                    serialPort1.Open();
                    bSerialOK = true;
                }
                catch (Exception ex)
                {
                    bSerialOK = false;
                };
        }

        private void button1_Click(object sender, EventArgs e)
        {
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
        }
        bool bChanged = false;
        bool NextCol = false;
        bool bSerialOK = false;
        bool bNetworkOK = false;
        Color cToSend1;
        Color cToSend2;
        System.Net.Sockets.Socket m_sSocket;
        int m_iNetworkState = 0;
        private void ProcessNetwork()
        {
            bNetworkOK = false; 
            while (true)
            {
                System.Threading.Thread.Sleep(250);
                int iLedCount = 150;
                if (!bChanged)
                    continue;
                bChanged = false;
                int iLen = (iLedCount * 3) + 4;
                int iCurPoint = 6;

                byte[] bBuffer = new byte[iLen + 2];
                bBuffer[0] = (byte)(iLen >> 8);
                bBuffer[1] = (byte)((iLen & 0xFF));
                bBuffer[2] = (byte)(0);
                bBuffer[3] = (byte)(4);
                bool bSelect = false;
                for (int i = 0; i < iLedCount; ++i)
                {
                    if (bSelect)
                    {
                        bBuffer[iCurPoint++] = (byte)(cToSend1.R);
                        bBuffer[iCurPoint++] = (byte)(cToSend1.G);
                        bBuffer[iCurPoint++] = (byte)(cToSend1.B);
                    }
                    else
                    {
                        bBuffer[iCurPoint++] = (byte)(cToSend2.R);
                        bBuffer[iCurPoint++] = (byte)(cToSend2.G);
                        bBuffer[iCurPoint++] = (byte)(cToSend2.B);
                    }
                    bSelect = !bSelect;

                }
                try
                {
                    if (m_sSocket == null || !m_sSocket.Connected)
                    {
                        m_sSocket = new System.Net.Sockets.Socket(System.Net.Sockets.AddressFamily.InterNetwork, System.Net.Sockets.SocketType.Stream, System.Net.Sockets.ProtocolType.Tcp);
                        m_sSocket.ReceiveTimeout = 100;
                        m_sSocket.SendTimeout = 100;
                        m_sSocket.Connect(label1.Text, 8081);
                    }
                    System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();
                    sw.Start();
                    m_sSocket.Send(bBuffer);
                    m_iNetworkState = 1;
                    byte[] bRecieve = new byte[5];
                    System.Threading.Thread.Sleep(10);
                    int amnt = 0;
                    while(amnt < 5)
                        amnt += m_sSocket.Receive(bRecieve);
                    sw.Stop();
                    long ms = sw.ElapsedMilliseconds;
                    bNetworkOK = true;
                    m_iNetworkState = 2;
                }
                catch (Exception ex)
                {
                    bNetworkOK = false;
                    m_sSocket = null;
                    m_iNetworkState = 0;
                }
            }
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
            label1.Text = bPacket.ipRemote.Address.ToString();
            label2.Text = bPacket.dt.ToString();
            string strDataRecieved = "";
            foreach (byte b in bPacket.bData)
            {
                strDataRecieved += ((char)b).ToString();
            }
            label3.Text = strDataRecieved;
        }

		private void button1_Click_1(object sender, EventArgs e)
		{
			
		}

		private void button2_Click(object sender, EventArgs e)
		{
            colorDialog1.ShowDialog();
            vScrollBar1.Value = colorDialog1.Color.R;
            vScrollBar2.Value = colorDialog1.Color.G;
            vScrollBar3.Value = colorDialog1.Color.B;
            SetNextColour();
            bChanged = true;
		}

        private void button3_Click(object sender, EventArgs e)
        {

        }

        private void vScrollBar1_Scroll(object sender, ScrollEventArgs e)
        {
            SetNextColour();
        }
        private void SetNextColour()
        {

            if (NextCol)
            {
                cToSend1 = Color.FromArgb(255, vScrollBar1.Value, vScrollBar2.Value, vScrollBar3.Value);
            }
            else
            {
                cToSend2 = Color.FromArgb(255, vScrollBar1.Value, vScrollBar2.Value, vScrollBar3.Value);
            }
            NextCol = !NextCol;
            bChanged = true;
        }

        private void vScrollBar2_Scroll(object sender, ScrollEventArgs e)
        {
            SetNextColour();
        }

        private void vScrollBar3_Scroll(object sender, ScrollEventArgs e)
        {
            SetNextColour();
        }
    }
    public class broadcastPacket
    {
        public DateTime dt;
        public byte[] bData;
        public System.Net.IPEndPoint ipRemote;
    };
    public class MidiEvent
    {
        public int iTime;
        public int iCommand;
        public int iData1;
        public int iData2;
    };
}
