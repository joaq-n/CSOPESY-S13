#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <vector>
#include <mutex>

#include "process.h"
#include "process_list.h"
#include "process_map.h"
#include "instruction_tracker.h"
#include "schedulersimulator.h"

// Declare global variable
int numCPU = 1;  // Default number of CPU cores
SchedulerSimulator* globalScheduler = nullptr;
std::atomic<bool> schedulerRunning(false);

// Function for get current timestamp
std::string getCurrentTimestamp() {
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::stringstream ss;
    ss << std::put_time(ltm, "%m/%d/%Y, %I:%M:%S %p");
    return ss.str();
}

void SchedulerSimulator::coreWorker(int coreId, ProcessList* readyQueue) {
    while (simulationRunning.load()) {
        Process* currentProcess = nullptr;
        {
            std::lock_guard<std::mutex> lock(queueMutex);

            for (auto* proc : readyQueue->processes) {
                if (!proc->isFinished() && proc->coreAssigned == -1) { // -1 means not assigned to a core yet
                    currentProcess = proc;
                    currentProcess->coreAssigned = coreId; // Assign this core
                    break;
                }
            }
        }

        if (currentProcess) {
            // Simulate execution based on scheduler type
            if (schedulerType == "fcfs") {
                // FCFS logic
                while (!currentProcess->isFinished() && simulationRunning.load()) {
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        if (!currentProcess->isFinished()) {
                            currentProcess->executedInstructions += 1;
                        } else {
                            currentProcess->coreAssigned = -1; // Mark core free
                            break; // Exit inner loop if process finished
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
                }
                // Ensure core is marked as free if it wasn't already marked finished above
                std::lock_guard<std::mutex> lock(queueMutex);
                if (currentProcess->isFinished()) {
                    currentProcess->coreAssigned = -1;
                }
            }
            else if (schedulerType == "rr") {
                // Round Robin: execute for a quantum (not required for this week)
                int instructionsToExecute = quantumCycles;
                while (instructionsToExecute > 0 && !currentProcess->isFinished() && simulationRunning.load()) {
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        if (!currentProcess->isFinished()) { // Re-check after acquiring lock
                            currentProcess->executedInstructions++;
                            instructionsToExecute--;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
                }
                std::lock_guard<std::mutex> lock(queueMutex);
                currentProcess->coreAssigned = -1; // Make core available
            }
        } else {
            // No process available for this core, sleep briefly to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void SchedulerSimulator::loadConfig(const std::string& filename) {
    std::ifstream infile(filename);
    std::string line;

    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "num-cpu") iss >> numCPU;
        else if (key == "scheduler") {
            if (!(iss >> std::quoted(schedulerType))) {
                iss.clear();
                iss >> schedulerType;  // fallback if not quoted
            }
        }
        else if (key == "quantum-cycles") iss >> quantumCycles;
        else if (key == "batch-process-freq") iss >> batchFreq;
        else if (key == "min-ins") iss >> minInstructions;
        else if (key == "max-ins") iss >> maxInstructions;
        else if (key == "delay-per-exec") iss >> delayPerExec;
    }
}

void SchedulerSimulator::stopSimulation() {
    simulationRunning.store(false); // Signal worker threads to stop
    for (auto& t : workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    workerThreads.clear();
}

void SchedulerSimulator::runTest() {
    loadConfig("config.txt");

    if (schedulerType == "fcfs") {
        runFCFS();
    } else if (schedulerType == "rr") {
        runRoundRobin();
    } else {
        std::cerr << "Unknown scheduler type: " << schedulerType << "\n";
    }
}

void SchedulerSimulator::runFCFS() {
    // Initialize atomic flag
    simulationRunning.store(true);

    ProcessList readyQueue;
    std::vector<Process*> allProcesses; // Store all created processes

    // Generate processes
    srand(static_cast<unsigned int>(time(0))); // Seed for random instruction counts
    int pidCounter = 0;

    // For demonstration, 10 processes with 100 print statements (100 instructions)
    for (int i = 0; i < 10; ++i) {
        int instructions = 100;
        Process* newProcess = new Process(pidCounter++, "Proc_" + std::to_string(i), instructions);
        newProcess->priority = 1; // FCFS doesn't typically use priority, but keep default
        allProcesses.push_back(newProcess);
    }

    // Create worker threads (one for each CPU core)
    for (int i = 0; i < numCPU; ++i) {
        workerThreads.emplace_back(&SchedulerSimulator::coreWorker, this, i, &readyQueue);
    }

    int releaseIndex = 0; // To track which batch of processes to release
    int cycle = 0;        // Simulation clock cycle

    while (simulationRunning.load()) {
        // --- Main thread handles adding processes and UI updates ---

        // Release new processes into the ready queue at batchFreq intervals
        if (cycle % batchFreq == 0 && releaseIndex < allProcesses.size()) {
            std::lock_guard<std::mutex> lock(queueMutex); // Protect adding to shared queue
            readyQueue.addProcess(allProcesses[releaseIndex]);
            releaseIndex++;
        }

        // Clear screen for UI update
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        // Print current timestamp
        std::cout << "Current Time: " << getCurrentTimestamp() << "\n";

        // Calculate CPU utilization
        // int currentCoresUsed = 0;
      
        // int coresAvailable = std::max(0, numCPU - currentCoresUsed);
        // int utilization = (numCPU > 0) ? (currentCoresUsed * 100 / numCPU) : 0; // Avoid division by zero

        // {
        //     std::lock_guard<std::mutex> lock(coutMutex); // Optional: Protect cout
        //     std::cout << "Scheduler Type: FCFS | Quantum: " << quantumCycles << "\n";
        //     std::cout << "CPU Utilization: " << utilization << "%\n";
        //     std::cout << "Cores used: " << currentCoresUsed << "\n";
        //     std::cout << "Cores available: " << coresAvailable << "\n";
        {
            std::cout << "----------------------------------------------\n";
            std::cout << "Running processes:\n";
            readyQueue.printRunningProcesses(); // These methods need to handle their own locks if they access process details
            std::cout << "\nFinished processes:\n";
            readyQueue.printFinishedProcesses();
            std::cout << "----------------------------------------------\n";
        }


        // Check if all processes are finished to terminate simulation
        bool allProcessesFinished = true;
        {
            std::lock_guard<std::mutex> lock(queueMutex); // Lock to read shared process data
            if (releaseIndex < allProcesses.size()) { // Still processes to be released
                allProcessesFinished = false;
            } else {
                for (const auto& proc : readyQueue.processes) {
                    if (!proc->isFinished()) {
                        allProcessesFinished = false;
                        break;
                    }
                }
            }
        }

        if (allProcessesFinished) {
            std::cout << "All processes finished. Simulation ending.\n";
            simulationRunning.store(false); // Signal worker threads to stop
            break; // Exit main simulation loop
        }

        cycle++;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
    }

    // Join all worker threads to ensure they finish cleanly
    for (auto& t : workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    workerThreads.clear(); // Clear the vector of threads

    // Clean up dynamically allocated processes
    for (Process* proc : allProcesses) {
        sessions.add(proc); // Add each process (pointer) to the global ProcessManager
    }
    allProcesses.clear();
}

void SchedulerSimulator::runRoundRobin() {
    // Initialize atomic flag
    simulationRunning.store(true);

    ProcessList readyQueue; // This will act as our shared ready queue
    std::vector<Process*> allProcesses; // To store all created processes for cleanup

    // Generate processes (similar to FCFS, or as per your existing logic)
    srand(static_cast<unsigned int>(time(0)));
    int pidCounter = 0;
    // For demonstration, let's create a fixed number of processes
    for (int i = 0; i < 10; ++i) {
        int instructions = minInstructions + (rand() % (maxInstructions - minInstructions + 1));
        Process* newProcess = new Process(pidCounter++, "Proc_" + std::to_string(i), instructions);
        newProcess->priority = 1; // Round Robin doesn't typically use priority
        allProcesses.push_back(newProcess);
    }

    // Create worker threads (one for each CPU core)
    // The coreWorker function handles both FCFS and RR based on schedulerType
    for (int i = 0; i < numCPU; ++i) {
        workerThreads.emplace_back(&SchedulerSimulator::coreWorker, this, i, &readyQueue);
    }

    int releaseIndex = 0; // To track which processes to release
    int cycle = 0;        // Simulation clock cycle

    while (true) {
        // --- Main thread handles adding processes and UI updates ---

        // Release new processes into the ready queue at batchFreq intervals
        if (cycle % batchFreq == 0 && releaseIndex < allProcesses.size()) {
            std::lock_guard<std::mutex> lock(queueMutex); // Protect adding to shared queue
            readyQueue.addProcess(allProcesses[releaseIndex]); // Assuming a method like add_process in ProcessList
            releaseIndex++;
        }

        // Clear screen for UI update
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif

        // Print header and current timestamp
        std::cout << "Current Time: " << getCurrentTimestamp() << "\n";

        // Calculate CPU utilization (needs to read shared data safely)
        int currentCoresUsed = 0;
        {
            std::lock_guard<std::mutex> lock(queueMutex); // Lock to read shared process data
            for (const auto& proc : readyQueue.processes) {
                if (!proc->isFinished() && proc->coreAssigned != -1) {
                    currentCoresUsed++;
                }
            }
        }
        // int coresAvailable = std::max(0, numCPU - currentCoresUsed);
        // int utilization = (numCPU > 0) ? (currentCoresUsed * 100 / numCPU) : 0;

        // {
        //     std::lock_guard<std::mutex> lock(coutMutex); // Optional: Protect cout
        //     std::cout << "Scheduler Type: Round Robin | Quantum: " << quantumCycles << "\n";
        //     std::cout << "CPU Utilization: " << utilization << "%\n";
        //     std::cout << "Cores used: " << currentCoresUsed << "\n";
        //     std::cout << "Cores available: " << coresAvailable << "\n";
        {
            std::cout << "----------------------------------------------\n";
            std::cout << "Running processes:\n";
            readyQueue.printRunningProcesses();
            std::cout << "\nFinished processes:\n";
            readyQueue.printFinishedProcesses();
            std::cout << "----------------------------------------------\n";
        }


        // Check if all processes are finished to terminate simulation
        bool allProcessesFinished = true;
        {
            std::lock_guard<std::mutex> lock(queueMutex); // Lock to read shared process data
            if (releaseIndex < allProcesses.size()) { // Still processes to be released
                allProcessesFinished = false;
            } else {
                for (const auto& proc : readyQueue.processes) {
                    if (!proc->isFinished()) {
                        allProcessesFinished = false;
                        break;
                    }
                }
            }
        }

        if (allProcessesFinished) {
            std::cout << "All processes finished. Simulation ending.\n";
            simulationRunning.store(false); // Signal worker threads to stop
            break; // Exit main simulation loop
        }

        cycle++;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
    }

    // Join all worker threads to ensure they finish cleanly
    for (auto& t : workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
    workerThreads.clear(); // Clear the vector of threads

    // Clean up dynamically allocated processes
    for (Process* proc : allProcesses) {
        sessions.add(proc); // Add each process (pointer) to the global ProcessManager
    }
    allProcesses.clear();
}

void SchedulerSimulator::startSimulation() {
    // This function will be called in a separate thread
    loadConfig("config.txt");

    if (schedulerType == "fcfs") {
        runFCFS();
    } else if (schedulerType == "rr") {
        runRoundRobin();
    } else {
        std::cerr << "Unknown scheduler type: " << schedulerType << "\n";
    }
    schedulerRunning.store(false); // Mark simulation as finished
}

std::string getUtilizationReport() {
    // int runningCount = 0;

    // for (const auto& session : sessions) {
    //     if (!session->isFinished()) ++runningCount;
    // }

    // int utilization = (runningCount == 0) ? 0 : std::min(100, (runningCount * 100 / numCPU));
    // int coresAvailable = std::max(0, static_cast<int>(numCPU) - runningCount);

    std::ostringstream output;
    // output << "CPU utilization: " << utilization << "%\n";
    // output << "Cores used: " << runningCount << "\n";
    // output << "Cores available: " << coresAvailable << "\n";
    // output << "----------------------------------------------\n";

    output << "Running processes:\n";
    for (const auto& session : sessions) {
        if (!session->isFinished()) {
            output << session->name << " ("
                   << std::put_time(std::localtime(&session->creationAt), "%m/%d/%Y %I:%M:%S %p") << ")"
                   << " Core: " << session->coreAssigned
                   << " " << session->executedInstructions << " / " << session->totalInstructions << "\n";
        }
    }

    output << "\nFinished processes:\n";
    for (const auto& session : sessions) {
        if (session->isFinished()) {
            output << session->name << " ("
                   << std::put_time(std::localtime(&session->creationAt), "%m/%d/%Y %I:%M:%S %p") << ")"
                   << " Finished " << session->executedInstructions << " / " << session->totalInstructions << "\n";
        }
    }

    output << "----------------------------------------------\n";
    return output.str();
}


// Function for print ASCII header
void printHeader() {
    std::cout << R"(
   _____   _____  ______ _____  ______   _____ __     __
  / ____| / ____||  __  |  __ \|  ____| / ___| \ \   / /
 | |     | (___  | |  | | |__) | |__   | (__    \ \_/ / 
 | |      \___ \ | |  | |  __ /|  __|   \___ \   \   /  
 | |____  ____) || |__| | |    | |____  ____) |   | |   
  \_____||_____/ |______|_|    |______||_____/    |_|   
)";
    std::cout << "\033[1;32mHello, Welcome to CSOPESY commandline!\033[0m\n";
    std::cout << "\033[1;33mUse 'screen -s <name>' to create, 'screen -r <name>' to resume\033[0m\n";
    std::cout << "\033[1;33mType 'exit' to quit, 'clear' to clear the screen\033[0m\n";
    std::cout << "Enter a command:\n";
}

// Forward declaration for recognizeCommand
void recognizeCommand(const std::string& cmd);

// Function for draw a screen session
void drawScreenSession(const Process& session) {
    std::cout << "\n========================================\n";
    std::cout << "   SCREEN SESSION: " << session.name << "\n";
    std::cout << "========================================\n";
    std::cout << "Process Name:  " << session.name << "\n";
    std::cout << "Instruction: "; 
    session.printCurrentLine();
    std::cout << "Created At: " << getCurrentTimestamp() << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "[ Output Stream Placeholder ]\n";
    std::cout << "\nType 'exit' to return to the Main Menu.\n";

    std::string cmd;
    while (true) {
        std::cout << "[" << session.name << "] $ ";
        std::getline(std::cin, cmd);
        if (cmd == "exit") {
            printHeader();
            break;
        }
        else if (cmd == "scheduler-test" || cmd == "st" || cmd == "scheduler-start" || cmd == "ss") {
            globalScheduler = new SchedulerSimulator(); 
            schedulerRunning.store(true);
            std::thread schedulerThread(&SchedulerSimulator::startSimulation, globalScheduler);
            schedulerThread.detach();
        } else if (cmd == "screen -ls" || cmd == "sl") {
            std::cout << getUtilizationReport();
        }
        else if (cmd == "report-util") {
            std::ofstream reportFile("report.txt");
            reportFile << getUtilizationReport();
            reportFile.close();
            std::cout << "Report saved to report.txt\n";
        } else {
            std::cout << "Unrecognized command in screen session. Type 'exit' to return.\n";
        }
    }
}

// Main command recognition and routing
void recognizeCommand(const std::string& cmd) {
    if (cmd == "initialize" || cmd == "scheduler-stop") {
        std::cout << cmd << " command recognized. Doing something.\n";
    }
    else if (cmd.substr(0, 9) == "screen -s") {
        if (cmd.length() <= 9 || cmd[9] != ' ' || cmd.substr(10).empty()) {
            std::cout << "Error: Please provide a screen name. Usage: screen -s <name>\n";
            return;
        }
        std::string name = cmd.substr(10);
        if (sessions.exists(name)) {
            std::cout << "Error: A session named '" << name << "' already exists. Use a different name or 'screen -r <name>'.\n";
            return;
        }

        Process* newSession = new Process(-1, name, 0); // Create on heap, get pointer
        newSession->priority = 3; // Use -> for pointer access
        newSession->creationAt = time(0); // Use -> for pointer access
        sessions.add(newSession); // Pass the pointer to the global sessions manager
        std::cout << "Screen session '" << name << "' created successfully.\n";
    }
    else if (cmd.substr(0, 9) == "screen -r") {
        if (cmd.length() <= 9 || cmd[9] != ' ' || cmd.substr(10).empty()) {
            std::cout << "Error: Please provide a screen name. Usage: screen -r <name>\n";
            return;
        }
        std::string name = cmd.substr(10);
        if (sessions.exists(name)) {
            drawScreenSession(*sessions[name]);
        } else {
            std::cout << "Error: No session named '" << name << "' found.\n";
        }
    }
    else if (cmd == "clear") {
        std::cout << "\033[2J\033[1;1H";
        printHeader();
    }
    else if (cmd == "exit") {
        std::cout << "Exiting the program...\n";
        exit(0);
    }
    else {
        std::cout << "Unrecognized command. Try again.\n";
    }
}

int main() {
    std::string command;

    printHeader();

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);
        recognizeCommand(command);
    }

    return 0;
}