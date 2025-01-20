package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class StompMessagingProtocolImp<T> implements StompMessagingProtocol<T> {
    private int connectionId; // The connection ID for the current client
    private Connections<T> connections; // The Connections object for communication
    private boolean shouldTerminate; // Flag to determine if the connection should be terminated
    private Map<String, Integer> subscriptions; // Map of topic -> subscription ID
    private boolean isConnected; // Indicates if the client has successfully connected
    private static final AtomicInteger messageCounter = new AtomicInteger(0);


    public StompMessagingProtocolImp() {
        this.shouldTerminate = false;
        this.subscriptions = new ConcurrentHashMap<>();
        this.isConnected = false;
    }

    @Override
    public void start(int connectionId, Connections<T> connections) {
        this.connectionId = connectionId;
        this.connections = connections;
    }

    private int generateMessageId() {
        return messageCounter.incrementAndGet(); // Increment and return the new value
    }

    @Override
    public T process(T message) {
        T response = null;
        if (message instanceof StompFrame) {
            StompFrame frame = (StompFrame) message;
            String command = frame.getCommand();
            switch (command) {
                case "CONNECT":
                    response = handleConnect(frame);
                    break;
                case "SEND":
                    response = handleSend(frame);
                    break;
                case "SUBSCRIBE":
                    response = handleSubscribe(frame);
                    break;
                case "UNSUBSCRIBE":
                    response = handleUnsubscribe(frame);
                    break;
                case "DISCONNECT":
                    response = handleDisconnect(frame);
                    break;
                default:
                    response = handleError("Invalid command: " + command);
                    break;
            }
        }
        return response;
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private T handleConnect(StompFrame frame) {
        // Retrieve headers from the CONNECT frame
        String acceptVersion = frame.getHeaders().get("accept-version");
        String host = frame.getHeaders().get("host");
        String username = frame.getHeaders().get("login");
        String password = frame.getHeaders().get("passcode");

        // Check if the client is already connected
        if (isConnected) {
            return handleError("Client is already connected");
        }
    
        // Check if accept-version is valid
        if (acceptVersion == null) {
            return handleError("Missing 'accept-version' header in CONNECT frame");
        }
        if (!acceptVersion.equals("1.2")) {
            return handleError("Invalid 'accept-version'. Expected: 1.2, but got: " + acceptVersion);
        }
    
        // Check if host is valid
        if (host == null) {
            return handleError("Missing 'host' header in CONNECT frame");
        }
        if (!host.equals("stomp.cs.bgu.ac.il")) {
            return handleError("Invalid 'host'. Expected: stomp.cs.bgu.ac.il, but got: " + host);
        }
    
        // Check if username and password are provided
        if (username == null) {
            return handleError("Missing 'login' header in CONNECT frame");
        }
        if (password == null) {
            return handleError("Missing 'passcode' header in CONNECT frame");
        }
    
        // Check if the username already exists
        if (connections.isUserRegistered(username)) {
            // If the username exists, check if the password matches
            if (!connections.isPasswordCorrect(username, password)) {
                return handleError("User '" + username + "' already exists with a different password");
            }
        } else {
            connections.registerUser(username, password);
        }
        isConnected = true;
    
        Map<String, String> headers = new HashMap<>();
        headers.put("version", acceptVersion);
        // headers.put("connection-id", String.valueOf(connectionId)); // Send the connectionId to the client????????????????
        StompFrame connectedFrame = new StompFrame("CONNECTED", headers, "");
        return (T) connectedFrame;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////

    private T handleSend(StompFrame frame) {
        // Retrieve the 'destination' header
        String destination = frame.getHeaders().get("destination");
    
        // Validate the 'destination' header
        if (destination == null) {
            return handleError("Missing 'destination' header in SEND frame");
        }
    
        // Retrieve the body of the frame (message content)
        String body = frame.getBody();
        if (body == null || body.isEmpty()) {
            return handleError("Empty body in SEND frame");
        }
    
        // Check if the client is subscribed to the topic
        if (!subscriptions.containsKey(destination)) {
            return handleError("Client is not subscribed to destination: " + destination);
        }
    
        // Create a MESSAGE frame to send to all subscribers
        int messageId = generateMessageId(); // Using AtomicInteger
        Map<String, String> messageHeaders = new HashMap<>();
        messageHeaders.put("destination", destination);
        messageHeaders.put("subscription", subscriptions.get(destination).toString());
        messageHeaders.put("message-id", String.valueOf(messageId));
    
        StompFrame messageFrame = new StompFrame("MESSAGE", messageHeaders, body);
    
        // Send the MESSAGE frame to all subscribers of the topic
        connections.send(destination, (T) messageFrame); // what happens if we send to a destination that has no subscribers????????????
        return (T) messageFrame;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    private T handleSubscribe(StompFrame frame) {
        // Retrieve headers
        String destination = frame.getHeaders().get("destination");
        String id = frame.getHeaders().get("id");
    
        // Validate headers
        if (destination == null) {
            return handleError("Missing 'destination' header in SUBSCRIBE frame");
        }
        if (id == null) {
            return handleError("Missing 'id' header in SUBSCRIBE frame");
        }
    
        int subscriptionId;
        try {
            // Validate that id is an integer
            subscriptionId = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            return handleError("Invalid 'id' header in SUBSCRIBE frame: must be an integer");
        }
    
        try {
            // Add the subscription locally
            subscriptions.put(destination, subscriptionId);
    
            // Register the subscription in Connections
            connections.subscribe(destination, connectionId);
    
            // No need to send a response for successful SUBSCRIBE
        } catch (Exception e) {
            // Handle any unexpected errors
            return handleError("Failed to process SUBSCRIBE frame: " + e.getMessage());
        }
        Map<String, String> headers = new HashMap<>();
        headers.put("receipt-id", frame.getHeaders().get("receipt"));
        return (T) new StompFrame("RECEIPT", headers, "Subscription added");
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    private T handleUnsubscribe(StompFrame frame) {
        // Retrieve the 'id' header
        String id = frame.getHeaders().get("id");
        if (id == null) {
            return handleError("Missing 'id' header in UNSUBSCRIBE frame");
        }
        int subscriptionId;
        try {
            // Validate that id is an integer
            subscriptionId = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            return handleError("Invalid 'id' header in UNSUBSCRIBE frame: must be an integer");
        }
    
        // Find the destination by checking each key in the map
        String destination = null;
        for (String dest : subscriptions.keySet()) {
            if (subscriptions.get(dest).equals(subscriptionId)) {
                destination = dest;
                break; // Exit the loop once the destination is found
            }
        }
    
        if (destination == null) {
            // If the subscription ID is not found, send an ERROR frame
            return handleError("Subscription ID " + subscriptionId + " not found");
        }
    
        try {
            // Remove the subscription locally
            subscriptions.remove(destination);
    
            // Unsubscribe the client from the topic in Connections
            connections.unsubscribe(destination, connectionId);
    
            // No response frame is needed for UNSUBSCRIBE
        } catch (Exception e) {
            // Handle any unexpected errors
            return handleError("Failed to process UNSUBSCRIBE frame: " + e.getMessage());
        }
        Map<String, String> headers = new HashMap<>();
        headers.put("receipt-id", frame.getHeaders().get("receipt"));
        return (T) new StompFrame("RECEIPT", headers, "Unsubscribed from " + destination);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    private T handleDisconnect(StompFrame frame) {
        // Retrieve the 'receipt' header
        String receiptId = frame.getHeaders().get("receipt");
    
        // Validate the 'receipt' header
        if (receiptId == null) {
            return handleError("Missing 'receipt' header in DISCONNECT frame");
        }
    
        // Send a RECEIPT frame back to the client
        Map<String, String> headers = new HashMap<>();
        headers.put("receipt-id", receiptId);
    
        StompFrame receiptFrame = new StompFrame("RECEIPT", headers, "");
    
        // Disconnect the client using the Connections implementation
        connections.disconnect(connectionId);
    
        // Mark the protocol as terminated
        shouldTerminate = true;
        return (T) receiptFrame;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////

    private T handleError(String errorMessage) {
        // Create an ERROR frame with the provided error message
        Map<String, String> headers = new HashMap<>();
        headers.put("message", errorMessage);

        StompFrame errorFrame = new StompFrame("ERROR", headers, "");
        shouldTerminate = true; // Mark the connection for termination
        connections.disconnect(connectionId);
        return (T) errorFrame;
    }
}