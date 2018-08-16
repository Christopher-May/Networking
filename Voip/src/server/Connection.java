package server;

import utils.Message;

import java.io.*;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

public class Connection extends Thread{
    private Server s;

    private static AtomicInteger next = new AtomicInteger();
    private int id;

    private final Object toSendlock = new Object();
    private final Object isEmpty = new Object();

    private ArrayList<Message> toSend = new ArrayList<>();

    private DatagramSocket socket;
    private InetAddress clientAddr;
    private int clientPort;

    private long lastMessage;

    private final AtomicBoolean running = new AtomicBoolean(true);
    /*
        Setting up the connection
    */
    public Connection(Server s,InetAddress clientAddr,int clientPort) {
        id = next.incrementAndGet();
        this.s= s;
        this.clientAddr = clientAddr;
        this.clientPort = clientPort;
        lastMessage = System.nanoTime();
        try {
            socket = new DatagramSocket(0);
        } catch (SocketException e) {
            e.printStackTrace();
        }

    }

    public int getClId() {
        return id;
    }

    public int getPort() {
        return socket.getLocalPort();
    }
    public InetAddress getIP(){
        return clientAddr;
    }
    /*
        add to the queue of messages to be sent to client and wake up sending thread if needed
    */
    public void addToQueue(Message m){
        synchronized (toSendlock) {
            toSend.add(m);
        }
        synchronized (isEmpty){
            isEmpty.notify();
        }
        System.out.println(toSend.size());
    }

    @Override
    public void run() {
        //start sendMesg thread
        new Thread(new SendMsg()).start();

        while (running.get()){
            //we haven't seen a message in 5minutes something happened
            if (TimeUnit.MINUTES.convert( System.nanoTime()-lastMessage, TimeUnit.NANOSECONDS) > 5) {
                running.set(false);
            }
            recvMsg();

        }

    }

    private void recvMsg() {
        byte[] buf = new byte[10000];
        try {
            DatagramPacket packet = new DatagramPacket(buf, buf.length, clientAddr, clientPort);
            socket.receive(packet);
            System.out.println(packet.getData().length);

            ObjectInputStream iStream = new ObjectInputStream(new ByteArrayInputStream(buf));
            Message m = (Message) iStream.readObject();
            iStream.close();

            if (m.getClId() == -1) {
                m.setClId(id);
                m.setTimestamp(System.nanoTime());
                s.addToBroadcastQueue(m);
                lastMessage = System.nanoTime();
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

    }
    /*
        sends message to client
    */
    private class SendMsg implements Runnable{
        @Override
        public void run() {
            Message m;
            try {

                while (running.get()){
                    //wait until queue isnt empty
                    synchronized (isEmpty){
                        while (toSend.isEmpty()){
                            isEmpty.wait();
                        }
                    }

                    //grab a message
                    synchronized (toSendlock) {
                        m = toSend.get(0);
                        toSend.remove(0);
                    }

                    try{
                        //Serialize
                        ByteArrayOutputStream bos = new ByteArrayOutputStream();
                        ObjectOutput out = new ObjectOutputStream(bos);
                        out.writeObject(m);
                        out.flush();
                        byte[] mBytes = bos.toByteArray();

                        //make packet
                        DatagramPacket packet = new DatagramPacket(mBytes,mBytes.length,clientAddr,clientPort);

                        //send packet
                        socket.send(packet);
                        System.out.println("sending message"+m.getTimestamp());

                        bos.close();
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            } catch (Exception ex) {
                System.exit(0);
            }
        }

    }

}
