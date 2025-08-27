# STOMP Client-Server Messaging System
This project implements a complete client-server messaging system using the STOMP 1.2 protocol. The system was developed as part of the Systems Programming Laboratory (SPL251) course at Ben-Gurion University of the Negev. It includes a multithreaded server and a command-line client, enabling users to connect, subscribe to topics, send messages, and disconnect—simulating real-world publish-subscribe behavior over TCP sockets.

## Features
- Full support for STOMP 1.2 commands (see below)
- Concurrent client handling via Reactor pattern
- Subscription management per topic
- Message broadcasting to relevant subscribers
- Graceful handling of receipts and error frames
- Docker-compatible development environment

## Supported STOMP Commands
- `CONNECT`: Establish a connection and authenticate
- `SUBSCRIBE`: Subscribe to a topic (destination)
- `SEND`: Send a message to a topic
- `UNSUBSCRIBE`: Unsubscribe from a topic
- `DISCONNECT`: Clean disconnection from the server
- `RECEIPT`: Acknowledge server/client operations
- `ERROR`: Sent by server in case of protocol violation

## Structure
- `server/`: Server-side logic and STOMP protocol implementation
- `client/`: CLI-based STOMP client that parses user commands and interacts with the server
- `.devcontainer/`: Docker environment setup for uniform builds
- `input/`: Contains example configuration or input files for simulation

## Build and Run

1. **Clone the repository**  
   `git clone https://github.com/stavb1510/SPL251_Assignment3.git && cd SPL251_Assignment3`

2. **Build the project**  
   `mvn clean install`

3. **Run the server**  
   From the `server/` directory:  
   `mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.ServerMain"`

4. **Run the client**  
   From the `client/bin` directory:  
   `./StompEsClient`  
   (or compile and run from source using Maven)

## Developer
**Stav Balaish**  
Ben-Gurion University of the Negev  