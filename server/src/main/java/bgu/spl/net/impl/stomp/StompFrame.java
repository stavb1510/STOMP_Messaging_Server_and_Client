package bgu.spl.net.impl.stomp;

import java.util.Map;


public class StompFrame {
    private final String command; 
    private final Map<String, String> headers; 
    private final String body; 
    
  
    public StompFrame(String command, Map<String, String> headers, String body) {
        this.command = command;
        this.headers = headers;
        this.body = body; 
    }
    
    public String getCommand() {
        return command;
    }
    
    public Map<String, String> getHeaders() {
        return headers;
    }
    
    public String getBody() {
        return body;
    }
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(command).append("\n");

        // Add headers
        for (Map.Entry<String, String> entry : headers.entrySet()) {
            sb.append(entry.getKey()).append(":").append(entry.getValue()).append("\n");
        }

        sb.append("\n"); // Separate headers from body
        if (body != null && !body.isEmpty()) {
            sb.append(body);
        }

        return sb.toString();
    }   

}
