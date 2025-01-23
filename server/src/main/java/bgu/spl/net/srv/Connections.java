package bgu.spl.net.srv;

import java.io.IOException;

public interface Connections<T> {

    boolean send(int connectionId, T msg);

    void send(String channel, T msg);

    void disconnect(int connectionId);

    void subscribe(String channel, int connectionId);

    void unsubscribe(String channel, int connectionId);

    boolean registerUser(String username, String password);

    boolean isUserRegistered(String username);

    boolean isPasswordCorrect(String username, String password);

    void addClient(int connectionId, ConnectionHandler<T> handler);

}
