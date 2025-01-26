#pragma once

#include "../include/ConnectionHandler.h"
#include <string>
#include <map>
#include <atomic>
#include <vector>
#include <event.h>


using namespace std;
class StompProtocol {
public:
    StompProtocol(ConnectionHandler &connectionHandler);
    void handleKeyboardInput(); // Handles input from the keyboard
    void handleServerCommunication();
    void setConnectFirstTime(bool value);
    std::string epochToDate(time_t epochTime) const;
    

private:
    ConnectionHandler &connectionHandler;
    bool isConnected;
    std::string password;
    std::string username;
    std::atomic<int> subscriptionid;
    std::atomic<int> receiptid;
    std::map<string, int> subscribedChannels;
    std::map<string, string> users;
    bool connectFirstTime;
    std::atomic<bool> shouldTerminate;
    std::atomic<bool> isLoginProcessing;
    std::mutex communicationMutex;
    int shouldDisconnect;
    std::map<std::string, std::vector<Event>> eventsByChannel;
    std::mutex eventsMutex;

    // Helper methods for specific commands
    void handleLogin(const std::string &input);
    void handleJoin(const std::string &input);
    void handleExit(const std::string &input);
    void handleLogout(const std::string &input);
    void handleReport(const std::string &input);
    void handleSummary(const std::string &input);

    // Helper methods for server frame processing
    void handleReceipt(const std::string &frame);
    void handleMessage(const std::string &frame);
    void handleError(const std::string &frame);
    
    std::map<std::string, std::string> parseCommand(const std::string &input);
    std::vector<string> splitInput(const string &input);
};
