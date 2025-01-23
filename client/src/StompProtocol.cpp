#include "../include/StompProtocol.h"
#include <iostream>
#include <sstream>

using namespace std;

StompProtocol::StompProtocol(ConnectionHandler &connectionHandler)
    : connectionHandler(connectionHandler), isConnected(false) {}

void StompProtocol::handleKeyboardInput() {
    string input;
    while (getline(cin, input)) {
        if (input.empty()) {
            continue;
        }

        istringstream iss(input);
        string command;
        iss >> command;

        if (command == "login") {
            handleLogin(input);
        } else if (command == "join") {
            handleJoin(input);
        } else if (command == "exit") {
            handleExit(input);
        } else if (command == "logout") {
            handleLogout(input);
        } else if (command == "report") {
            handleReport(input);
        } else if (command == "summary") {
            handleSummary(input);
        } else {
            if (!isConnected){
                cerr << "please login first" << endl;
            }
            ////////// check what to print here
            cerr << "Unknown command: " << command << endl;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleLogin(const std::string &input) {
    if (isConnected) {
        cout << "Already logged in." << endl;
        return;
    }
    auto args = parseCommand(input);

    // Check if the command has exactly 4 arguments (login, host:port, username, password)
    if (args.size() != 4) {
        cout << "login command needs 3 args: {host:port} {username} {password}" << endl;
        return;
    }

    string host = args["host"];
    string port = args["port"];
    string username = args["username"];
    string password = args["password"];
    if(host != "127.0.0.1" || port != "7777") {
        cout << "host:port are illegal" << endl;
    }

    if (!connectionHandler.connect()) {
        cerr << "Failed to connect to server at " << host << ":" << port << endl;
        return;
    }

    // Build and send a STOMP CONNECT frame
    string connectFrame = "CONNECT\n"
                          "accept-version:1.2\n"
                          "host:" + host + "\n"
                          "login:" + username + "\n"
                          "passcode:" + password + "\n\n\0";
    if (!connectionHandler.sendLine(connectFrame)) {
        cerr << "Failed to send CONNECT frame" << endl;
        return;
    }

    isConnected = true;
    cout << "Login successful" << endl;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleJoin(const std::string &input) {
    if (!isConnected) {
        cerr << "Not connected. Please login first." << endl;
        return;
    }

    auto args = parseCommand(input);
    string channel = args["channel"];

    // Build and send a SUBSCRIBE frame
    string subscribeFrame = "SUBSCRIBE\n"
                            "destination:" + channel + "\n"
                            "id:" + to_string(rand()) + "\n\n\0";
    if (!connectionHandler.sendLine(subscribeFrame)) {
        cerr << "Failed to send SUBSCRIBE frame" << endl;
        return;
    }

    cout << "Joined channel: " << channel << endl;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleExit(const std::string &input) {
    if (!isConnected) {
        cerr << "Not connected. Please login first." << endl;
        return;
    }

    auto args = parseCommand(input);
    string channel = args["channel"];

    // Build and send an UNSUBSCRIBE frame
    string unsubscribeFrame = "UNSUBSCRIBE\n"
                              "destination:" + channel + "\n\n\0";
    if (!connectionHandler.sendLine(unsubscribeFrame)) {
        cerr << "Failed to send UNSUBSCRIBE frame" << endl;
        return;
    }

    cout << "Exited channel: " << channel << endl;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleLogout(const std::string &input) {
    if (!isConnected) {
        cerr << "Not connected. Please login first." << endl;
        return;
    }

    // Build and send a DISCONNECT frame
    string disconnectFrame = "DISCONNECT\n\n\0";
    if (!connectionHandler.sendLine(disconnectFrame)) {
        cerr << "Failed to send DISCONNECT frame" << endl;
        return;
    }

    isConnected = false;
    connectionHandler.close();
    cout << "Logged out successfully." << endl;
}

////////////////////////////////////////////////////////////////////////////////

/*void StompProtocol::handleReport(const std::string &input) {
    if (!isConnected) {
        cerr << "Not connected. Please login first." << endl;
        return;
    }

    auto args = parseCommand(input);
    string reportFilePath = args["file"];

    // Open and parse the report file
    ifstream reportFile(reportFilePath);
    if (!reportFile.is_open()) {
        cerr << "Failed to open report file: " << reportFilePath << endl;
        return;
    }

    json reportData;
    try {
        reportFile >> reportData;
    } catch (const exception &e) {
        cerr << "Error parsing JSON report file: " << e.what() << endl;
        return;
    }

    // Convert the report data to a STOMP MESSAGE frame
    string reportFrame = "SEND\n"
                         "destination:/report\n"
                         "\n" +
                         reportData.dump() + // Serialize JSON data to string
                         "\n\0";

    if (!connectionHandler.sendLine(reportFrame)) {
        cerr << "Failed to send REPORT frame" << endl;
        return;
    }

    cout << "Report sent successfully: " << reportFilePath << endl;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleSummary(const std::string &input) {
    if (!isConnected) {
        cerr << "Not connected. Please login first." << endl;
        return;
    }

    auto args = parseCommand(input);
    string summaryFilePath = args["file"];

    // Placeholder: Open the summary file for local processing
    ifstream summaryFile(summaryFilePath);
    if (!summaryFile.is_open()) {
        cerr << "Failed to open summary file: " << summaryFilePath << endl;
        return;
    }

    // Parse the file (e.g., JSON or custom format)
    json summaryData;
    try {
        summaryFile >> summaryData;
    } catch (const exception &e) {
        cerr << "Error parsing summary file: " << e.what() << endl;
        return;
    }

    // Perform local summary calculations (this is just a placeholder for actual logic)
    cout << "Processing summary for file: " << summaryFilePath << endl;
    for (auto &event : summaryData["events"]) {
        cout << "Event: " << event["name"] << " | Details: " << event["description"] << endl;
    }

    cout << "Summary processed successfully." << endl;
}*/

////////////////////////////////////////////////////////////////////////////////

map<string, string> StompProtocol::parseCommand(const string &input) {
    istringstream iss(input);
    map<string, string> args;
    string key, value;

    // Split the input into key:value pairs
    while (getline(iss, key, ':') && getline(iss, value, ' ')) {
        args[key] = value;
    }

    return args;
}