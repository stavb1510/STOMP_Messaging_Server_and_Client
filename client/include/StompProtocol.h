#pragma once

#include "../include/ConnectionHandler.h"
#include <string>
#include <map>
#include <atomic>
#include <vector>

class StompProtocol {
public:
    StompProtocol(ConnectionHandler &connectionHandler);
    void handleKeyboardInput(); // Handles input from the keyboard
    void handleServerCommunication();
    

private:
    ConnectionHandler &connectionHandler;
    bool isConnected;
    string password;
    string username;
    atomic<int> subscriptionid;
    atomic<int> reciptid;
    map<string, int> subscribedChannels;
    

    // Helper methods for specific commands
    void handleLogin(const std::string &input);
    void handleJoin(const std::string &input);
    void handleExit(const std::string &input);
    void handleLogout(const std::string &input);
    void handleReport(const std::string &input);
    void handleSummary(const std::string &input);

    
    std::map<std::string, std::string> parseCommand(const std::string &input);
    vector<string> splitInput(const string &input);
};
