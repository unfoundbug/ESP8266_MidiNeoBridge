using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO.Ports;
namespace LuaLightControl
{
    public partial class SelectSerial : Form
    {
        public SelectSerial(SerialInterface host)
        {
            m_host = host;
            InitializeComponent();
        }
        SerialInterface m_host;
        private void SelectSerial_Load(object sender, EventArgs e)
        {
            listBox1.Items.Clear();
            listBox1.Items.AddRange(SerialPort.GetPortNames());
        }

        private void btnOK_Click(object sender, EventArgs e)
        {
            m_host.ConnectTo((string)listBox1.SelectedItem);
            this.Close();
        }

        private void btnCncl_Click(object sender, EventArgs e)
        {
            m_host.ConnectTo(null);
            this.Close();
        }
    }
}
