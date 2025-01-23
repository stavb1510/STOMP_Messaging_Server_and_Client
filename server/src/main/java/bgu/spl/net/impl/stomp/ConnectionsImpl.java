package bgu.spl.net.impl.stomp;
import bgu.spl.net.srv.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CopyOnWriteArrayList;

public class ConnectionsImpl<T> implements Connections<T> {
    private Map<Integer, ConnectionHandler<T>> clients = new ConcurrentHashMap<>();
    private Map<String, List<Integer>> topics = new ConcurrentHashMap<>(); //every client that subscribe to specific topic
    private Map<String, String> users = new ConcurrentHashMap<>(); // Map for username -> password

    @Override
    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> handler = clients.get(connectionId);
        if (handler != null) {
            handler.send(msg);
            return true;
        }
        return false;
    }

    @Override
    public void send(String topic, T msg) {
        List<Integer> subscribers = topics.get(topic);
        if (subscribers != null) {
            for (int id : subscribers) {
                send(id, msg);
            }
        }
    }

    @Override
    public void subscribe(String topic, int connectionId) {
        topics.computeIfAbsent(topic, k -> new CopyOnWriteArrayList<>()).add(connectionId);
    }

    @Override
    public void unsubscribe(String topic, int connectionId) {
        List<Integer> subscribers = topics.get(topic);
        if (subscribers != null) {
            subscribers.remove(Integer.valueOf(connectionId)); // Remove the subscriber from the list
            if (subscribers.isEmpty()) {
                topics.remove(topic); // Remove the channel if it has no subscribers
            }
        }
    }

    @Override
    public void disconnect(int connectionId) {
        clients.remove(connectionId);
        for (List<Integer> list : topics.values()) {
            list.remove(Integer.valueOf(connectionId));
        }
    }

    public void addClient(int connectionId, ConnectionHandler<T> handler) {
        clients.put(connectionId, handler);
    }

    @Override
    public boolean registerUser(String username, String password) {
        return users.putIfAbsent(username, password) == null;
    }

    @Override
    public boolean isPasswordCorrect(String username, String password) {
        return password.equals(users.get(username));
    }

    @Override
    public boolean isUserRegistered(String username) {
        return users.containsKey(username);
    }
}