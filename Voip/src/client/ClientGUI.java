package client;


import java.awt.*;
import java.awt.event.*;
import java.util.concurrent.atomic.AtomicBoolean;
import javax.swing.*;

public class ClientGUI {
    private JFrame mainFrame;
    private JLabel headerLabel;
    private JLabel statusLabel;
    private JPanel controlPanel;
    private static Client client;


    private ClientGUI(){
        prepareGUI();
    }
    private void prepareGUI(){
        mainFrame = new JFrame("Client");
        mainFrame.setSize(400,275);
        mainFrame.setLayout(new GridLayout(3, 1));



        headerLabel = new JLabel("",JLabel.CENTER );
        statusLabel = new JLabel("",JLabel.CENTER);
        statusLabel.setSize(350,100);

        mainFrame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent windowEvent){
                System.exit(0);
            }
        });
        controlPanel = new JPanel();
        controlPanel.setLayout(new FlowLayout());

        mainFrame.add(headerLabel);
        mainFrame.add(controlPanel);
        mainFrame.add(statusLabel);
        mainFrame.setVisible(true);
    }
    private void startConnect() {
        headerLabel.setText("VOIP");
        AtomicBoolean clientRunning = new AtomicBoolean(false);

        JTextField ipField = new JTextField("172.18.0.1", 12);
        JTextField portField = new JTextField("4011", 4);
        JButton connectBtn = new JButton("Connect");
        JSlider micVol = new javax.swing.JSlider();
        micVol.setMaximum(100);
        micVol.setMinimum(0);
        micVol.setValue(100);

        JProgressBar micLev = new JProgressBar(0,100);
        micLev.setPreferredSize(new Dimension(350, 20));
        micLev.setValue(0);
        micLev.setStringPainted(false);

        /*
            function for button that starts client thread or stops it
        */
        connectBtn.addActionListener(ae -> {

            String ipValue = ipField.getText();
            String portValue = portField.getText();
            //if client isnt running and ip and port are valid start up client
            if(clientRunning.get() == false) {
                if (validIP(ipValue) && isNumeric(portValue)) {
                    statusLabel.setText(ipValue + ":" + portValue);
                    client = new Client(ipValue, portValue, statusLabel, micVol, micLev, connectBtn);
                    new Thread(client).start();
                    clientRunning.set(true);


                } else {
                    statusLabel.setText("wrong IP and PORT format");
                }
            }
            //client is running turn it off
            else{
                client.turnoff();
                clientRunning.set(false);
            }

        });

        controlPanel.add(ipField);
        controlPanel.add(portField);
        controlPanel.add(connectBtn);
        controlPanel.add(micVol);
        controlPanel.add(micLev);

        mainFrame.setVisible(true);
    }
    /*
        checks if ip is a valid ip address
    */
    private static boolean validIP (String ip) {
        try {
            if ( ip == null || ip.isEmpty() ) {
                return false;
            }

            String[] parts = ip.split( "\\." );
            if ( parts.length != 4 ) {
                return false;
            }

            for ( String s : parts ) {
                int i = Integer.parseInt( s );
                if ( (i < 0) || (i > 255) ) {
                    return false;
                }
            }

            return !ip.endsWith(".");

        } catch (NumberFormatException nfe) {
            return false;
        }
    }
    /*
        checks if port is working
    */
    private static boolean isNumeric(String str) {
        return str.matches("-?\\d+(\\.\\d+)?");
    }
    public static void main(String[] args){
        ClientGUI clientGUI = new ClientGUI();
        clientGUI.startConnect();
    }
}