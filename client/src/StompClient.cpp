#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"
#include <iostream>
#include <thread>
#include <atomic>

using namespace std;

atomic<bool> shouldTerminate(false);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <host> <port>" << endl;
        return -1;
    }

    string host = argv[1];
    short port = atoi(argv[2]);

    ConnectionHandler connectionHandler(host, port);
    if (!connectionHandler.connect()) {
        cerr << "Cannot connect to " << host << ":" << port << endl;
        return 1;
    }

    StompProtocol stompProtocol(connectionHandler);

    thread keyboardThread(&StompProtocol::handleKeyboardInput, &stompProtocol);

    thread serverThread(&StompProtocol::handleServerCommunication, &stompProtocol);

    keyboardThread.join();
    serverThread.join();

    return 0;
}