#include <iostream>
#include <string>
#include <iomanip>
#include <map>
#include <ctime>
#include <sstream>
using namespace std;

// Function for ASCII Header
void printHeader() {
    cout << R"(
   _____  _____  ______ _____  ______   _____ __     __
  / ____|/ ____||  __  |  __ \|  ____| / ____|\ \   / /
 | |    | (___  | |  | | |__) | |__   | (___   \ \_/ / 
 | |     \___ \ | |  | |  ___/|  __|   \___ \   \   /  
 | |____ ____) || |__| | |    | |____  ____) |   | |   
  \_____|_____/ |______|_|    |______||_____/    |_|    
)";
    cout << "\033[1;32mHello, Welcome to CSOPESY commandline!\033[0m\n"; // Green text
    cout << "\033[1;33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n"; // Yellow text
    cout << "Enter a command:\n";
}

// Function to get the current timestamp
string getCurrentTimestamp() {
    time_t now = time(0);
    tm localTime;
    localtime_s(&localTime, &now);

    ostringstream oss;
    oss << put_time(&localTime, "%m/%d/%Y, %I:%M:%S %p");
    return oss.str();
}

// Class for Console Screen
class ConsoleScreen {
private:
    string processName;
    int currentInstructionLine;
    int totalInstructionLines;
    string timestamp;

public:
    ConsoleScreen() : processName(""), currentInstructionLine(0), totalInstructionLines(0), timestamp(getCurrentTimestamp()) {} // Default constructor

    ConsoleScreen(const string& name) : processName(name), currentInstructionLine(0), totalInstructionLines(0), timestamp(getCurrentTimestamp()) {}

    // Display info
    void display() const {  
        cout << "\n--- SCREEN: " << processName << " ---" << endl;
        cout << "Process Name: " << endl;
        cout << "Instruction: " << currentInstructionLine << "/" << totalInstructionLines << endl;
        cout << "Created On: " << timestamp << endl;
        cout << "Type 'exit' to return to the main menu.\n" << endl;
    }
};

map<string, ConsoleScreen> screens;

// Function to display a screen
void screenView(const string& name) {
    ConsoleScreen& screen = screens[name];
    screen.display();

    string command;
    while (true) {
        cout << "[" << name << "] $ ";
        getline(cin, command);
        if (command == "exit") {
            cout << "\033[2J\033[1;1H";
            break;
        } else {
            cout << "Unrecognized command in screen view. Type 'exit' to return to the main menu.\n";
        }
    }
    printHeader();
}

// Function for Command Recognition
void recognizeCommand(const string& cmd) {
    if (cmd == "initialize" || cmd == "screen" || cmd == "scheduler-test" ||
        cmd == "scheduler-stop" || cmd == "report-util") {
        cout << cmd << " command recognized. Doing something.\n";
    } else if (cmd == "clear") {
        cout << "\033[2J\033[1;1H";
        printHeader();
    } else if (cmd == "exit") {
        cout << "Exiting the program...\n";
        exit(0);
    } else if (cmd == "screen -s") {
        cout << "Please specify a screen name. Usage: screen -s <name>\n";
    } else if (cmd == "screen -r") {
        cout << "Please specify an existing screen name. Usage: screen -r <name>\n";
    } else if (cmd.rfind("screen -s ", 0) == 0) {
        string name = cmd.substr(10);
        if (screens.count(name)) {
            cout << "Screen '" << name << "' already exists.\n";
        } else {
            screens.emplace(name, ConsoleScreen(name));
            cout << "Screen '" << name << "' created.\n";
        }

    } else if (cmd.rfind("screen -r ", 0) == 0) {
        string name = cmd.substr(10);
        if (screens.count(name)) {
            screenView(name);
        } else {
            cout << "Screen '" << name << "' not found.\n";
        }
        
    } else {
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