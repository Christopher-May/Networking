package server;

import utils.Message;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Scanner;
import java.util.concurrent.TimeUnit;


public class Server {

    private final Object lock = new Object();
    private final Object isEmpty = new Object();

    private ArrayList<Message> broadCastQueue = new ArrayList<Message>();
    public ArrayList<Connection> clients = new ArrayList<Connection>();
    private DatagramSocket socket;

    public void addToBroadcastQueue(Message m) { //add a message to the broadcast queue. this method is used by all ClientConnection instances
        synchronized (lock) {
            broadCastQueue.add(m);
        }
        synchronized (isEmpty){
            isEmpty.notify();
        }
    }

    public Server(int port) {

        try {
            socket = new DatagramSocket(port);
        } catch (SocketException e) {
            e.printStackTrace();
        }
        new Thread(new BroadCast()).start();

        byte[] buf = new byte[256];
        DatagramPacket packet;

        InetAddress clientAddr;
        int clientPort;
        String newPort;


        System.out.println("Starting server at Port:"+port+"\n");

        for(;;){
            packet = new DatagramPacket(buf,buf.length);
            System.out.println("receiving\n");

            try {
                socket.receive(packet);
            } catch (IOException e) {
                e.printStackTrace();
            }
            System.out.println("got packet \n");

            clientAddr = packet.getAddress();
            clientPort = packet.getPort();

            boolean found = false;
            for(Connection c : clients) {

                if(c.getIP().equals(clientAddr) && clientPort == c.getPort()){
                    newPort = String.valueOf(c.getPort());
                    buf = newPort.getBytes();
                    packet = new DatagramPacket(buf,buf.length,clientAddr,clientPort);
                    try {
                        socket.send(packet);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }

                    found = true;
                    break;
                }
            }

            if(!found) {
                Connection c = new Connection(this, clientAddr, clientPort);
                c.start();
                clients.add(c);

                newPort = String.valueOf(c.getPort());
                buf = newPort.getBytes();
                packet = new DatagramPacket(buf, buf.length, clientAddr, clientPort);

                try {
                    socket.send(packet);
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        }


    }
    private class BroadCast implements Runnable{

        @Override
        public void run() {
            // list of connections that should be removed because they timedout
            ArrayList<Message> toRemove;
            for(;;){

                toRemove = new ArrayList<Message>();
                //there is something to tell the connections to send
                if( !broadCastQueue.isEmpty() ){
                    //get the message and remove it from the queue
                    Message m;
                    synchronized (lock) {
                        m = broadCastQueue.get(0);
                        broadCastQueue.remove(m);
                    }
                    //check if any clients need to be culled
                    for(Connection c : clients) {
                        if (!c.isAlive()) {
                            toRemove.add(m);
                        }
                    }
                    //cull
                    clients.removeAll(toRemove);
                    //check if the message is old
                    if (TimeUnit.SECONDS.convert( System.nanoTime()-m.getTimestamp(), TimeUnit.NANOSECONDS) < 2) {
                        //it isn't so we send it
                        for(Connection c : clients) {

                            if (c.getClId() != m.getClId()) {
                                c.addToQueue(m);
                            }
                        }
                    }
                    //it is so we don't send it

                //there is no messages to be sent to connections to we wait until we're notified by addtobroadcastqueue()
                } else {
                    try {
                        synchronized (isEmpty){
                            while (broadCastQueue.isEmpty()){

                                isEmpty.wait();
                            }
                        }
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }

            }
        }
    }

    public static void main(String[] args){
        //grab the "main" port to look out for
        Scanner reader = new Scanner(System.in);
        System.out.println("Enter the port number\n");
        int port = reader.nextInt();
        reader.close();
        //start the server
        new Server(port);
    }
}
