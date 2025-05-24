#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <sstream>
using namespace std;

// Struct for storing session information
struct ScreenSession {
    string name;
    int currentLine;
    int totalLines;
    string createdAt;
};

// Store sessions in a map
map<string, ScreenSession> sessions;

// Function for get current timestamp
string getCurrentTimestamp() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    stringstream ss;
    ss << put_time(ltm, "%m/%d/%Y, %I:%M:%S %p");
    return ss.str();
}

// Function for print ASCII header
void printHeader() {
    cout << R"(
   _____  _____  ______ _____  ______   _____ __     __
  / ____|/ ____||  __  |  __ \|  ____||/ ____|\ \   / /
 | |    | (___  | |  | | |__) | |__   | (___   \ \_/ / 
 | |    |\___ \ | |  | | |__/ |  __|  |\___ \   \   /  
 | |____|____) || |__| | |    | |____ |____) |   | |   
  \_____|_____/ |______|_|    |______||_____/    |_|   
)";

    cout << "\033[1;32mHello, Welcome to CSOPESY commandline!\033[0m\n";
    cout << "\033[1;33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n";
    cout << "Enter a command:\n";
}

// Function for draw a screen session
void drawScreenSession(const ScreenSession& session) {
    cout << "\n========================================\n";
    cout << "   SCREEN SESSION: " << session.name << "\n";
    cout << "========================================\n";
    cout << "Process Name:        " << session.name << "\n";
    cout << "Instruction Line:    " << session.currentLine << "/" << session.totalLines << "\n";
    cout << "Created At:          " << session.createdAt << "\n";
    cout << "----------------------------------------\n";
    cout << "[ Output Stream Placeholder ]\n";
    cout << "\nType 'exit' to return to the Main Menu.\n";

    string cmd;
    while (true) {
        cout << "> ";
        getline(cin, cmd);
        if (cmd == "exit") {
            printHeader();
            break;
        } else {
            cout << "Still in screen session. Type 'exit' to return.\n";
        }
    }
}

// Main command recognition and routing
void recognizeCommand(const string& cmd) {
    if (cmd == "initialize" || cmd == "screen" || cmd == "scheduler-test" ||
             cmd == "scheduler-stop" || cmd == "report-util") {
        cout << cmd << " command recognized. Doing something.\n";
    }
    else if (cmd.substr(0, 9) == "screen -s") {
        string name = cmd.substr(10);
        if (name.empty()) {
            cout << "Please provide a screen name.\n";
            return;
        }
        if (sessions.find(name) != sessions.end()) {
            cout << "Error: A session named '" << name << "' already exists. Use a different name or use 'screen -r " << name << "' to reconnect.\n";
            return;
        }
        ScreenSession newSession = {name, 3, 10, getCurrentTimestamp()};
        sessions[name] = newSession;
        drawScreenSession(newSession);
    }
    else if (cmd.substr(0, 9) == "screen -r") {
        string name = cmd.substr(10);
        if (sessions.find(name) != sessions.end()) {
            drawScreenSession(sessions[name]);
        } else {
            cout << "No such session named '" << name << "'. Use screen -s <name> to create.\n";
        }
    }
    else if (cmd == "clear") {
        cout << "\033[2J\033[1;1H";
        printHeader();
    }
    else if (cmd == "exit") {
        cout << "Exiting the program...\n";
        exit(0);
    }
    else {
        cout << "Unrecognized command. Try again.\n";
    }
}


int main() {
    string command;

    printHeader();

    while (true) {
        cout << "> ";
        getline(cin, command);
        recognizeCommand(command);
    }

    return 0;
}