package client;

import utils.Message;
import utils.SoundPacket;

import javax.sound.sampled.*;
import javax.swing.*;
import java.io.*;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.zip.GZIPOutputStream;


public class Client implements Runnable {
    /* Socket variables */
    private InetAddress ip;
    private int port;
    private int chatPort;
    private DatagramSocket socket;

    /* GUI variables */
    private JSlider micVol;
    private JProgressBar micLev;
    private JLabel status;
    private JButton connectBtn;

    /* Every client thread will keep running until this is false */
    private final AtomicBoolean running = new AtomicBoolean(true);

    /*
     * list of audiochannels
     * each audiochannel plays sound for another client
     */
    private ArrayList<AudioChannel> chs = new ArrayList<>();
    /*
        Takes in information needed from GUI and sets up datagram socket
        @param ip: The ip of the server
        @param port: The port of the client
        @param status: A Jlabel to display to the user the state of connection
        @param micVol: A slider that the user can change mic volume with
        @param micLev: A progress bar that displays if the microphone is in use or not
        @param connectBtn: The button used to start client
     */
    Client(String ip, String port, JLabel status, JSlider micVol, JProgressBar micLev,JButton connectBtn) {
        try {
            this.ip = InetAddress.getByName(ip);
            socket = new DatagramSocket();
        } catch (Exception e) {
            e.printStackTrace();
        }

        this.port = Integer.parseInt(port);
        this.status = status;
        this.micLev = micLev;
        this.micVol = micVol;
        this.connectBtn = connectBtn;

    }
    public void turnoff(){
        running.set(false);
    }
    /*
        makes a connection with the server and receives a new port to send / receive
        @return true if we made a connection else false
     */
    private boolean getChatIP(){
        try {
            // the message does not matter */
            byte[] buf = "hi".getBytes();
            DatagramPacket packet = new DatagramPacket(buf, buf.length, ip, port);
            socket.send(packet);

            // timeout incase the server did not receive the message we can try again
            socket.setSoTimeout(2000);
            // try and receive new port
            buf = new byte[256];
            packet = new DatagramPacket(buf, buf.length);
            socket.receive(packet);
            // parse new port message
            String string = new String(buf).trim();
            System.out.println("New port " + Integer.parseInt(string));
            chatPort = Integer.parseInt(string);
            //remove timeout
            socket.setSoTimeout(0);

            return true;
        } catch (IOException e) {

            return false;
        }
    }
    /*
        Receives a packet from server and returns it as a Message object

        @return returns message received from server
    */
    private Message recvMessage(){
        byte[] buf = new byte[10000];
        DatagramPacket packet = new DatagramPacket(buf, buf.length);
        Message m = null;

        try {

            socket.receive(packet);
            //System.out.println("received message");

            //take buf from packet and turn it into object
            ByteArrayInputStream bais = new ByteArrayInputStream(buf);
            ObjectInputStream iStream = new ObjectInputStream(bais);
            iStream.close();

            // turn the object we received into a Message object
            m= (Message) iStream.readObject();

        } catch (Exception e) {
            e.printStackTrace();
        }

        return m;
    }
    /*
        Takes a message and assigns it to correct audiochannel

        @param m A message received from server
    */
    private void manageAudioChannel(Message m){
        AudioChannel sendTo = null;

        //check if we have any audiochannels for client already
        for (AudioChannel ch : chs) {
            if(!ch.isAlive()){
                chs.remove(ch);
            }
            else if (ch.getClId() == m.getClId()) {
                sendTo = ch;
            }
        }

        if (sendTo != null) {
            sendTo.addToQueue(m);
        } else { //new AudioChannel is needed
            AudioChannel ch = new AudioChannel(m.getClId());
            ch.addToQueue(m);
            chs.add(ch);
            ch.start();
        }
    }
    @Override
    public void run(){
        System.out.println("we're going \n");
        boolean gotIP = false;
        Message m;

        status.setText("Connecting to Server...");
        while(!gotIP){
            gotIP = getChatIP();
        }

        status.setText("Connected to Server");
        Microphone microphone = new Microphone();
        new Thread(microphone).start();
        try{
            connectBtn.setText("Turn off");
            while (running.get()){

                m = recvMessage();

                assert(m instanceof Message);

                manageAudioChannel(m);

            }
            connectBtn.setText("Connect");
            status.setText("");

        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    private class Microphone implements Runnable {
        AudioFormat format = new AudioFormat(8000.0f, 16, 1, true, true);
        TargetDataLine mic;
        int packetSize = 6000;
        SourceDataLine sourceDataLine;

        private Microphone() {
            try {
                mic = AudioSystem.getTargetDataLine(format);

                DataLine.Info info = new DataLine.Info(TargetDataLine.class, format);
                mic = (TargetDataLine) AudioSystem.getLine(info);

                mic.open(format);
                mic.start();

                DataLine.Info dataLineInfo = new DataLine.Info(SourceDataLine.class, format);
                sourceDataLine = (SourceDataLine) AudioSystem.getLine(dataLineInfo);
                sourceDataLine.open(format);
                sourceDataLine.start();



            } catch (LineUnavailableException e) {
                e.printStackTrace();
            }
        }
        /*
            gets audio from microphone

            @returns message object with bytearray of audio from microphone
        */
        private Message getVoice() {
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            int numBytesRead;
            int CHUNK_SIZE = 1024;
            byte[] data = new byte[mic.getBufferSize() / 5];
            byte[] buf;
            int total=0;
            int t;
            int amp = micLev.getValue();

            int bytesRead = 0;

            try {
                //grab mic data
                while (bytesRead < packetSize) {
                    numBytesRead = mic.read(data, 0, CHUNK_SIZE);
                    bytesRead = bytesRead + numBytesRead;
                    System.out.println(bytesRead);
                    out.write(data, 0, numBytesRead);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
            // this is my attempt on volume control
            buf = out.toByteArray();
            for (byte aBuf : buf) {
                t = aBuf;
                micLev.setValue(t); //sets value of miclevel
                t *=amp;
                total +=t;
            }

            if(total == 0){
                return new Message(-1, -1, new SoundPacket(null));
            }
            else {
                //zip audio to try and speed it up
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                GZIPOutputStream go;
                try {
                    go = new GZIPOutputStream(baos);

                    go.write(buf);
                    go.flush();
                    go.close();
                    baos.flush();
                    baos.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }

                return new Message(-1, -1, new SoundPacket(baos.toByteArray()));
            }
        }

        @Override
        public void run() {
            while (running.get()){
                    //get microphone data
                    Message m = getVoice();

                    assert(m instanceof Message);

                    //convert message into bytestream
                    ByteArrayOutputStream bStream = new ByteArrayOutputStream();
                    ObjectOutput oo;
                    try {
                        oo = new ObjectOutputStream(bStream);

                    oo.writeObject(m);
                    oo.close();

                    byte[] mBytes = bStream.toByteArray();

                    //send message
                    DatagramPacket packet = new DatagramPacket(mBytes,mBytes.length,ip,chatPort);
                    socket.send(packet);
                    System.out.println("sending message"+m.getTimestamp());

                    } catch (IOException e) {
                        e.printStackTrace();
                    }


                }

        }
    }
}
