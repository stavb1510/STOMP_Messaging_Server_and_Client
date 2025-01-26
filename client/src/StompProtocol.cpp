#include "../include/StompProtocol.h"
#include <iostream>
#include <sstream>
#include "StompProtocol.h"
#include <atomic>
#include <event.h>
#include <ctime> 
#include <iomanip> 
#include <mutex>
#include <fstream>
#include <algorithm>
#include <map>
#include <vector>

using namespace std;

StompProtocol::StompProtocol(ConnectionHandler &connectionHandler):connectionHandler(connectionHandler), 
          isConnected(false),
          password(""),
          username(""),
          subscriptionid(0),
          receiptid(0),
          subscribedChannels(),
          users(),
          connectFirstTime(true),
          shouldTerminate(false),
          isLoginProcessing(true),
          shouldDisconnect(-1),
          eventsByChannel(),
          communicationMutex(),
          eventsMutex()
{}

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
            cerr << "Unknown command" << endl;
        }
    }
}

void StompProtocol::handleServerCommunication() {
    while (!shouldTerminate) {
        while (isLoginProcessing) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::string frame;
        // BEGIN: Scope protected by mutex
        {
            std::unique_lock<std::mutex> lock(communicationMutex); // Mutex lock starts here
            // Attempt to read a frame from the server
            if (!connectionHandler.getFrameAscii(frame, '\0')) {
                std::cerr << "Failed to read frame from server. Disconnecting..." << std::endl;
                shouldTerminate = true;
                return;
            }
        }
        // END: Scope protected by mutex

        std::istringstream stream(frame);
        std::string command;
        std::getline(stream, command);

        if (command == "MESSAGE") {
            handleMessage(frame);
        } else if (command == "RECEIPT") {
            handleReceipt(frame);
        } else if (command == "ERROR") {
            handleError(frame);
            break; // In case of ERROR, disconnect.
        } else {
            std::cerr << "Unknown frame type received: " << command << std::endl;
        }
    }
    shouldTerminate = true;
}

void StompProtocol::handleReceipt(const std::string &frame) {
    vector<string> tokens = splitInput(frame);

    string receiptId;
    for (const string &token : tokens) {
        if (token.find("receipt-id:") == 0) {
            receiptId = token.substr(11);
            break;
        }
    }
    if (std::stoi(receiptId) == shouldDisconnect) {
            cout << "Disconnect confirmed by server. Closing connection..." << endl;
            shouldTerminate = true;
            connectionHandler.close();
            subscribedChannels.clear();
            isConnected = false;
            receiptid.store(0);
            subscriptionid.store(0);
            isLoginProcessing = true;
            cout << "Logged out successfully" << endl;
        }
}

    void StompProtocol::handleMessage(const std::string &frame) {
    size_t destinationPos = frame.find("destination:");
    size_t bodyPos = frame.find("\n\n");

    if (destinationPos != std::string::npos && bodyPos != std::string::npos) {
        std::string destination = frame.substr(destinationPos + 12, frame.find('\n', destinationPos) - (destinationPos + 12));
        std::string body = frame.substr(bodyPos + 2);

        cout << "Message received from channel: " << destination << endl;
        cout << "Body: " << body << endl;
        
        // Parse the event from the message body
        Event event(body);

        // Update the local map of events
        {
            std::unique_lock<std::mutex> lock(eventsMutex); // Protect the map
            eventsByChannel[destination].push_back(event);
        }

    } else {
        cerr << "Malformed MESSAGE frame received." << endl;
    }
}

void StompProtocol::handleError(const std::string &frame) {
    size_t bodyPos = frame.find("\n\n");
    std::string errorMessage = (bodyPos != std::string::npos) ? frame.substr(bodyPos + 2) : "Unknown error";

    cerr << "Error frame received: " << errorMessage << endl;
    connectionHandler.close();
    shouldTerminate = true;
    isConnected = false;
    
}

void StompProtocol::setConnectFirstTime(bool value) {
    connectFirstTime = value;
}

////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleLogin(const std::string &input) {
    // Split the input into arguments using parseCommand
    auto args = parseCommand(input);

    // Check if the command has exactly 5 arguments (login, host, port, username, password)
    if (args.size() != 5 || args["command"] != "login") {
        cout << "login command needs 3 args: {host:port} {username} {password}" << endl;
        return;
    }
    if (isConnected) {
            cout << "The client is already logged in, log out before trying again" << endl;
            return;
        }

    if(!connectFirstTime){
        if (!connectionHandler.connect()) {
        cerr << "Cannot connect"  << endl;
        }
    }

    
    string host = args["host"];
    string portStr = args["port"];
    short port = 0;
    try {
        port = stoi(portStr); 
    } catch (...) {
        cout << "Invalid port number" << endl;
        return;
    }

    username = args["username"];
    password = args["password"];

    auto it = users.find(username);
    if (it != users.end()) {
        if (it->second != password) {
            std::cout << "Wrong password, please try again..." << std::endl;
            return;
        } 
    } else {
        users[username] = password;
    }

    // Validate host and port
    if (host != "127.0.0.1" || port != 7777) {
        cout << "host:port are illegal" << endl;
        return;
    }

    // Build and send a STOMP CONNECT frame
    string connectFrame = "CONNECT\n"
                          "accept-version:1.2\n"
                          "host:stomp.cs.bgu.ac.il\n"
                          "login:" + username + "\n"
                          "passcode:" + password + "\n\n\0";

    if (!connectionHandler.sendFrameAscii(connectFrame, '\0')) {
        cerr << "Failed to send CONNECT frame" << endl;
        return;
    }
    // Process the server response
    {
        std::unique_lock<std::mutex> lock(communicationMutex); 
        string frame;
        if (!connectionHandler.getFrameAscii(frame,'\0')) {
            cerr << "Failed to read frame from server. Disconnecting..." << endl;
            return;
        }

        std::istringstream stream(frame);
        std::string command;
        std::getline(stream, command);

        if (command == "CONNECTED") {
            isConnected = true;
            connectFirstTime = false;
            isLoginProcessing = false;
            cout << "Login successful." << endl;
        } else if (command == "ERROR") {
            handleError(frame);
        }
    }
    // END: Scope protected by mutex

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

    if (subscribedChannels.find(channel) != subscribedChannels.end()){
        cout << "already subscribed to that channel" << endl;
        return;
    }

    int subid = subscriptionid.fetch_add(1);
    // Build and send a SUBSCRIBE frame
    string subscribeFrame = "SUBSCRIBE\n"
                            "destination:" + channel + "\n"
                            "id:" + to_string(subid) + "\n" + 
                            "receipt:" + to_string(receiptid.fetch_add(1)) +
                            "\n\n\0";
    if (!connectionHandler.sendFrameAscii(subscribeFrame,'\0')) {
        cerr << "Failed to send SUBSCRIBE frame" << endl;
        return;
    }
    
    subscribedChannels[channel] = subid;

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
                              "id:" + to_string(id) + "\n" + 
                              "receipt:" + to_string(receiptid.fetch_add(1)) +
                              "\n\n\0";

    if (!connectionHandler.sendFrameAscii(unsubscribeFrame, '\0')) {
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
    shouldDisconnect = receiptid.fetch_add(1);
    string disconnectFrame = "DISCONNECT\n"
                             "receipt:" + std::to_string(shouldDisconnect) +
                             "\n\n\0";
    if (!connectionHandler.sendFrameAscii(disconnectFrame,'\0')) {
        cerr << "Failed to send DISCONNECT frame" << endl;
        return;
    }
}

void StompProtocol::handleReport(const std::string &input) {
    if (!isConnected) {
        cerr << "Please login first." << endl;
        return;
    }

    auto args = splitInput(input);

    if (args.size() != 2 || args[0] != "report") {
        cerr << "report command needs 1 argument: {file}" << endl;
        return;
    }

    std::string fileName = args[1];

    names_and_events parsedData = parseEventsFile(fileName);
    try {
        parsedData = parseEventsFile(fileName);
    } catch (const std::exception &e) {
        cerr << "Failed to parse file: " << e.what() << endl;
        return;
    }

    std::string channelName = parsedData.channel_name;
    std::vector<Event> events = parsedData.events;

    for (const Event &event : events) {
        std::string eventFrame = "SEND\n"
                                 "destination:" + channelName + "\n\n" +
                                 "user:" + username + "\n" +
                                 "event name:" + event.get_name() + "\n" +
                                 "city:" + event.get_city() + "\n" +
                                 "date time:" + std::to_string(event.get_date_time()) + "\n" +
                                 "description:" + event.get_description() + "\n" +
                                 "general information:\n";

        const std::map<std::string, std::string> &generalInfo = event.get_general_information();
        for (const auto &entry : generalInfo) {
            eventFrame += "  " + entry.first + ":" + entry.second + "\n";
        }
        eventFrame += "\0";

        if (!connectionHandler.sendFrameAscii(eventFrame, '\0')) {
            cerr << "Failed to send event to channel: " << channelName << endl;
            return;
        }
    }

    cout << "Events reported successfully to channel: " << channelName << endl;
}
////////////////////////////////////////////////////////////////////////////////

void StompProtocol::handleSummary(const std::string &input) {
    auto args = splitInput(input);

    if (args.size() != 4 || args[0] != "summary") {
        cerr << "summary command needs 3 arguments: {channel_name} {user} {file}" << endl;
        return;
    }

    std::string channelName = args[1];
    std::string user = args[2];
    std::string filePath = "../bin/" + args[3]; 

    std::vector<Event> filteredEvents;

    {
        std::unique_lock<std::mutex> lock(eventsMutex);

        if (eventsByChannel.find(channelName) == eventsByChannel.end()) {
            cerr << "Channel " << channelName << " not found." << endl;
            return;
        }

        for (const auto &event : eventsByChannel[channelName]) {
            if (event.getEventOwnerUser() == user) {
                filteredEvents.push_back(event);
            }
        }
    } 

    std::sort(filteredEvents.begin(), filteredEvents.end(), [](const Event &a, const Event &b) {
        if (a.get_date_time() != b.get_date_time()) {
            return a.get_date_time() < b.get_date_time();
        }
        return a.get_name() < b.get_name();
    });

    std::ostringstream summary;
    summary << "Channel " << channelName << "\n";
    summary << "Stats:\n";
    summary << "Total: " << filteredEvents.size() << "\n";

    int activeCount = 0;
    int forcesArrivalCount = 0;

    for (const auto &event : filteredEvents) {
        const auto &generalInfo = event.get_general_information();
        if (generalInfo.find("active") != generalInfo.end() && generalInfo.at("active") == "true") {
            activeCount++;
        }
        if (generalInfo.find("forces_arrival_at_scene") != generalInfo.end() &&
            generalInfo.at("forces_arrival_at_scene") == "true") {
            forcesArrivalCount++;
        }
    }

    summary << "Active: " << activeCount << "\n";
    summary << "Forces arrival at scene: " << forcesArrivalCount << "\n";
    summary << "Event Reports:\n";

    for (const auto &event : filteredEvents) {
        summary << "Report:\n";
        summary << "city: " << event.get_city() << "\n";
        summary << "date time: " << epochToDate(event.get_date_time()) << "\n";
        summary << "event name: " << event.get_name() << "\n";

         std::string description = event.get_description();
         cout <<"helloooooooooo:" <<description<<endl;
        if (description.size() > 27) {
            description = description.substr(0, 27) + "...";
        }
        summary << "description: " << description << "\n";

        const auto &generalInfo = event.get_general_information();
        if (!generalInfo.empty()) {
            summary << "general information:\n";
            for (const auto &entry : generalInfo) {
                summary << "  " << entry.first << ": " << entry.second << "\n";
            }
        }

        summary << "\n";
    }

    std::ofstream outputFile(filePath);
    if (!outputFile.is_open()) {
        cerr << "Failed to open file: " << filePath << endl;
        return;
    }
    outputFile << summary.str();
    outputFile.close();

    cout << "Summary saved to " << filePath << endl;
}
// Helper function to convert epoch to date-time string
std::string StompProtocol::epochToDate(time_t epochTime) const
{
    char buffer[20];
    struct tm *tm_info = localtime(&epochTime);
    strftime(buffer, sizeof(buffer), "%d/%m/%y %H:%M", tm_info);
    return string(buffer);
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