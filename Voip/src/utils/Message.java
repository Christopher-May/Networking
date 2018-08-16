package utils;

import java.io.Serializable;

/*
    What is being sent through UDP, has the metadata and the soundpacket in data (could be anything)
*/
public class Message implements Serializable{
    private long clId;
    private long timestamp;
    private Object data;


    public Message(long clId, long timestamp, Object data) {
        this.clId = clId;
        this.timestamp = timestamp;
        this.data = data;
    }

    public long getClId() {
        return clId;
    }
    public void setClId(long chId) {
        this.clId = chId;
    }

    public Object getData() {
        return data;
    }

    public long getTimestamp() {
        return timestamp;
    }
    public void setTimestamp(long timestamp) {
        this.timestamp = timestamp;
    }

}