#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>

#include "scheduler.h"
#include "process.h"
#include "config.h"
#include "memory_manager.h"

void printHeader() {
    std::cout << R"(
   _____   _____  ______ _____  ______   _____ __     __
  / ____| / ____||  __  |  __ \|  ____| / ___| \ \   / /
 | |     | (___  | |  | | |__) | |__   | (__    \ \_/ / 
 | |      \___ \ | |  | |  __ /|  __|   \___ \   \   /  
 | |____  ____) || |__| | |    | |____  ____) |   | |   
  \_____||_____/ |______|_|    |______||_____/    |_|   
  
)";
}

// CLI Class
class CLI {
private:
    Scheduler scheduler;
    Config config;
    bool initialized = false;
    bool running = true;
    Process* current_screen_process = nullptr;
    
public:
    void run() {
        printHeader();
        std::cout << "Type 'initialize' to start, or 'exit' to quit.\n\n";
        while (running) {
            if (current_screen_process) {
                std::cout << "[" << current_screen_process->name << "] >> ";
            } else {
                std::cout << ">> ";
            }
            
            std::string input;
            std::getline(std::cin, input);
            
            if (current_screen_process) {
                processScreenCommand(input);
            } else {
                processMainMenuCommand(input);
            }
        }
    }
    
private:
    void processMainMenuCommand(const std::string& command) {
        std::vector<std::string> tokens = tokenize(command);
        
        if (tokens.empty()) return;
        
        std::string cmd = tokens[0];
        
        if (cmd == "exit") {
            handleExit();
        } else if (cmd == "initialize") {
            handleInitialize();
        } else if (!initialized && cmd != "initialize") {
            std::cout << "Error: System not initialized. Please run 'initialize' first.\n";
        } else if (cmd == "screen") {
            if (tokens.size() > 1) {
                std::string args;
                for (size_t i = 1; i < tokens.size(); i++) {
                    args += tokens[i];
                    if (i < tokens.size() - 1) args += " ";
                }
                handleScreen(args);
            } else {
                std::cout << "Usage: screen -s <process_name> | screen -r <process_name> | screen -ls\n";
            }
        } else if (cmd == "scheduler-start") {
            handleSchedulerStart();
        } else if (cmd == "scheduler-stop") {
            handleSchedulerStop();
        } else if (cmd == "report-util") {
            handleReportUtil();
        } else if (cmd == "debug") {
            debugProcessStates();
        } else {
            std::cout << "Unknown command: " << cmd << "\n";
            std::cout << "Available commands: initialize, exit, screen, scheduler-start, scheduler-stop, report-util\n";
        }
    }

    void processScreenCommand(const std::string& command) {
        std::vector<std::string> tokens = tokenize(command);
        
        if (tokens.empty()) return;
        
        std::string cmd = tokens[0];
        
        if (cmd == "exit") {
            current_screen_process = nullptr;
            clearScreen();
            printHeader();
        } else if (cmd == "process-smi") {
            if (current_screen_process) {
                std::cout << "\nProcess: " << current_screen_process->name << "\n";
                std::cout << "ID: " << current_screen_process->id << "\n";
                
                if (current_screen_process->isFinished()) {
                    std::cout << "Status: Finished!\n";
                } else {
                    std::cout << "Current instruction line: " << current_screen_process->current_instruction + 1 << " / " << current_screen_process->instructions.size() << "\n";
                    std::cout << "State: ";
                    switch (current_screen_process->state) {
                        case ProcessState::READY: std::cout << "Ready\n"; break;
                        case ProcessState::RUNNING: std::cout << "Running\n"; break;
                        case ProcessState::WAITING: std::cout << "Waiting\n"; break;
                        case ProcessState::FINISHED: std::cout << "Finished\n"; break;
                    }
                }
                
                std::cout << "\nLogs:\n";
                for (const auto& log : current_screen_process->output_logs) {
                    std::cout << log << "\n";
                }
            }
        } else {
            std::cout << "Unknown command in process screen: " << cmd << "\n";
            std::cout << "Available commands: process-smi, exit\n";
        }
    }

    void handleInitialize() {
        if (loadConfig()) {
            scheduler.initialize(config);
            scheduler.startScheduler();
            initialized = true;
            std::cout << "System initialized successfully.\n";
            std::cout << "Configuration:\n";
            std::cout << "CPU cores: " << config.num_cpu << "\n";
            std::cout << "Scheduler: " << config.scheduler;
            if (config.scheduler == "rr") {
                std::cout << " (Round Robin)";
            } else if (config.scheduler == "fcfs") {
                std::cout << " (First Come First Serve)";
            }
            std::cout << "\n";
            if (config.scheduler == "rr") {
                std::cout << "Quantum cycles: " << config.quantum_cycles << "\n";
            }
            std::cout << "Process generation frequency: " << config.batch_process_freq << " ticks\n";
            std::cout << "Instructions per process: " << config.min_ins << "-" << config.max_ins << "\n";
            
            // Display memory configuration
            std::cout << "\nMemory Configuration:\n";
            std::cout << "Total memory: " << config.max_overall_mem << " KB\n";
            std::cout << "Memory per frame: " << config.mem_per_frame << " KB\n";
            std::cout << "Memory per process: " << config.mem_per_proc << " KB\n";
            std::cout << "Maximum processes in memory: " << (config.max_overall_mem / config.mem_per_proc) << "\n";
        } else {
            std::cout << "Failed to load configuration. Using default values.\n";
            scheduler.initialize(config);
            scheduler.startScheduler();
            initialized = true;
        }
    }

    void handleScreen(const std::string& args) {
        std::vector<std::string> tokens = tokenize(args);
        
        if (tokens.size() < 1) {
            std::cout << "Usage: screen -s <process_name> | screen -r <process_name> | screen -ls\n";
            return;
        }
        
        if (tokens[0] == "-s" && tokens.size() >= 2) {
            std::string process_name = tokens[1];
            Process* process = scheduler.createProcess(process_name);
            current_screen_process = process;
            clearScreen();
            std::cout << "Created and attached to process: " << process_name << "\n";
        } else if (tokens[0] == "-r" && tokens.size() >= 2) {
            std::string process_name = tokens[1];
            Process* process = scheduler.findProcess(process_name);
            if (process && !process->isFinished()) {
                current_screen_process = process;
                clearScreen();
                std::cout << "Attached to process: " << process_name << "\n";
            } else {
                std::cout << "Process " << process_name << " not found or finished.\n";
            }
        } else if (tokens[0] == "-ls") {
            handleScreenList();
        } else {
            std::cout << "Usage: screen -s <process_name> | screen -r <process_name> | screen -ls\n";
        }
    }

    void handleSchedulerStart() {
        scheduler.startProcessGeneration();
        std::cout << "Scheduler started. Generating processes...\n";
    }

    void handleSchedulerStop() {
        scheduler.stopProcessGeneration();
        std::cout << "Scheduler stopped.\n";
    }

    void handleReportUtil() {
        std::cout << "\nCPU Utilization Report\n";
        std::cout << "======================\n";
        std::cout << "CPU utilization: " << std::fixed << std::setprecision(2) << scheduler.getCPUUtilization() << "%\n";
        std::cout << "Cores used: " << scheduler.getUsedCores() << "\n";
        std::cout << "Cores available: " << scheduler.getAvailableCores() << "\n";
        std::cout << "Current CPU ticks: " << scheduler.getCurrentTicks() << "\n";
        
        // Add memory utilization information
        std::cout << "\nMemory Utilization\n";
        std::cout << "==================\n";
        std::cout << "Processes in memory: " << scheduler.getProcessesInMemory() << "\n";
        std::cout << "Total external fragmentation: " << scheduler.getTotalExternalFragmentation() << " KB\n";
        
        auto running_processes = scheduler.getRunningProcesses();
        auto finished_processes = scheduler.getFinishedProcesses();
        
        std::cout << "\nRunning processes: " << running_processes.size() << "\n";
        for (const auto& process : running_processes) {
            std::cout << "  " << process->name << " (ID: " << process->id << ")\n";
        }
        
        std::cout << "\nFinished processes: " << finished_processes.size() << "\n";
        for (const auto& process : finished_processes) {
            std::cout << "  " << process->name << " (ID: " << process->id << ")\n";
        }
        
        saveReport();
        std::cout << "\nReport saved to report-util.txt\n";
    }

    void handleScreenList() {
        std::cout << "\nCPU utilization: " << std::fixed << std::setprecision(2) << scheduler.getCPUUtilization() << "%\n";
        std::cout << "Cores used: " << scheduler.getUsedCores() << "\n";
        std::cout << "Cores available: " << scheduler.getAvailableCores() << "\n";
        
        // Add memory information
        std::cout << "Processes in memory: " << scheduler.getProcessesInMemory() << "\n";
        std::cout << "External fragmentation: " << scheduler.getTotalExternalFragmentation() << " KB\n\n";
        
        auto running_processes = scheduler.getRunningProcesses();
        auto finished_processes = scheduler.getFinishedProcesses();
        
        std::cout << "Running processes:\n";
        for (const auto& process : running_processes) {
            // Show ALL processes that are not finished (RUNNING, READY, or WAITING)
            if (process->state != ProcessState::FINISHED) {
                // Get current timestamp
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::tm* tm_ptr = std::localtime(&time_t);
                
                std::ostringstream timestamp;
                timestamp << std::put_time(tm_ptr, "%m/%d/%Y, %I:%M:%S%p");
                
                // Show core assignment or status
                std::cout << process->name 
                        << " (" << timestamp.str() << ") ";
                
                if (process->cpu_core_assigned >= 0) {
                    std::cout << "Core: " << process->cpu_core_assigned;
                } else {
                    std::cout << "Core: Not assigned";
                }
                
                std::cout << " " << process->current_instruction << "/" << process->instructions.size() << "\n";
            }
        }
    
        std::cout << "\nFinished processes:\n";
        for (const auto& process : finished_processes) {
            // Get current timestamp for finished processes
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm* tm_ptr = std::localtime(&time_t);
            
            std::ostringstream timestamp;
            timestamp << std::put_time(tm_ptr, "%m/%d/%Y, %I:%M:%S%p");
            
            std::cout << process->name 
                    << " (" << timestamp.str() << ") "
                    << "Finished " << process->instructions.size() << "/" << process->instructions.size()
                    << "\n";
        }
    }

    void debugProcessStates() {
        auto running_processes = scheduler.getRunningProcesses();
        
        std::cout << "\n=== DEBUG: Process States ===\n";
        std::cout << "Processes in memory: " << scheduler.getProcessesInMemory() << "\n";
        std::cout << "External fragmentation: " << scheduler.getTotalExternalFragmentation() << " KB\n\n";
        
        for (const auto& process : running_processes) {
            std::cout << process->name << " - State: ";
            switch (process->state) {
                case ProcessState::READY: std::cout << "READY"; break;
                case ProcessState::RUNNING: std::cout << "RUNNING"; break;
                case ProcessState::WAITING: std::cout << "WAITING"; break;
                case ProcessState::FINISHED: std::cout << "FINISHED"; break;
            }
            std::cout << " - Core: " << process->cpu_core_assigned;
            std::cout << " - Instruction: " << process->current_instruction + 1 << "/" << process->instructions.size();
            std::cout << " - Sleep ticks: " << process->sleep_ticks_remaining;
            
            // Show current instruction type
            if (process->current_instruction < process->instructions.size()) {
                auto& inst = process->instructions[process->current_instruction];
                std::cout << " - Current inst: ";
                switch (inst.type) {
                    case InstructionType::PRINT: std::cout << "PRINT"; break;
                    case InstructionType::DECLARE: std::cout << "DECLARE"; break;
                    case InstructionType::ADD: std::cout << "ADD"; break;
                    case InstructionType::SUBTRACT: std::cout << "SUBTRACT"; break;
                    case InstructionType::SLEEP: std::cout << "SLEEP(" << inst.args[0] << ")"; break;
                    case InstructionType::FOR_START: std::cout << "FOR_START"; break;
                    case InstructionType::FOR_END: std::cout << "FOR_END"; break;
                }
            }
            std::cout << "\n";
        }
        std::cout << "==============================\n\n";
    }

    void handleExit() {
        scheduler.stopScheduler();
        running = false;
        std::cout << "Goodbye!\n";
    }

    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    bool loadConfig() {
        std::ifstream file("config.txt");
        if (!file.is_open()) {
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string key, value;
            
            if (iss >> key >> value) {
                if (key == "num-cpu") {
                    config.num_cpu = std::stoi(value);
                } else if (key == "scheduler") {
                    config.scheduler = value;
                } else if (key == "quantum-cycles") {
                    config.quantum_cycles = std::stoi(value);
                } else if (key == "batch-process-freq") {
                    config.batch_process_freq = std::stoi(value);
                } else if (key == "min-ins") {
                    config.min_ins = std::stoi(value);
                } else if (key == "max-ins") {
                    config.max_ins = std::stoi(value);
                } else if (key == "delays-per-exec") {
                    config.delays_per_exec = std::stoi(value);
                } else if (key == "max-overall-mem") {
                    config.max_overall_mem = std::stoi(value);
                } else if (key == "mem-per-frame") {
                    config.mem_per_frame = std::stoi(value);
                } else if (key == "mem-per-proc") {
                    config.mem_per_proc = std::stoi(value);
                }
            }
        }
        
        file.close();
        return true;
    }

    void saveReport() {
        std::ofstream file("report-util.txt");
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        file << "CPU Utilization Report\n";
        file << "Generated at: " << std::ctime(&time_t);
        file << "==============================\n\n";
        
        file << "CPU utilization: " << std::fixed << std::setprecision(2) << scheduler.getCPUUtilization() << "%\n";
        file << "Cores used: " << scheduler.getUsedCores() << "\n";
        file << "Cores available: " << scheduler.getAvailableCores() << "\n";
        file << "Current CPU ticks: " << scheduler.getCurrentTicks() << "\n\n";
        
        // Add memory information to report
        file << "Memory Utilization:\n";
        file << "Processes in memory: " << scheduler.getProcessesInMemory() << "\n";
        file << "Total external fragmentation: " << scheduler.getTotalExternalFragmentation() << " KB\n\n";
        
        auto running_processes = scheduler.getRunningProcesses();
        auto finished_processes = scheduler.getFinishedProcesses();
        
        file << "Running processes: " << running_processes.size() << "\n";
        for (const auto& process : running_processes) {
            file << "  " << process->name << " (ID: " << process->id << ")\n";
        }
        
        file << "\nFinished processes: " << finished_processes.size() << "\n";
        for (const auto& process : finished_processes) {
            file << "  " << process->name << " (ID: " << process->id << ")\n";
        }
        
        file.close();
    }

    std::vector<std::string> tokenize(const std::string& str) {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;
        
        while (iss >> token) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
};

int main() {
    CLI cli;
    cli.run();
    return 0;
}