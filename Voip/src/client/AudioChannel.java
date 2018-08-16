package client;

import utils.Message;
import utils.SoundPacket;

import javax.sound.sampled.*;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.zip.GZIPInputStream;

public class AudioChannel extends Thread {
    //an id unique for each client
    private long clId;

    private final Object lock = new Object();
    private final Object toPlay = new Object();

    private ArrayList<Message> queue = new ArrayList<Message>();

    private AudioFormat format = new AudioFormat(8000.0f, 16, 1, true, true);

    private AudioInputStream audioInputStream;
    private SourceDataLine sourceDataLine;

    private final AtomicBoolean running= new AtomicBoolean(true);
    private long lastMessage;

    /*
        gets the clid from client
        @param clid: The unique id for the client
    */
    AudioChannel(long clId) {
        this.clId = clId;

    }
    /*
        @return the unique id for the client
    */
    public long getClId() {
        return clId;
    }
    public long getLastMessage(){
        return lastMessage;
    }
    /*
        adds a message to the queue to be played

        @param m: the message to be added to queue
    */
    public void addToQueue(Message m) { //adds a message to the play queue
        synchronized (lock){
            queue.add(m);
        }
        synchronized (toPlay){
            toPlay.notify();
        }

    }
    /*
        Takes a message, grabs the audiodata then unzips it and plays it to the speakers
    */
    private void playVoice(Message m) {
        byte[] audioData;
        InputStream byteArrayInputStream;
        SoundPacket sp;
        GZIPInputStream gis = null;
        ByteArrayOutputStream  baos;

        sp = (SoundPacket) m.getData();

        if(sp != null){

            if(sp.getData() != null){
                //unzip the sound data
                try {
                    gis = new GZIPInputStream(new ByteArrayInputStream(sp.getData()));
                } catch (IOException e) {
                    e.printStackTrace();
                }
                baos = new ByteArrayOutputStream();
                for (; ; ) {
                    int b = 0;
                    try {
                        assert gis != null;

                        b = gis.read();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    if (b == -1) {
                        break;
                    } else {
                        baos.write((byte) b);
                    }
                }
                //play decompressed data
                audioData = baos.toByteArray();

                byteArrayInputStream = new ByteArrayInputStream(audioData);
                audioInputStream = new AudioInputStream(byteArrayInputStream, format, audioData.length / format.getFrameSize());


                int cnt;
                byte tempBuffer[] = new byte[10000];
                //
                try {
                    while ((cnt = audioInputStream.read(tempBuffer, 0, tempBuffer.length)) != -1) {
                        if (cnt > 0) {
                            // Write data to the internal buffer of
                            // the data line where it will be
                            // delivered to the speaker.
                            sourceDataLine.write(tempBuffer, 0, cnt);

                        }
                    }

                } catch (IOException e) {

                }
            }
        }
        sourceDataLine.drain();

    }

    @Override
    public void run() {
        System.out.println("playing audio package");

        try {
            //set up speaker
            DataLine.Info dataLineInfo = new DataLine.Info(SourceDataLine.class, format);
            sourceDataLine = (SourceDataLine) AudioSystem.getLine(dataLineInfo);
            sourceDataLine.open(format);
            sourceDataLine.start();

        } catch (LineUnavailableException e) {
            e.printStackTrace();
        }

        while (running.get()){
            //if there isnt a msg in queue wait
            synchronized (toPlay){
                while (queue.isEmpty()){

                    try {
                        toPlay.wait();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
            //get message and remove it from queue
            Message m;
            synchronized (lock){
                m=queue.get(0);
                queue.remove(0);
            }
            //play
            playVoice(m);

        }
    }

}
