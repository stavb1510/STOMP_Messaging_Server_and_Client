package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.out.println("Usage: java StompServer <port> <reactor|tpc>");
            return;
        }

        int port = Integer.parseInt(args[0]);
        String serverType = args[1];

        ConnectionsImpl<StompFrame> connections = new ConnectionsImpl<>();
        if (serverType.equalsIgnoreCase("tpc")) {
            Server.threadPerClient(
                    port,
                    () -> new StompMessagingProtocolImp<>(),
                    () -> new MessageEncoderDecoderImp() 
            ).serve();
        } else if (serverType.equalsIgnoreCase("reactor")) {
            Server.reactor(
                    Runtime.getRuntime().availableProcessors(),
                    port,
                    () -> new StompMessagingProtocolImp<>(),
                    () -> new MessageEncoderDecoderImp(),
                    connections
            ).serve();
        } else {
            System.out.println("Invalid server type. Use 'tpc' for Thread-Per-Client or 'reactor' for Reactor.");
        }
    }
}
