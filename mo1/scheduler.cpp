#include "scheduler.h"
#include <thread>
#include <chrono>

void Scheduler::initialize(const Config& cfg) {
    config = cfg;
    cpu_cores_busy.resize(config.num_cpu, false);
    running_processes.resize(config.num_cpu, nullptr);
    process_time_slice.resize(config.num_cpu, 0);
}

void Scheduler::startScheduler() {
    if (!scheduler_running) {
        scheduler_running = true;
        scheduler_thread = std::thread(&Scheduler::schedulerLoop, this);
    }
}

void Scheduler::stopScheduler() {
    scheduler_running = false;
    process_generation_active = false;
    
    if (scheduler_thread.joinable()) {
        scheduler_thread.join();
    }
    if (process_generator_thread.joinable()) {
        process_generator_thread.join();
    }
}

void Scheduler::startProcessGeneration() {
    if (!process_generation_active) {
        process_generation_active = true;
        process_generator_thread = std::thread(&Scheduler::processGeneratorLoop, this);
    }
}

void Scheduler::stopProcessGeneration() {
    process_generation_active = false;
    if (process_generator_thread.joinable()) {
        process_generator_thread.join();
    }
}

Process* Scheduler::createProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    
    // Fixed: Process constructor only takes the name parameter
    std::unique_ptr<Process> process(new Process(name));
    process->generateRandomInstructions(config.min_ins, config.max_ins);
    
    Process* process_ptr = process.get();
    all_processes.push_back(std::move(process));
    ready_queue.push(process_ptr);
    
    return process_ptr;
}

Process* Scheduler::findProcess(const std::string& name) {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    
    for (const auto& process : all_processes) {
        if (process->name == name) {
            return process.get();
        }
    }
    return nullptr;
}

std::vector<Process*> Scheduler::getAllProcesses() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    std::vector<Process*> processes;
    
    for (const auto& process : all_processes) {
        processes.push_back(process.get());
    }
    
    return processes;
}

std::vector<Process*> Scheduler::getRunningProcesses() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    std::vector<Process*> processes;
    
    for (const auto& process : all_processes) {
        if (process->state == ProcessState::RUNNING || process->state == ProcessState::READY || process->state == ProcessState::WAITING) {
            processes.push_back(process.get());
        }
    }
    
    return processes;
}

std::vector<Process*> Scheduler::getFinishedProcesses() {
    std::lock_guard<std::mutex> lock(scheduler_mutex);
    std::vector<Process*> processes;
    
    for (const auto& process : all_processes) {
        if (process->state == ProcessState::FINISHED) {
            processes.push_back(process.get());
        }
    }
    
    return processes;
}

double Scheduler::getCPUUtilization() {
    int used_cores = getUsedCores();
    return (static_cast<double>(used_cores) / config.num_cpu) * 100.0;
}

int Scheduler::getUsedCores() {
    int used = 0;
    for (bool busy : cpu_cores_busy) {
        if (busy) used++;
    }
    return used;
}

int Scheduler::getAvailableCores() {
    return config.num_cpu - getUsedCores();
}

void Scheduler::schedulerLoop() {
    while (scheduler_running) {
        cpu_ticks++;
        
        {
            std::lock_guard<std::mutex> lock(scheduler_mutex);
            scheduleProcess();
            executeProcesses();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 100ms per tick
    }
}

void Scheduler::processGeneratorLoop() {
    int tick_count = 0;
    
    while (process_generation_active) {
        tick_count++;
        
        if (tick_count >= config.batch_process_freq) {
            std::string process_name = generateProcessName();
            createProcess(process_name);
            tick_count = 0;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Scheduler::scheduleProcess() {
    if (config.scheduler == "fcfs") {
        // First Come First Serve
        while (!ready_queue.empty()) {
            bool scheduled = false;
            for (int i = 0; i < config.num_cpu; i++) {
                if (!cpu_cores_busy[i]) {
                    Process* process = ready_queue.front();
                    ready_queue.pop();
                    
                    process->state = ProcessState::RUNNING;
                    process->cpu_core_assigned = i;
                    running_processes[i] = process;
                    cpu_cores_busy[i] = true;
                    scheduled = true;
                    break;
                }
            }
            if (!scheduled) break;
        }
    } else if (config.scheduler == "rr") {
        // Round Robin
        while (!ready_queue.empty()) {
            bool scheduled = false;
            for (int i = 0; i < config.num_cpu; i++) {
                if (!cpu_cores_busy[i]) {
                    Process* process = ready_queue.front();
                    ready_queue.pop();
                    
                    process->state = ProcessState::RUNNING;
                    process->cpu_core_assigned = i;
                    running_processes[i] = process;
                    cpu_cores_busy[i] = true;
                    process_time_slice[i] = config.quantum_cycles;
                    scheduled = true;
                    break;
                }
            }
            if (!scheduled) break;
        }
    }
}

void Scheduler::executeProcesses() {
    // First, handle all sleeping processes (not assigned to cores)
    for (const auto& process_ptr : all_processes) {
        Process* process = process_ptr.get();
        if (process->state == ProcessState::WAITING && process->sleep_ticks_remaining > 0) {
            process->sleep_ticks_remaining--;
            if (process->sleep_ticks_remaining == 0) {
                process->state = ProcessState::READY;
                ready_queue.push(process);
            }
        }
    }
    
    // Then handle processes running on CPU cores
    for (int i = 0; i < config.num_cpu; i++) {
        if (cpu_cores_busy[i] && running_processes[i]) {
            Process* process = running_processes[i];
            
            if (process->state == ProcessState::RUNNING) {
                bool continuing = process->executeNextInstruction(config.delays_per_exec);
                
                // Handle Round Robin time quantum
                if (config.scheduler == "rr" && process->state == ProcessState::RUNNING) {
                    process_time_slice[i]--;
                    
                    // Time quantum expired - preempt the process
                    if (process_time_slice[i] <= 0 && continuing && process->state == ProcessState::RUNNING) {
                        process->state = ProcessState::READY;
                        ready_queue.push(process);
                        
                        // CLEAR CORE ASSIGNMENT when preempting
                        process->cpu_core_assigned = -1;
                        running_processes[i] = nullptr;
                        cpu_cores_busy[i] = false;
                        process_time_slice[i] = 0;
                        continue; // Skip the rest of the processing for this core
                    }
                }
                
                if (!continuing || process->state == ProcessState::FINISHED) {
                    // CLEAR CORE ASSIGNMENT when process finishes
                    if (process->state == ProcessState::FINISHED) {
                        process->cpu_core_assigned = -1;
                    }
                    running_processes[i] = nullptr;
                    cpu_cores_busy[i] = false;
                    process_time_slice[i] = 0;
                } else if (process->state == ProcessState::WAITING) {
                    // Process went to sleep, remove from CPU
                    // CLEAR CORE ASSIGNMENT when going to sleep
                    process->cpu_core_assigned = -1;
                    running_processes[i] = nullptr;
                    cpu_cores_busy[i] = false;
                    process_time_slice[i] = 0;
                }
            }
        }
    }
}

std::string Scheduler::generateProcessName() {
    std::string name = "process" + std::to_string(process_counter);
    process_counter++;
    return name;
}