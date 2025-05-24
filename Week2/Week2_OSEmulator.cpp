#include <iostream>
#include <string>
using namespace std;

// Function for ASCII Header
void printHeader() {
    cout << R"(
   _____  _____  ______ _____  ______   _____ __     __
  / ____|/ ____||  __  |  __ \|  ____||/ ____|\ \   / /
 | |    | (___  | |  | | |__) | |__   | (___   \ \_/ / 
 | |    |\___ \ | |  | | |__/ |  __|  |\___ \   \   /  
 | |____|____) || |__| | |    | |____ |____) |   | |   
  \_____|_____/ |______|_|    |______||_____/    |_|   
)";

    cout << "\033[1;32mHello, Welcome to CSOPESY commandline!\033[0m\n"; // Green text
    cout << "\033[1;33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n"; // Yellow text
    cout << "Enter a command:\n";
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