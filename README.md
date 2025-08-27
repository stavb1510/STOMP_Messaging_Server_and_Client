# STOMP Messaging Server and Client

This project implements a messaging server and client in Java based on the STOMP 1.2 protocol. The system allows clients to connect, subscribe to topics, send and receive messages, and manage user sessions over TCP. It was developed as part of the Systems Programming Laboratory course at Ben-Gurion University.


## Build and Run

### Prerequisites
- Java 11 or higher
- Maven
- Docker with VSCode DevContainer support (optional but recommended)

### Build
```
mvn clean install -DskipTests
```

### Run Server
```
cd server
mvn exec:java -Dexec.mainClass="bgu.spl.net.impl.stomp.StompServer"
```

### Run Client
```
cd client
mvn exec:java -Dexec.mainClass="bgu.spl.client.StompEsClient"
```

## Features
- Full support for STOMP 1.2 frame types: CONNECT, SEND, SUBSCRIBE, UNSUBSCRIBE, DISCONNECT, ERROR
- Thread-safe connection management using ConcurrentHashMaps
- Reactor pattern for efficient handling of multiple clients
- Configurable via JSON for test automation and flexibility

## Author
Stav Balaish  
stavbalaish2000@gmail.com