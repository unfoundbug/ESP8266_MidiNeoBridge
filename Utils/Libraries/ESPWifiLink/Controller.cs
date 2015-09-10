using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;

namespace ESPWifiLink
{
    public class WifiController
    {
        public override string ToString()
        {
            if (m_bNetworkOK)
                return "Connected to: " + m_strTarget;
            else
                return "Connection Failure";
        }
        bool bRunning;
        System.Net.Sockets.Socket m_sSocket;
        int m_iNetworkState = 0;
        bool m_bNetworkOK;
        bool bDrawing;
        Color[] m_cDrawColours;
        public void SetDrawDetails(Color[] cColours)
        {
            if (!bDrawing)
                m_cDrawColours = cColours;
        }
        public WifiController()
        {
            m_strTarget = "";
            bRunning = false;
        }
        string m_strTarget;
        void SetTarget(string strTar)
        {
            m_strTarget = strTar;
        }
        void ProcessBroadCast()
        {
            System.Net.IPEndPoint ipRemote = new System.Net.IPEndPoint(System.Net.IPAddress.Any, 8282);
            System.Net.Sockets.UdpClient udpReciever = new System.Net.Sockets.UdpClient(8282);
            udpReciever.Client.ReceiveTimeout = (5);
            while (bRunning)
            {
                try
                {
                    m_strTarget = "127.0.0.1";
                    udpReciever.Receive(ref ipRemote);
                    m_strTarget = ipRemote.Address.MapToIPv4().ToString();
                }
                catch (Exception ex)
                {
                }
            }
        }
        byte bSentData = 0;
        void ProcessOutput()
        {
            while (bRunning)
            {
                if (m_cDrawColours == null)
                {
                    System.Threading.Thread.Sleep(200);
                    continue;
                }
                System.Threading.Thread.Sleep(25);
                int iLen = (m_cDrawColours.Length * 3) + 4;
                int iCurPoint = 6;

                byte[] bBuffer = new byte[iLen + 2];
                iCurPoint = 0;
                bBuffer[iCurPoint++] = (byte)(iLen >> 8);
                bBuffer[iCurPoint++] = (byte)((iLen & 0xFF));
                bBuffer[iCurPoint++] = (byte)(0);
                bBuffer[iCurPoint++] = (byte)(4);
                bDrawing = true;
                for (int i = 0; i < m_cDrawColours.Length; ++i)
                {
                    bBuffer[iCurPoint++] = (byte)(m_cDrawColours[i].R);
                    bBuffer[iCurPoint++] = (byte)(m_cDrawColours[i].G);
                    bBuffer[iCurPoint++] = (byte)(m_cDrawColours[i].B);
                }
                bDrawing = false;
                try
                {
                    if (m_sSocket == null || !m_sSocket.Connected)
                    {
                        if (m_strTarget.Length == 0) continue;
                        m_sSocket = new System.Net.Sockets.Socket(System.Net.Sockets.AddressFamily.InterNetwork, System.Net.Sockets.SocketType.Dgram, System.Net.Sockets.ProtocolType.Udp);
                        m_sSocket.Connect(m_strTarget, 8081);
                    }
                    System.Diagnostics.Stopwatch sw = new System.Diagnostics.Stopwatch();
                    sw.Start();
                    m_sSocket.Send(bBuffer);
                    m_iNetworkState = 1;
                    byte[] bRecieve = new byte[5];
                    System.Threading.Thread.Sleep(1);
                    sw.Stop();
                    long ms = sw.ElapsedMilliseconds;
                    m_bNetworkOK = true;
                    m_iNetworkState = 2;
                }
                catch (Exception ex)
                {
                    m_sSocket.Close();
                    m_strTarget = "";
                    m_bNetworkOK = false;
                    m_sSocket = null;
                    m_iNetworkState = 0;
                }
            }
        }

        System.Threading.ThreadStart m_tsNetwork;
        System.Threading.Thread m_nTNetwork;
        System.Threading.ThreadStart m_tsBroadcast;
        System.Threading.Thread m_nTBroadcast;

        public void Start()
        {
            bRunning = true;
            m_tsNetwork = new System.Threading.ThreadStart(this.ProcessOutput);
            m_nTNetwork = new System.Threading.Thread(m_tsNetwork);
            m_nTNetwork.Start();
            m_tsBroadcast = new System.Threading.ThreadStart(this.ProcessBroadCast);
            m_nTBroadcast = new System.Threading.Thread(m_tsBroadcast);
            m_nTBroadcast.Start();
        }
        public void Stop()
        {
            bRunning = false;
        }
    }
}
