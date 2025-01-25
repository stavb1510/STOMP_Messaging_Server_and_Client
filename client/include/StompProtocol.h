#pragma once

#include "../include/ConnectionHandler.h"
#include <string>
#include <map>
#include <atomic>
#include <vector>
using namespace std;
class StompProtocol {
public:
    StompProtocol(ConnectionHandler &connectionHandler);
    void handleKeyboardInput(); // Handles input from the keyboard
    void handleServerCommunication();
    

private:
    ConnectionHandler &connectionHandler;
    bool isConnected;
    std::string password;
    std::string username;
    std::atomic<int> subscriptionid;
    std::atomic<int> reciptid;
    std::map<string, int> subscribedChannels;
    

    // Helper methods for specific commands
    void handleLogin(const std::string &input);
    void handleJoin(const std::string &input);
    void handleExit(const std::string &input);
    void handleLogout(const std::string &input);
    void handleReport(const std::string &input);
    void handleSummary(const std::string &input);

    
    std::map<std::string, std::string> parseCommand(const std::string &input);
    std::vector<string> splitInput(const string &input);
};
