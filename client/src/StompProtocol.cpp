#include "../include/StompProtocol.h"
#include <iostream>
#include <sstream>
#include "StompProtocol.h"
#include <atomic>
#include <event.h>
#include <ctime> 
#include <iomanip> 

using namespace std;

StompProtocol::StompProtocol(ConnectionHandler &connectionHandler):connectionHandler(connectionHandler), 
          isConnected(false),
          password(""),
          username(""),
          subscriptionid(0),
          reciptid(0),
          subscribedChannels()
{
}

void StompProtocol::handleKeyboardInput()
{
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

void StompProtocol::handleServerCommunication()
{
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleLogin(const std::string &input) {
    if (isConnected) {
        cout << "The client is already logged in, log out before tryingagain" << endl;
        return;
    }

    // Split the input into arguments using parseCommand
    auto args = parseCommand(input);

    // Check if the command has exactly 5 arguments (login, host, port, username, password)
    if (args.size() != 5 || args["command"] != "login") {
        cout << "login command needs 3 args: {host:port} {username} {password}" << endl;
        return;
    }

    string host = args["host"];
    string portStr = args["port"];
    short port = 0;
    try {
        port = stoi(portStr); // Convert port to short
    } catch (...) {
        cout << "Invalid port number" << endl;
        return;
    }

    username = args["username"];
    password = args["password"];

    // Validate host and port
    if (host != "127.0.0.1" || port != 7777) {
        cout << "host:port are illegal" << endl;
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
    // להתמודד עם ארור מהסרבר

    isConnected = true;
    cout << "Login successful" << endl;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleJoin(const std::string &input) {
    if (!isConnected) {
        cerr << "Please login first." << endl;
        return;
    }

    auto args = splitInput(input);

    if (args.size() != 2 || args[0] != "join") {
        cerr << "join command needs 1 args: {channel_name}" << endl;
        return;
    }
    
    string channel = args[1];

    // Build and send a SUBSCRIBE frame
    string subscribeFrame = "SUBSCRIBE\n"
                            "destination:" + channel + "\n"
                            "id:" + to_string(subscriptionid.fetch_add(1)) + "/n"
                            "receipt" + to_string(reciptid.fetch_add(1)) + 
                             "\n\n\0";
    if (!connectionHandler.sendLine(subscribeFrame)) {
        cerr << "Failed to send SUBSCRIBE frame" << endl;
        return;
    }
    
    subscribedChannels[channel] = subscriptionid;

    cout << "Joined channel: " << channel << endl;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleExit(const std::string &input) {
    if (!isConnected) {
        cerr << "Please login first" << endl;
        return;
    }
    auto args = splitInput(input);

    if (args.size() != 2 || args[0] != "exit") {
        cerr << "exit command needs 1 args: {channel_name}" << endl;
        return;
    }

    string channelName = args[1];

    auto it = subscribedChannels.find(channelName);
    if (it == subscribedChannels.end()) {
        cerr << "You are not subscribed to channel: " << channelName << endl;
        return;
    }

    int id = it->second;

    string unsubscribeFrame = "UNSUBSCRIBE\n"
                              "id:" + to_string(id) + "\n"
                              "receipt:" + to_string(reciptid.fetch_add(1)) + 
                              "\n\n\0";

    if (!connectionHandler.sendLine(unsubscribeFrame)) {
        cerr << "Failed to send UNSUBSCRIBE frame for channel: " << channelName << endl;
        return;
    }

    subscribedChannels.erase(it);

    cout << "Exited channel: " << channelName << endl;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleLogout(const std::string &input) {
    if (!isConnected) {
        cerr << "Please login first" << endl;
        return;
    }
    auto args = splitInput(input);

    if (args.size() != 1) {
        cerr << "logout command needs 0 args" << endl;
        return;
    } 
    int shouldDisconnect = reciptid.fetch_add(1);
    string disconnectFrame = "DISCONNECT\n\n\0"
                             "receipt" + std::to_string(shouldDisconnect);
    if (!connectionHandler.sendLine(disconnectFrame)) {
        cerr << "Failed to send DISCONNECT frame" << endl;
        return;
    }
    // התמודדות עם קבלה של רסיפט וסגירה של הסוקט!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    isConnected = false;
    connectionHandler.close();
    subscribedChannels.clear(); 
    cout << "Logged out successfully." << endl;
}

void StompProtocol::handleReport(const std::string &input)
{
    if (!isConnected) {
        cerr << "Please login first" << endl;
        return;
    }
    auto args = splitInput(input);

    if (args.size() != 2 || args[0] != "report") {
        cerr << "exit command needs 1 args: {file}" << endl;
        return;
    }
    string fileName = args[1];

    names_and_events parsedData;
    try {
        parsedData = parseEventsFile(fileName); 
    } catch (const std::exception &e) {
        cerr << "Failed to parse file: " << e.what() << endl;
        return;
    }
    vector<Event> events = parsedData.events;
    sort(events.begin(), events.end(), [](const Event &a, const Event &b) {
        return a.get_date_time() < b.get_date_time();
    });
    string channelName = parsedData.channel_name;
    for (const Event &event : events) {
        string eventFrame = "SEND\n"
                            "destination:" + channelName + "\n\n" +
                            "user:" + username + "\n" +
                            "event name:" + event.get_name() + "\n" +
                            "city:" + event.get_city() + "\n" +
                            "date time:" + to_string(event.get_date_time()) + "\n" +
                            "description:" + event.get_description() + "\n" +
                            "general information:\n";
        const map<string, string> &generalInfo = event.get_general_information();
        for (const auto &entry : generalInfo) {
            eventFrame += "  " + entry.first + ":" + entry.second + "\n";
        }
        eventFrame +="\0";
        if (!connectionHandler.sendLine(eventFrame)) {
            cerr << "Failed to send event to channel: " << channelName << endl;
            return;
        }
    }

} 
////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleSummary(const std::string &input)
{
}

////////////////////////////////////////////////////////////////////////////////

map<string, string> StompProtocol::parseCommand(const string &input) {
    istringstream iss(input);
    vector<string> tokens;
    string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    map<string, string> args;

    if (tokens.size() < 4) {
        return args; 
    }

    args["command"] = tokens[0];



    size_t colonPos = tokens[1].find(':');
    if (colonPos != string::npos) {
        args["host"] = tokens[1].substr(0, colonPos);
        args["port"] = tokens[1].substr(colonPos + 1);

    }

    args["username"] = tokens[2];
    args["password"] = tokens[3];
    return args;
}

vector<string> StompProtocol::splitInput(const string &input)
{
    istringstream iss(input);
    vector<string> tokens;
    string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}


