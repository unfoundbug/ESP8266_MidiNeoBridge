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

        byte[] bExpectedHeader = { 0x4d, 0x54, 0x68, 0x64, 0x00, 0x00, 0x00, 0x06 };
        byte[] bExpectedTrack = { 0x4d, 0x54, 0x72, 0x6B };
        int uiTimeScale;
        int uiTrackCount;
        byte[][] lbTrackData;
        List<List<MidiEvent>> llMidiEvents;

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

		private void button1_Click_1(object sender, EventArgs e)
		{

			openFileDialog1.ShowDialog();
			byte [] bHeader = new byte[14];
			byte[] bTrackHeader = new byte[8];
            llMidiEvents = new List<List<MidiEvent>>();
			if (openFileDialog1.CheckFileExists)
			{
				System.IO.Stream strFile = openFileDialog1.OpenFile();
				//read header
				strFile.Read(bHeader, 0, 14);
				if (memcmp(bHeader, bExpectedHeader, 8) == false)
				{
					return;
				}
				if (bHeader[9] != 1) //Expect simple multi-track recordings
				{
					return;
				}
				uint uiHeaderSize = bHeader[7];
				uiHeaderSize|=((uint)bHeader[6]) << 8;
				uiHeaderSize|=((uint)bHeader[5]) << 16;
				uiTrackCount = bHeader[11];
				uiTrackCount |= ((int)bHeader[10]) << 8;
				uiTimeScale = bHeader[13];
				uiTimeScale |= ((int)bHeader[12]) << 8;

				lbTrackData = new byte[uiTrackCount][];

				for(int i = 0; i < uiTrackCount; ++i)
				{
					strFile.Read(bTrackHeader, 0, 8);
					if(!memcmp(bTrackHeader, bExpectedTrack, 4))
					{ ///Track incorrect
						return;
					}
					uint uiTrackLength = bTrackHeader[7];
					uiTrackLength |= ((uint)bTrackHeader[6]) << 8;
					uiTrackLength |= ((uint)bTrackHeader[5]) << 16;
					uiTrackLength |= ((uint)bTrackHeader[4]) << 24;
					lbTrackData[i] = new byte[uiTrackLength];
                    strFile.Read(lbTrackData[i], 0, (int)uiTrackLength);
                    llMidiEvents.Add(new List<MidiEvent>());
                    //Start Pre-Process
					for (uint j = 0; j < uiTrackLength; ++j)
					{
						//Read Timing bytes
						int iTimeStep = lbTrackData[i][j] & 0X7f;
						{
							while ((lbTrackData[i][j] & (byte)0x80) != 0)
							{
								++j;
								iTimeStep = (iTimeStep << 7) + (lbTrackData[i][j] & (byte)0x80);
							}
						}
						++j;
						byte bCommand = lbTrackData[i][j];
                        ++j;
						if (bCommand == 0xff)
						{
                            //Meta Events
							byte bSubCommand = lbTrackData[i][j];
							++j;
								//dynamic length that can be ignored
								int iCommandLength = lbTrackData[i][j] & 0X7f;
								{
									while ((lbTrackData[i][j] & (byte)0x80) != 0)
									{
										++j;
										iTimeStep = (iTimeStep << 7) + (lbTrackData[i][j] & (byte)0x80);
									}
								}
								j += (uint)iCommandLength ;
						}
                        else if (bCommand == 0xF0)
                        {
                            byte bCommandLen = lbTrackData[i][j];
                            j += (uint)bCommandLen + 1;
                        }
                        else
                        {
                            MidiEvent nm = new MidiEvent();
                            byte nibCom = (byte)(bCommand & 0xF0);
                            byte nibChan = (byte)i;
                            nm.iCommand = nibChan | nibCom;
                            nm.iTime = iTimeStep;
                            nm.iData1 = lbTrackData[i][j];
                            ++j;
                            nm.iData2 = lbTrackData[i][j];
                            ++j;
                            if (nm.iCommand < 0xC0)
                            {
                                nm.iData3 = lbTrackData[i][j];
                                ++j;
                            }
                            else
                            {
                                nm.iData3 = -1;
                            }
                            llMidiEvents[i].Add(nm);
                        }
					}
				}
				//read track
				//read data
			}
		}

		private void button2_Click(object sender, EventArgs e)
		{

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
        public int iData3;
    };
}
