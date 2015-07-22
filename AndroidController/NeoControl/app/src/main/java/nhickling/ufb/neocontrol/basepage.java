package nhickling.ufb.neocontrol;

import android.graphics.Color;
import android.provider.ContactsContract;
import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.ImageButton;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Random;

public class basepage extends ActionBarActivity {

    ImageButton m_ColourDisplay;
    SeekBar m_sBarR;
    SeekBar m_sBarG;
    SeekBar m_sBarB;
    TextView m_tBroadcastResult;
    Button m_bModeChange;

    int[] bColour= new int[3];

    Thread m_tBroadcastReciever;
    Thread m_tColourSend;
    String m_sTargetNeo;
    Boolean m_bOkToSend;
    DatagramSocket m_sBroadcastSocket;

    enum LightOutMode{
        RGB, RAINBOW, FLASH, MODE_COUNT
    }
    LightOutMode m_eLightOut;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_basepage);

        m_ColourDisplay = (ImageButton) findViewById((R.id.colourDisp));

        m_sBarR= (SeekBar) findViewById((R.id.barRed));
        m_sBarG= (SeekBar) findViewById((R.id.barGreen));
        m_sBarB= (SeekBar) findViewById((R.id.barBlue));

        bColour[0] = 0;
        bColour[1] = 0;
        bColour[2] = 0;
        m_sBarR.setOnSeekBarChangeListener(seekChangeHandler);
        m_sBarG.setOnSeekBarChangeListener(seekChangeHandler);
        m_sBarB.setOnSeekBarChangeListener(seekChangeHandler);

        m_tBroadcastResult = (TextView) findViewById((R.id.textBroadcast));

        m_bModeChange = (Button) findViewById(R.id.button);
        m_bModeChange.setOnClickListener(buttonClickHandler);
        m_eLightOut = LightOutMode.RGB;
        m_bModeChange.setText(m_eLightOut.toString());


        m_bOkToSend = false;
        m_tBroadcastReciever = new Thread(
            new Runnable() {
               public void run()
               {
                   while(true) {
                       byte[] recvBuf = new byte[64];
                       String strMessage;
                       try {
                           if (m_sBroadcastSocket == null || m_sBroadcastSocket.isClosed()) {
                               m_sBroadcastSocket = new DatagramSocket(8282);
                               m_sBroadcastSocket.setBroadcast(true);
                               m_sBroadcastSocket.setSoTimeout(6000);
                           }
                           DatagramPacket udpPacket = new DatagramPacket(recvBuf, 64);
                           m_sBroadcastSocket.receive(udpPacket);
                           m_sTargetNeo = udpPacket.getAddress().getHostAddress();
                           strMessage = ("Recieved - " + m_sTargetNeo);
                           m_bOkToSend = true;

                       } catch (Exception ex) {
                           continue;
                       }
                       final String strForUI = strMessage;
                       runOnUiThread(new Runnable() {

                           @Override
                           public void run() {
                               m_tBroadcastResult.setText(strForUI.toCharArray(), 0, strForUI.length());
                           }
                       });
                   }
               }
            }
        );
        m_tBroadcastReciever.start();

        m_tColourSend = new Thread(
                new Runnable() {
                    public void run() {
                        Socket s = new Socket();
                        OutputStream outStream = null;
                        InputStream iStream = null;
                        byte[] sendBuf = new byte[94];
                        int bCycleCount = 0;
                        int bCycleSubCount = 0;
                        int iTimeToWait = 10;
                        int r,g,b;
                        int iBufferIter = 0;
                        while (true) {
                            try {
                                ++bCycleCount;
                                if (m_bOkToSend) {

                                    //build packet
                                    sendBuf[iBufferIter++] = 0;
                                    sendBuf[iBufferIter++] = 92;
                                    sendBuf[iBufferIter++] = 0;
                                    sendBuf[iBufferIter++] = 4;
                                    if(m_eLightOut == LightOutMode.RGB) {
                                        for (int i = 0; i < 30; ++i) {
                                            sendBuf[iBufferIter++] = (byte) bColour[0];
                                            sendBuf[iBufferIter++] = (byte) bColour[2];
                                            sendBuf[iBufferIter++] = (byte) bColour[1];
                                        }
                                    }else if (m_eLightOut == LightOutMode.RAINBOW){
                                        bCycleCount++;
                                        int bCurrentCount = bCycleCount;
                                        iTimeToWait = bColour[0] + 2;
                                        for(int i = 0; i < 30; ++i) {
                                            ++bCurrentCount;
                                            float[] hsv = new float[3];
                                            float hue = (bCurrentCount + (i * (bColour[1] / 8.5f))) * (float)1.4;
                                            while(hue > 360) hue -= 360;
                                            hsv[0] = hue;
                                            hsv[1] = 1.0f;
                                            hsv[2] = (bColour[2] / 255.0f);


                                            int c = Color.HSVToColor(hsv);
                                            r = (byte) (c & 0xFF);
                                            c = c >> 8;
                                            b = (byte) (c & 0xFF);
                                            c = c >> 8;
                                            g = (byte) (c & 0xFF);
                                            c = c >> 8;
                                            sendBuf[iBufferIter++] = (byte) r;
                                            sendBuf[iBufferIter++] = (byte) b;
                                            sendBuf[iBufferIter++] = (byte) g;
                                        }
                                    }
                                    else if(m_eLightOut == LightOutMode.FLASH) {
                                        for(int i = 2; i < 92; ++i) {
                                            int AmountToMove = Math.min(Math.abs(sendBuf[i] - (int) 0x09), 5);
                                                sendBuf[i] -= (byte)AmountToMove;

                                        }
                                        Random rand = new Random();
                                        if(rand.nextInt(255) < bColour[0]) {
                                            int iNextFlash = rand.nextInt((25));
                                            int iFlashWidth = rand.nextInt(3)+2;
                                            int iRandHue = rand.nextInt(360);
                                            float[] hsv = new float[3];
                                            hsv[0] = iRandHue;
                                            hsv[1] = bColour[1] / 255.0f;
                                            hsv[2] = bColour[2] / 255.0f;
                                            int c = Color.HSVToColor(hsv);
                                            r = (byte) (c & 0xFF);
                                            c = c >> 8;
                                            b = (byte) (c & 0xFF);
                                            c = c >> 8;
                                            g = (byte) (c & 0xFF);
                                            c = c >> 8;
                                            for(int iToFlash = 0; iToFlash < iFlashWidth; ++iToFlash) {
                                                int iLed = (iNextFlash + iToFlash) * 3;
                                                sendBuf[iBufferIter++] = (byte)r;
                                                sendBuf[iBufferIter++] = (byte)b;
                                                sendBuf[iBufferIter++] = (byte)g;
                                            }
                                        }
                                    }

                                    InetAddress ia = InetAddress.getByName(m_sTargetNeo);
                                    if(!s.isConnected()) {
                                        s = new Socket(ia, 8081);
                                        s.setSoTimeout(5000);
                                        outStream = s.getOutputStream();
                                        iStream = s.getInputStream();
                                    }
                                    if(s.isConnected()) {
                                        outStream.write(sendBuf, 0, --iBufferIter);
                                        byte[] rcvBuf = new byte[1];
                                        while(s.getInputStream().available() == 0);
                                            iStream.read(rcvBuf);
                                    }
                                    Thread.sleep(iTimeToWait);
                                } else {
                                    Thread.sleep(2000);
                                }
                            } catch (Exception ex) {
                                m_bOkToSend = false;
                                try {
                                    s.close();
                                } catch (IOException e) {
                                    e.printStackTrace();
                                }
                                final String strForUI = "Failed To Send";
                                runOnUiThread(new Runnable() {

                                    @Override
                                    public void run() {
                                        m_tBroadcastResult.setText(strForUI.toCharArray(), 0, strForUI.length());
                                    }
                                });
                            }
                        }
                    }

                    ;
                }
        );
        m_tColourSend.start();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_basepage, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
    SeekBar.OnSeekBarChangeListener seekChangeHandler = new SeekBar.OnSeekBarChangeListener() {
        @Override
        public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
            bColour[0] = m_sBarR.getProgress();
            bColour[1] = m_sBarG.getProgress();
            bColour[2] = m_sBarB.getProgress();
            m_ColourDisplay.setBackgroundColor(android.graphics.Color.argb(255, bColour[0], bColour[1], bColour[2]));
        }
        @Override
        public void onStopTrackingTouch(SeekBar seekBar) {
            onProgressChanged(seekBar, seekBar.getProgress(), false);
        }
        @Override
        public void onStartTrackingTouch(SeekBar seekBar) {
            onProgressChanged(seekBar, seekBar.getProgress(), false);
        }
    };

    Button.OnClickListener buttonClickHandler = new Button.OnClickListener() {
        public void onClick(View v) {
            if(m_eLightOut == LightOutMode.RAINBOW)
                m_eLightOut = LightOutMode.RGB;
            else if( m_eLightOut == LightOutMode.RGB)
                m_eLightOut = LightOutMode.FLASH;
            else
                m_eLightOut = LightOutMode.RAINBOW;
            m_bModeChange.setText(m_eLightOut.toString());
        };
    };
}
