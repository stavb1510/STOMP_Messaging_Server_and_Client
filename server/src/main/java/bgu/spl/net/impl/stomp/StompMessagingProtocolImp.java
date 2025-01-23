package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.ConnectionHandler;
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
    private ConnectionHandler<T> handler;

    public StompMessagingProtocolImp() {
        this.shouldTerminate = false;
        this.subscriptions = new ConcurrentHashMap<>();
        this.isConnected = false;
    }

    @Override
    public void start(int connectionId, Connections<T> connections, ConnectionHandler<T> handler) {
        this.connectionId = connectionId;
        this.connections = connections;
        this.handler = handler;
    }

    private int generateMessageId() {
        return messageCounter.incrementAndGet(); // Increment and return the new value
    }

    @Override
    public void process(T message) {
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
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }

    private void handleConnect(StompFrame frame) {
        String acceptVersion = frame.getHeaders().get("accept-version");
        String host = frame.getHeaders().get("host");
        String username = frame.getHeaders().get("login");
        String password = frame.getHeaders().get("passcode");

        if (isConnected) {
            handleError("Client is already connected");
            return;
        }

        if (acceptVersion == null || !acceptVersion.equals("1.2")) {
            handleError("Invalid or missing 'accept-version'");
            return;
        }

        if (host == null || !host.equals("stomp.cs.bgu.ac.il")) {
            handleError("Invalid or missing 'host'");
            return;
        }

        if (username == null || password == null) {
            handleError("Missing 'login' or 'passcode'");
            return;
        }

        if (connections.isUserRegistered(username)) {
            if (!connections.isPasswordCorrect(username, password)) {
                handleError("Incorrect password for user: " + username);
                return;
            }
        } else {
            connections.registerUser(username, password);
        }

        isConnected = true;
        Map<String, String> headers = new HashMap<>();
        headers.put("version", acceptVersion);
        StompFrame connectedFrame = new StompFrame("CONNECTED", headers, "");
        connections.addClient(connectionId, handler);
        connections.send(connectionId, (T) connectedFrame);
    }

    private void handleSend(StompFrame frame) {
        String destination = frame.getHeaders().get("destination");
        if (destination == null) {
            handleError("Missing destination");
            return;
        }

        String body = frame.getBody();
        if (body == null || body.isEmpty()) {
            handleError("Empty message body");
            return;
        }

        if (!subscriptions.containsKey(destination)) {
            handleError("Client is not subscribed to destination: " + destination);
            return;
        }
        // To do: put the right subscriptionId to every messege we create!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        int messageId = generateMessageId();
        Map<String, String> messageHeaders = new HashMap<>();
        messageHeaders.put("destination", destination);
        messageHeaders.put("subscription", subscriptions.get(destination).toString());
        messageHeaders.put("message-id", String.valueOf(messageId));

        StompFrame messageFrame = new StompFrame("MESSAGE", messageHeaders, body);
        connections.send(destination, (T) messageFrame);
    }

    private void handleSubscribe(StompFrame frame) {
        String destination = "/" + frame.getHeaders().get("destination");
        String id = frame.getHeaders().get("id");

        if (destination == null || id == null) {
            handleError("Missing 'destination' or 'id'");
            return;
        }
        int subscriptionId;
        try {
            subscriptionId = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            handleError("Invalid 'id': must be an integer");
            return;
        }
        if (subscriptions.containsKey(destination)) {
            return;
        }
        subscriptions.put(destination, subscriptionId);
        connections.subscribe(destination, connectionId);
    }

    private void handleUnsubscribe(StompFrame frame) {
        String id = frame.getHeaders().get("id");

        if (id == null) {
            handleError("Missing 'id'");
            return;
        }
        
        int subscriptionId;
        try {
            subscriptionId = Integer.parseInt(id);
        } catch (NumberFormatException e) {
            handleError("Invalid 'id': must be an integer");
            return;
        }

        String destination = null;
        for (String dest : subscriptions.keySet()) {
            if (subscriptions.get(dest).equals(subscriptionId)) {
                destination = dest;
                break;
            }
        }
        if (destination == null) {
            return;
        }

        subscriptions.remove(destination);
        connections.unsubscribe(destination, connectionId);
    }

    private void handleDisconnect(StompFrame frame) {
        String receiptId = frame.getHeaders().get("receipt");

        if (receiptId == null) {
            handleError("Missing 'receipt'");
            return;
        }

        Map<String, String> headers = new HashMap<>();
        headers.put("receipt-id", receiptId);

        StompFrame receiptFrame = new StompFrame("RECEIPT", headers, "");
        connections.send(connectionId, (T) receiptFrame);

        connections.disconnect(connectionId);
        shouldTerminate = true;
    }

    private void handleError(String errorMessage) {
        Map<String, String> headers = new HashMap<>();
        headers.put("message", errorMessage);

        StompFrame errorFrame = new StompFrame("ERROR", headers, "");
        connections.send(connectionId, (T) errorFrame);
        shouldTerminate = true;
        connections.disconnect(connectionId);
    }
}
