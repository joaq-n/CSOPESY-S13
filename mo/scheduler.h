#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <vector>
#include <memory>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include "process.h"
#include "config.h"

class Scheduler {
private:
    Config config;
    std::vector<std::unique_ptr<Process>> all_processes;
    std::queue<Process*> ready_queue;
    std::vector<Process*> running_processes;
    std::vector<bool> cpu_cores_busy;
    std::vector<int> process_time_slice; // Time slice remaining for each core
    std::atomic<bool> scheduler_running{false};
    std::atomic<bool> process_generation_active{false};
    std::atomic<long long> cpu_ticks{0};
    std::mutex scheduler_mutex;
    std::thread scheduler_thread;
    std::thread process_generator_thread;
    int process_counter = 1;
    int next_process_id = 1;
    
public:
    Scheduler() = default;
    ~Scheduler() { stopScheduler(); }
    
    void initialize(const Config& cfg);
    void startScheduler();
    void stopScheduler();
    void startProcessGeneration();
    void stopProcessGeneration();
    Process* createProcess(const std::string& name);
    Process* findProcess(const std::string& name);
    std::vector<Process*> getAllProcesses();
    std::vector<Process*> getRunningProcesses();
    std::vector<Process*> getFinishedProcesses();
    double getCPUUtilization();
    int getUsedCores();
    int getAvailableCores();
    long long getCurrentTicks() { return cpu_ticks; }
    
private:
    void schedulerLoop();
    void processGeneratorLoop();
    void scheduleProcess();
    void executeProcesses();
    std::string generateProcessName();
};

#endif