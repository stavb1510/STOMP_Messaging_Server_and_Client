package bgu.spl.net.impl.rci;

import bgu.spl.net.api.MessagingProtocol;
import java.io.Serializable;

import bgu.spl.net.srv.*;


public class RemoteCommandInvocationProtocol<T> implements MessagingProtocol<Serializable> {

    private T arg;

    public RemoteCommandInvocationProtocol(T arg) {
        this.arg = arg;
    }

    @Override
    public void process(Serializable msg) {
        return;
    }

    @Override
    public boolean shouldTerminate() {
        return false;
    }

    @Override
    public void start(int connectionId, Connections<Serializable> connections, ConnectionHandler<Serializable> handler) {    
        System.out.println("hi!");
    }   

}
