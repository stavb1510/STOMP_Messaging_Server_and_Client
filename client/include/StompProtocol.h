#pragma once

#include "../include/ConnectionHandler.h"
#include <string>
#include <map>

class StompProtocol {
public:
    StompProtocol(ConnectionHandler &connectionHandler);
    void handleKeyboardInput(); // Handles input from the keyboard
    void handleServerCommunication();

private:
    ConnectionHandler &connectionHandler;
    bool isConnected;

    // Helper methods for specific commands
    void handleLogin(const std::string &input);
    void handleJoin(const std::string &input);
    void handleExit(const std::string &input);
    void handleLogout(const std::string &input);
    void handleReport(const std::string &input);
    void handleSummary(const std::string &input);

    // Utility to parse commands
    std::map<std::string, std::string> parseCommand(const std::string &input);
};
