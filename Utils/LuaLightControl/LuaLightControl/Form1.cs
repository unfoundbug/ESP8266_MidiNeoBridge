using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.IO.Ports;
using ESPWifiLink;
using NLua;

namespace LuaLightControl
{
    public partial class Form1 : Form
    {
        WifiController m_wWifiInteraction = new WifiController();
        LuaInterface lInterface = new LuaInterface();
        public Form1()
        {
            InitializeComponent();
        }

        private void textBox1_TextChanged(object sender, EventArgs e)
        {

        }

        private void btnLoadScripts_Click(object sender, EventArgs e)
        {
            cbEnableDraw.Checked = false;
            lstScripts.Items.Clear();
            //if (fbdFolderSelector.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                fbdFolderSelector.SelectedPath = "C:\\SVN\\ESP\\trunk\\Utils\\Lua";
                //fbdFolderSelector.SelectedPath = "E:\\Shares\\SyncDocuments\\Source\\Lua";
                //fbdFolderSelector.SelectedPath = "G:\\Lua";
                Directory.SetCurrentDirectory(fbdFolderSelector.SelectedPath);
                string strDirectory = fbdFolderSelector.SelectedPath;
                string[] strFiles = Directory.GetFiles(strDirectory, "*.lua");
                foreach (string file in strFiles)
                {
                    string strFile = Path.GetFileName(file);
                    if (file.Contains("CLRPackage"))
                        continue;
                    lstScripts.Items.Add(strFile);
                }
                lblDirectory.Text = strDirectory;
                
            }
        }

        private void lstScripts_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (bgWorker.IsBusy)
            {
                bgWorker.CancelAsync();

                while (bgWorker.IsBusy)
                    Application.DoEvents();   
            }
            textBox1.Text = "";
            lblDirectory.Text = "Scripts Not loaded";
            lInterface.reset();
            
            try
            {
                lInterface.DoFile(fbdFolderSelector.SelectedPath + "\\" + lstScripts.SelectedItem.ToString());
                lblDirectory.Text = lstScripts.SelectedItem.ToString();
                numericUpDown1_ValueChanged(null, null);
                bgWorker.RunWorkerAsync();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Source + '\n' + ex.ToString() + "\n\n" + ex.InnerException, "Error occurred");
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            try
            {
                lInterface.DoString("displayConfig()");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Source + '\n' +  ex.ToString() + "\n\n" + ex.InnerException, "Error occurred");
            }
        }
        List<PictureBox> m_pbList = new List<PictureBox>();
        private void Form1_Load(object sender, EventArgs e)
        {
            m_pbList.Clear();
            for (int i = 0; i < 60; i++)
            {
                PictureBox pb = new PictureBox();
                int y = i%10;
                int x = (i - y) / 10;
                pb.Location = new Point(258+(20*x), 60+(y*20));
                pb.Size = new Size(20, 20);
                pb.BackColor = Color.Black;
                this.Controls.Add(pb);
                m_pbList.Add(pb);
            }
            m_wWifiInteraction.Start();
            
        }
        int iFrame = 0;
        private void button2_Click(object sender, EventArgs e)
        {
            Color[] drawResult = lInterface.DrawPixels(iFrame++);
            for (int i = 0; i < drawResult.Count(); ++i)
            {
                if (i < m_pbList.Count())
                    m_pbList[i].BackColor = drawResult[i];
            }
            m_wWifiInteraction.SetDrawDetails(drawResult);
        }

        private void numericUpDown1_ValueChanged(object sender, EventArgs e)
        {
            lInterface.DoString("pixelCount = " + numericUpDown1.Value.ToString());
        }

        private void bgWorker_DoWork(object sender, DoWorkEventArgs e)
        {
            while (!bgWorker.CancellationPending)
            {
                if (cbEnableDraw.Checked)
                    button2_Click(null, null);
                System.Threading.Thread.Sleep((int)numericUpDown2.Value);
            }
            e.Cancel = true;
            return;
        }
        private void bgWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {

        }

        private void cbEnableDraw_CheckedChanged(object sender, EventArgs e)
        {
            if (cbEnableDraw.Checked == false)
            {
                //sInterface.ClearStrip();
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            try
            {
                if(textBox2.Text.Length > 0)
                lInterface.DoString(textBox2.Text);
            }
            catch (Exception ex)
            {
                textBox1.Text = ex.ToString() + "\n" + ex.InnerException;
            }
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            lblNetwork.Text = m_wWifiInteraction.ToString();
        }

        private void textBox2_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter)
                button3_Click(null, null);
        }
    }
    public class LuaInterface
    {
        Lua nLua;
        System.Threading.Mutex mInterfaceLock = new System.Threading.Mutex();
        System.Collections.Generic.Dictionary<string, LuaFunction> m_funcMap;
        public LuaInterface()
        {
            reset();
        }
        public object[] DoString(string strScript)
        {
            mInterfaceLock.WaitOne();
            object[] ret;
            try
            {
                ret = nLua.DoString(strScript);
            }
            catch (Exception ex)
            {
                ret = null;
            }
            mInterfaceLock.ReleaseMutex();
            return ret;
        }
        public object[] DoFile(string strFileName)
        {
            mInterfaceLock.WaitOne();
            object[] ret = nLua.DoFile(strFileName);
            mInterfaceLock.ReleaseMutex();
            return ret;

        }
        public void reset()
        {
            nLua = new Lua();
            m_funcMap = new Dictionary<string, LuaFunction>();
            DoString("pixelCount = 0");
        }
        LuaFunction getFunction(string strName)
        {
            LuaFunction lFunction;
            if (m_funcMap.ContainsKey(strName))
                lFunction = m_funcMap[strName];
            else
            {
                lFunction = nLua[strName] as LuaFunction;
                m_funcMap.Add(strName, lFunction);
            }
            return lFunction;
        }
        public Color[] DrawPixels(int frame)
        {
            mInterfaceLock.WaitOne();
            Color[] returnStruct;
            try
            {
                LuaFunction pixelCall = getFunction("drawPixels");
                object returnList = pixelCall.Call(frame, frame).First();
                LuaTable ColourSets = (LuaTable)returnList;
                System.Collections.IEnumerator tCounter = ColourSets.Values.GetEnumerator();
                tCounter.MoveNext();
                LuaTable tCol = (LuaTable)tCounter.Current;
                int pixelsReturned = tCol.Values.Count;
                returnStruct = new Color[pixelsReturned];
                double[] dBuffer = new double[3 * pixelsReturned];
                int oSet = 0;
                foreach (LuaTable tColourSet in ColourSets.Values)
                {
                    foreach (double val in tColourSet.Values)
                    {
                        dBuffer[oSet++] = val;
                    }
                }
                for (int i = 0; i < pixelsReturned; ++i)
                {
                    returnStruct[i] = Color.FromArgb((int)(dBuffer[i]), (int)(dBuffer[i + pixelsReturned]), (int)(dBuffer[i + pixelsReturned + pixelsReturned]));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Source + '\n' + ex.ToString() + "\n\n" + ex.InnerException, "Error Occured");
                 returnStruct = new Color[0];
            }
            mInterfaceLock.ReleaseMutex();
            return returnStruct;
        }

        public object[] callFunction(string strName, object[] variable)
        {
            mInterfaceLock.WaitOne();
            object[] ret = getFunction(strName).Call(variable);
            mInterfaceLock.ReleaseMutex();
            return ret;
        }
    }
    public class SerialInterface
    {
        bool bLockTransfer;

     }
}
