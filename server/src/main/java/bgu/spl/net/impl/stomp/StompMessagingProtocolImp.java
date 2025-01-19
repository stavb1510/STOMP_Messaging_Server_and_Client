package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

public class StompMessagingProtocolImp<T> implements StompMessagingProtocol<T> {
    private int connectionId; // The connection ID for the current client
    private Connections<T> connections; // The Connections object for communication
    private boolean shouldTerminate; // Flag to determine if the connection should be terminated
    private Map<String, Integer> subscriptions; // Map of topic -> subscription ID
    private boolean isConnected; // Indicates if the client has successfully connected

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

    @Override
    public T process(T message) {
        if (message instanceof StompFrame) {
            StompFrame frame = (StompFrame) message;
            String command = frame.getCommand();

            switch (command) {
                case "CONNECT":
                    handleConnect(frame);
                    break;
                case "SEND":
                    handleSend(frame);
                    break;
                case "SUBSCRIBE":
                    handleSubscribe(frame);
                    break;
                case "UNSUBSCRIBE":
                    handleUnsubscribe(frame);
                    break;
                case "DISCONNECT":
                    handleDisconnect(frame);
                    break;
                default:
                    handleError("Invalid command: " + command);
                    break;
            }
        }
        return null;
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private void handleConnect(StompFrame frame) {
        // Retrieve headers from the CONNECT frame
        String acceptVersion = frame.getHeaders().get("accept-version");
        String host = frame.getHeaders().get("host");
        String username = frame.getHeaders().get("login");
        String password = frame.getHeaders().get("passcode");
    
        // Check if accept-version is valid
        if (acceptVersion == null) {
            handleError("Missing 'accept-version' header in CONNECT frame");
            return;
        }
        if (!acceptVersion.equals("1.2")) {
            handleError("Invalid 'accept-version'. Expected: 1.2, but got: " + acceptVersion);
            return;
        }
    
        // Check if host is valid
        if (host == null) {
            handleError("Missing 'host' header in CONNECT frame");
            return;
        }
        if (!host.equals("stomp.cs.bgu.ac.il")) {
            handleError("Invalid 'host'. Expected: stomp.cs.bgu.ac.il, but got: " + host);
            return;
        }
    
        // Check if username and password are provided
        if (username == null) {
            handleError("Missing 'login' header in CONNECT frame");
            return;
        }
        if (password == null) {
            handleError("Missing 'passcode' header in CONNECT frame");
            return;
        }
    
        // Check if the username already exists
        if (connections.isUserRegistered(username)) {
            // If the username exists, check if the password matches
            if (!connections.isPasswordCorrect(username, password)) {
                handleError("User '" + username + "' already exists with a different password");
                return;
            }
        } else {
            // Register the user if not already registered
            connections.registerUser(username, password);
        }
    
        // Mark the client as connected
        isConnected = true;
    
        // Send a CONNECTED frame back to the client
        Map<String, String> headers = new HashMap<>();
        headers.put("version", acceptVersion);
    
        StompFrame connectedFrame = new StompFrame("CONNECTED", headers, "");
        connections.send(connectionId, (T) connectedFrame);
    }

    private void handleSend(StompFrame frame) {
        String destination = frame.getHeaders().get("destination");
        if (destination == null || !subscriptions.containsKey(destination)) {
            // If the destination is missing or the client is not subscribed to the topic, send an ERROR
            handleError("Cannot send to destination: " + destination);
            return;
        }

        String body = frame.getBody();

        Map<String, String> headers = new HashMap<>();
        headers.put("subscription", subscriptions.get(destination).toString());
        headers.put("destination", destination);

        StompFrame messageFrame = new StompFrame("MESSAGE", headers, body);
        connections.send(destination, (T) messageFrame);
    }

    private void handleSubscribe(StompFrame frame) {
        String destination = frame.getHeaders().get("destination");
        String id = frame.getHeaders().get("id");

        if (destination == null || id == null) {
            // If the destination or id is missing, send an ERROR
            handleError("Missing destination or id in SUBSCRIBE frame");
            return;
        }

        subscriptions.put(destination, Integer.parseInt(id));
        connections.subscribe(destination, connectionId);
    }

    private void handleUnsubscribe(StompFrame frame) {
        String id = frame.getHeaders().get("id");
        if (id == null) {
            // If the id is missing, send an ERROR
            handleError("Missing id in UNSUBSCRIBE frame");
            return;
        }

        subscriptions.entrySet().removeIf(entry -> entry.getValue().equals(Integer.parseInt(id)));
    }

    private void handleDisconnect(StompFrame frame) {
        String receipt = frame.getHeaders().get("receipt");
        if (receipt != null) {
            // Send a RECEIPT frame if requested
            Map<String, String> headers = new HashMap<>();
            headers.put("receipt-id", receipt);

            StompFrame receiptFrame = new StompFrame("RECEIPT", headers, "");
            connections.send(connectionId, (T) receiptFrame);
        }
        shouldTerminate = true; // Mark the connection for termination
        connections.disconnect(connectionId);
    }

    private void handleError(String errorMessage) {
        // Create an ERROR frame with the provided error message
        Map<String, String> headers = new HashMap<>();
        headers.put("message", errorMessage);

        StompFrame errorFrame = new StompFrame("ERROR", headers, "");
        connections.send(connectionId, (T) errorFrame);
        shouldTerminate = true; // Mark the connection for termination
        connections.disconnect(connectionId);
    }
}