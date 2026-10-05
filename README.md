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
   `git clone https://github.com/stavb1510/STOMP_Messaging_Server_and_Client.git && cd STOMP_Messaging_Server_and_Client`

2. **Build and run the server** (Java, Maven)
   ```bash
   cd server
   mvn clean compile
   mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer" -Dexec.args="7777 tpc"   # or: reactor
   ```

3. **Build and run the client** (C++, needs Boost)
   ```bash
   cd client
   make
   ./bin/StompEMIClient
   ```

## Developer
**Stav Balaish**  
Ben-Gurion University of the Negev  