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
			dataGridView1.Rows.Clear();
			DialogResult dr = openFileDialog1.ShowDialog();
			if (dr != DialogResult.OK) return;
			byte [] bHeader = new byte[14];
			byte[] bTrackHeader = new byte[8];
            llMidiEvents = new List<List<MidiEvent>>();
			liMidiPlayTime = new List<int>(0);
			Dictionary<byte, string> strFRes = new Dictionary<byte, string>();
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
					liMidiPlayTime.Add(0);
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
					int bCurrentCommand = 0;
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
							if (bSubCommand == 0x03 || bSubCommand == 0x51)
							{
								string strValue;
								string strRes = "";
								if(bSubCommand == 0x03)
									strRes = i.ToString() + ": " + System.Text.Encoding.UTF8.GetString(lbTrackData[i], (int)j + 1, (int)iCommandLength) + "\n";
								else
								{
									Int32 tempo = lbTrackData[i][j+1] << 16;
									tempo += lbTrackData[i][j+2] << 8;
									tempo += lbTrackData[i][j+3];
									tempo = 60000000 / tempo;
									strRes = i.ToString() + ": " + tempo.ToString() + "\n";
								}
								if (strFRes.TryGetValue(bSubCommand, out strValue) == true)
									strFRes[bSubCommand] = strValue + strRes;
								else
									strFRes.Add(bSubCommand, strRes);
							}
							j += (uint)iCommandLength;
						}
                        else if (bCommand == 0xF0)
                        {
                            byte bCommandLen = lbTrackData[i][j];
                            j += (uint)bCommandLen + 1;
                        }
                        else
                        {
                            MidiEvent nm = new MidiEvent();
							nm.iCommand = bCommand;
							if (bCommand < 0x80)
							{
								--j;
								nm.iCommand = bCurrentCommand;
							}
							else
							{
								nm.iCommand = bCommand;
								bCurrentCommand = bCommand;
							}

                            nm.iTime = iTimeStep;
                            nm.iData1 = lbTrackData[i][j];
                            if (nm.iCommand < 0xE0 && bCommand > 0xBF)
                            {
								nm.iData2 = -1;
                            }
                            else
                            {
								++j;
								nm.iData2 = lbTrackData[i][j];
                            }
                            llMidiEvents[i].Add(nm);
                        }
					}
				}
				bool bEventsLeft;
				int iLastEvent = 0;
				int iNextEntry = 0;
				do
				{
					bEventsLeft = false;
					    int iClosestEvent = (int)0x0FFFFFFF;
					iNextEntry = -1;

					for (int i = 0; i < llMidiEvents.Count; ++i)
					{
						if (llMidiEvents[i].Count > 0)
						{
							int iNextEvent;
							iNextEvent = (llMidiEvents[i][0].iTime + liMidiPlayTime[i]);
							if (iClosestEvent >= iNextEvent)
							{
								iClosestEvent = iNextEvent;
								iNextEntry = i;
							}
						}
					}
					if (iNextEntry == -1)
					{
						int iBreakHere = 1;
					}
					int iWaitTime = (iClosestEvent - iLastEvent);
					iLastEvent = iClosestEvent;
					liMidiPlayTime[iNextEntry] += iClosestEvent - liMidiPlayTime[iNextEntry];
					string[] row = new string[] { iClosestEvent.ToString(), iWaitTime.ToString(), llMidiEvents[iNextEntry][0].iCommand.ToString("X2"), llMidiEvents[iNextEntry][0].iData1.ToString(), llMidiEvents[iNextEntry][0].iData2.ToString() };
					dataGridView1.Rows.Add(row);
					llMidiEvents[iNextEntry].RemoveAt(0);
					foreach (List<MidiEvent> lE in llMidiEvents)
						if (lE.Count > 0)
							bEventsLeft = true;
				} while (bEventsLeft);
				foreach (KeyValuePair<byte, string> pair in strFRes)
				{
					MessageBox.Show(pair.Value, pair.Key.ToString());
				}
			}
		}

		private void button2_Click(object sender, EventArgs e)
		{

            int iLen = (dataGridView1.Rows.Count * 5) + 2;
            int iCurPoint = 4;
            MessageBox.Show("Estimated data length: " + iLen.ToString());

            byte[] bBuffer = new byte[iLen+2];
            bBuffer[0] =(byte)(iLen >> 8);
            bBuffer[1] = (byte)((iLen & 0xFF));
            int iUsPerBeat = 8333;
            bBuffer[2] = (byte)((iUsPerBeat & 0xFF00) >> 8);
            bBuffer[3] = (byte)((iUsPerBeat & 0xFF));
			foreach (DataGridViewRow row in dataGridView1.Rows)
			{
                string strCell1, strCell2, strCell3, strCell4;
                strCell1 = row.Cells[1].Value.ToString();
                strCell2 = row.Cells[2].Value.ToString();
                strCell3 = row.Cells[3].Value.ToString();
                strCell4 = row.Cells[4].Value.ToString();
                int iTime = int.Parse(strCell1);
                bBuffer[iCurPoint++] = (byte)((iTime & 0xF0) << 8);
                bBuffer[iCurPoint++] = (byte) (iTime & 0xF);
                bBuffer[iCurPoint++] = (byte)int.Parse(strCell2, System.Globalization.NumberStyles.HexNumber);
                bBuffer[iCurPoint++] = (byte)int.Parse(strCell3);
                bBuffer[iCurPoint++] = (byte)int.Parse(strCell4);
			}
            System.Net.Sockets.Socket s = new System.Net.Sockets.Socket(System.Net.Sockets.AddressFamily.InterNetwork, System.Net.Sockets.SocketType.Stream, System.Net.Sockets.ProtocolType.Tcp);
            s.Connect("192.168.43.178", 8081);
            s.Send(bBuffer);
            s.Close();

		}

        private void button3_Click(object sender, EventArgs e)
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
    };
}
