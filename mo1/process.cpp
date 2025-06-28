#include "process.h"
#include <random>
#include <thread>
#include <sstream>
#include <iomanip>

int Process::next_id = 1;
std::mutex Process::id_mutex;

Process::Process(const std::string& process_name) : 
    name(process_name), 
    state(ProcessState::READY),
    current_instruction(0),
    sleep_ticks_remaining(0),
    cpu_core_assigned(-1),
    creation_time(std::chrono::system_clock::now()),
    total_instructions_executed(0),
    for_stack_size(0) {
        std::lock_guard<std::mutex> lock(id_mutex);
        id = next_id++;
        
        // Initialize for loop tracking
        for (int i = 0; i < 3; i++) {
            for_stack[i] = 0;
            for_current_repeat[i] = 0;
        }
    }

// fit value within uint16_t range
uint16_t Process::clamp(int64_t value) {
    if (value < 0) return 0;
    if (value > std::numeric_limits<uint16_t>::max()) return std::numeric_limits<uint16_t>::max();
    return static_cast<uint16_t>(value);
}

// Sets up parameters for generateInstructionsRecursive()
// Initiates instruction generation
void Process::generateRandomInstructions(int min_ins, int max_ins) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ins_count_dist(min_ins, max_ins);
    std::uniform_int_distribution<> ins_type_dist(0, 5);
    std::uniform_int_distribution<> value_dist(0, std::numeric_limits<uint16_t>::max());
    std::uniform_int_distribution<> sleep_dist(0, 255);
    std::uniform_int_distribution<> for_repeat_dist(1, 3);
    std::uniform_int_distribution<> for_inner_count_dist(1, 3);
    
    int instruction_count = ins_count_dist(gen);
    generateInstructionsRecursive(instruction_count, gen, ins_type_dist, value_dist, sleep_dist, for_repeat_dist, for_inner_count_dist, 0);
}

// Assigns instructions for the processes (PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR)
// Handles for loops and processes within it
void Process::generateInstructionsRecursive(int instruction_count, std::mt19937& gen, 
    std::uniform_int_distribution<>& ins_type_dist,
    std::uniform_int_distribution<>& value_dist,
    std::uniform_int_distribution<>& sleep_dist,
    std::uniform_int_distribution<>& for_repeat_dist,
    std::uniform_int_distribution<>& for_inner_count_dist,
    int nesting_level) {
    
    int current_count = 0;
    
    while (current_count < instruction_count) {
        Instruction inst;
        int type = ins_type_dist(gen);
        int remaining_budget = instruction_count - current_count;
        // Basic Process Instructions
        switch (type) {
            case 0: // PRINT
                inst.type = InstructionType::PRINT;
                inst.args.push_back("Hello world from " + name + "!");
                instructions.push_back(inst);
                current_count++;
                break;
            case 1: // DECLARE
                inst.type = InstructionType::DECLARE;
                inst.args.push_back("var" + std::to_string(instructions.size()));
                inst.args.push_back(std::to_string(value_dist(gen)));
                instructions.push_back(inst);
                current_count++;
                break;
            case 2: // ADD
                inst.type = InstructionType::ADD;
                inst.args.push_back("result" + std::to_string(instructions.size()));
                inst.args.push_back(std::to_string(value_dist(gen)));
                inst.args.push_back(std::to_string(value_dist(gen)));
                instructions.push_back(inst);
                current_count++;
                break;
            case 3: // SUBTRACT
                inst.type = InstructionType::SUBTRACT;
                inst.args.push_back("result" + std::to_string(instructions.size()));
                inst.args.push_back(std::to_string(value_dist(gen)));
                inst.args.push_back(std::to_string(value_dist(gen)));
                instructions.push_back(inst);
                current_count++;
                break;
            case 4: // SLEEP
                inst.type = InstructionType::SLEEP;
                inst.args.push_back(std::to_string(sleep_dist(gen)));
                instructions.push_back(inst);
                current_count++;
                break;
            case 5: // FOR
                {
                    // Need at least 3 instructions for a minimal FOR loop (FOR_START + 1 inner + FOR_END)
                    if (remaining_budget < 3) {
                        // Not enough budget for FOR loop, generate a simple instruction instead
                        inst.type = InstructionType::PRINT;
                        inst.args.push_back("Hello world from " + name + "!");
                        instructions.push_back(inst);
                        current_count++;
                        break;
                    }
                    
                    // Add FOR_START (start of loop)
                    inst.type = InstructionType::FOR_START;
                    inst.for_repeats = for_repeat_dist(gen);
                    instructions.push_back(inst);
                    current_count++;
                    
                    // Calculate budget for inner instructions (so that instructions don't go beyond max)
                    int max_inner_budget = remaining_budget - 1 - 1; // reserved 1 for FOR_END and 1 for FOR_START
                    int desired_inner_count = for_inner_count_dist(gen);
                    int actual_inner_count = std::min(desired_inner_count, max_inner_budget);
                    actual_inner_count = std::max(1, actual_inner_count); // At least 1 inner instruction
                    
                    // Generate inner instructions recursively
                    int instructions_before_inner = instructions.size();
                    generateInstructionsRecursive(actual_inner_count, gen, ins_type_dist, value_dist, 
                        sleep_dist, for_repeat_dist, for_inner_count_dist, nesting_level + 1);
                    
                    // Count how many instructions were actually added
                    int actual_inner_added = instructions.size() - instructions_before_inner;
                    current_count += actual_inner_added;
                    
                    // Add FOR_END (end of loop)
                    Instruction end_inst;
                    end_inst.type = InstructionType::FOR_END;
                    instructions.push_back(end_inst);
                    current_count++;
                }
                break;
            default:
                inst.type = InstructionType::PRINT;
                inst.args.push_back("Hello world from " + name + "!");
                instructions.push_back(inst);
                current_count++;
                break;
        }
    }
}

// Executes a single instruction from a process during tick
bool Process::executeNextInstruction(int delays_per_exec) {
     if (sleep_ticks_remaining > 0) {
         state = ProcessState::WAITING;
         return true;
     }
     
     if (current_instruction >= instructions.size()) {
        state = ProcessState::FINISHED;
        finish_time = std::chrono::system_clock::now();
        return false; // Process finished
    }

    executeInstruction(instructions[current_instruction]);
    total_instructions_executed++;

    if (delays_per_exec > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delays_per_exec));
    }
    
    current_instruction++;

    return true;
}

// Specific instruction execution logic
// Handles PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR_START, FOR
void Process::executeInstruction(const Instruction& inst) {
    switch (inst.type) {
        case InstructionType::PRINT: {
            std::string output = inst.args[0];
            addOutput(output);
            break;
        }
        case InstructionType::DECLARE: {
            if (inst.args.size() >= 2) {
                std::string var_name = inst.args[0];
                uint16_t value = static_cast<uint16_t>(std::stoi(inst.args[1]));
                variables[var_name] = clamp(value);
            }
            break;
        }
        case InstructionType::ADD: {
            if (inst.args.size() >= 3) {
                std::string result_var = inst.args[0];
                uint16_t val1 = std::stoi(evaluateExpression(inst.args[1]));
                uint16_t val2 = std::stoi(evaluateExpression(inst.args[2]));
                uint64_t result = val1 + val2;
                variables[result_var] = clamp(result);
            }
            break;
        }
        case InstructionType::SUBTRACT: {
            if (inst.args.size() >= 3) {
                std::string result_var = inst.args[0];
                uint16_t val1 = std::stoi(evaluateExpression(inst.args[1]));
                uint16_t val2 = std::stoi(evaluateExpression(inst.args[2]));
                uint64_t result = val1 - val2;
                variables[result_var] = clamp(result);
            }
            break;
        }
        case InstructionType::SLEEP: {
            if (!inst.args.empty()) {
                sleep_ticks_remaining = std::stoi(inst.args[0]);
                state = ProcessState::WAITING;
            }
            break;
        }
        case InstructionType::FOR_START: {
            // Push current position onto for stack
            if (for_stack_size < 3) {
                for_stack[for_stack_size] = current_instruction; // Store start position
                for_current_repeat[for_stack_size] = 1; // Start with iteration 1
                for_stack_size++;
            }
            break;
        }
        case InstructionType::FOR_END: {
            if (for_stack_size > 0) {
                int current_level = for_stack_size - 1; // Top of stack
                int for_start_index = for_stack[current_level];
                
                // Get the FOR_START instruction to check repeat count
                const Instruction& for_start_inst = instructions[for_start_index];
                
                // Check if we need more iterations
                if (for_current_repeat[current_level] < for_start_inst.for_repeats) {
                    for_current_repeat[current_level]++;
                    current_instruction = for_start_index; // Jump back to start
                } else {
                    for_stack_size--;
                }
            }
            break;
        }
    }
}

std::string Process::evaluateExpression(const std::string& expr) {
    // Check if it's a variable
    if (variables.find(expr) != variables.end()) {
        return std::to_string(variables[expr]);
    }
    return expr;
}

void Process::addOutput(const std::string& output) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_ptr = std::localtime(&time_t);
    
    std::ostringstream timestamp;
    timestamp << std::put_time(tm_ptr, "%m/%d/%Y %I:%M:%S%p");
    
    // Format: (timestamp) Core:X - original_output
    std::string formatted_output = "(" + timestamp.str() + ") Core:" + 
                                  std::to_string(cpu_core_assigned) + " " + output;
    
    output_logs.push_back(formatted_output);
}