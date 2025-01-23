package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.MessageEncoderDecoder;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

public class MessageEncoderDecoderImp implements MessageEncoderDecoder<StompFrame> {
    
    private byte[] bytes = new byte[1 << 10]; 
    private int len = 0; 

    @Override
    public StompFrame decodeNextByte(byte nextByte) {
        // If we encounter the termination character '\u0000', decode the message
        if (nextByte == '\u0000') {
            String message = popString(); 
            System.out.println("Full message received: " + message); // Debug: print the full message
            return parseFrame(message);
        }
        // Add the byte to the buffer
        pushByte(nextByte);
        return null; // Message is not complete yet
    }

    @Override
    public byte[] encode(StompFrame frame) {
        // Construct the STOMP message format as a string
        StringBuilder sb = new StringBuilder();
        sb.append(frame.getCommand()).append("\n"); // Add the command

        for (Map.Entry<String, String> header : frame.getHeaders().entrySet()) {
            sb.append(header.getKey()).append(":").append(header.getValue()).append("\n");
        }
        sb.append("\n"); 
        if (frame.getBody() != null && !frame.getBody().isEmpty()) {
            sb.append(frame.getBody()).append("\n"); 
        }

        sb.append("\u0000"); 
        return sb.toString().getBytes(StandardCharsets.UTF_8); 
    }

    private void pushByte(byte nextByte) {
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);
        }

        // Add the byte to the buffer
        bytes[len++] = nextByte;
    }

    private String popString() {
        // Convert the collected bytes into a string
        String result = new String(bytes, 0, len, StandardCharsets.UTF_8);
        len = 0; // Reset the buffer
        return result;
    }

    private StompFrame parseFrame(String message) {
        String[] lines = message.split("\n");
        String command = lines[0]; 

        // Parse headers in "key:value" format
        Map<String, String> headers = new HashMap<>();
        int i = 1;
        while (i < lines.length && !lines[i].isEmpty()) {
            String[] headerParts = lines[i].split(":", 2); 
            if (headerParts.length == 2) {
                headers.put(headerParts[0].trim(), headerParts[1].trim());
            }
            i++;
        }

        // Parse the body (if it exists)
        StringBuilder bodyBuilder = new StringBuilder();
        for (i = i + 1; i < lines.length; i++) {
            bodyBuilder.append(lines[i]).append("\n");
        }
        String body = bodyBuilder.toString().trim();
        return new StompFrame(command, headers, body);
    }
}