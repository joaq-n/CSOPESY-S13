#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <chrono>
#include <random>
#include "instruction.h"

enum class ProcessState {
    READY,
    RUNNING,
    WAITING,
    FINISHED
};

class Process {
private:
    static int next_id;
    static std::mutex id_mutex;
    
public:
    int id;
    std::string name;
    ProcessState state;
    std::vector<Instruction> instructions;
    int current_instruction;
    std::map<std::string, uint16_t> variables;
    std::vector<std::string> output_logs;
    int sleep_ticks_remaining;
    int cpu_core_assigned;
    std::chrono::steady_clock::time_point creation_time;
    std::chrono::steady_clock::time_point finish_time;
    int for_stack[3]; // For nested loops (max 3 levels)
    int for_stack_size;
    int for_current_repeat[3];
    int total_instructions_executed;
    
    Process(const std::string& process_name);
    
    void generateRandomInstructions(int min_ins, int max_ins);
    bool executeNextInstruction(int delays_per_exec);
    void addOutput(const std::string& output);
    bool isFinished() const { return state == ProcessState::FINISHED; }

    double getCompletionPercentage() const {
        if (instructions.empty()) return 0.0;
        return (static_cast<double>(total_instructions_executed) / instructions.size()) * 100.0;
    }
    
private:
    void executeInstruction(const Instruction& inst);
    std::string evaluateExpression(const std::string& expr);

    void generateInstructionsRecursive(int target_count, std::mt19937& gen, 
        std::uniform_int_distribution<>& ins_type_dist,
        std::uniform_int_distribution<>& value_dist,
        std::uniform_int_distribution<>& sleep_dist,
        std::uniform_int_distribution<>& for_repeat_dist,
        std::uniform_int_distribution<>& for_inner_count_dist,
        int nesting_level, int& max_total_instructions);
};

#endif