CFLAGS := -c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS := -lboost_system -lpthread
BIN_DIR := bin
SRC_DIR := src

# Default target
all: StompEMIClient

# Build the main StompEMClient executable
StompEMIClient: $(BIN_DIR)/ConnectionHandler.o $(BIN_DIR)/StompProtocol.o $(BIN_DIR)/StompClient.o $(BIN_DIR)/event.o
	g++ -o $(BIN_DIR)/StompEMIClient $(BIN_DIR)/ConnectionHandler.o $(BIN_DIR)/StompProtocol.o $(BIN_DIR)/StompClient.o $(BIN_DIR)/event.o $(LDFLAGS)

# Compile each source file into object files
$(BIN_DIR)/ConnectionHandler.o: $(SRC_DIR)/ConnectionHandler.cpp
	g++ $(CFLAGS) -o $(BIN_DIR)/ConnectionHandler.o $(SRC_DIR)/ConnectionHandler.cpp

$(BIN_DIR)/StompProtocol.o: $(SRC_DIR)/StompProtocol.cpp
	g++ $(CFLAGS) -o $(BIN_DIR)/StompProtocol.o $(SRC_DIR)/StompProtocol.cpp

$(BIN_DIR)/StompClient.o: $(SRC_DIR)/StompClient.cpp
	g++ $(CFLAGS) -o $(BIN_DIR)/StompClient.o $(SRC_DIR)/StompClient.cpp

$(BIN_DIR)/event.o: $(SRC_DIR)/event.cpp
	g++ $(CFLAGS) -o $(BIN_DIR)/event.o $(SRC_DIR)/event.cpp

# Clean target to remove all build artifacts
.PHONY: clean
clean:
	rm -f $(BIN_DIR)/*