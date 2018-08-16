package utils;

import java.io.Serializable;
import javax.sound.sampled.AudioFormat;

/*
    Class is basically a byte array of data
*/
public class SoundPacket implements Serializable{

    private byte[] data;

    public SoundPacket(byte[] data) {

        this.data = data;
    }

    public byte[] getData() {

        return data;
    }
    
}
