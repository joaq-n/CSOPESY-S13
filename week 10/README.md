configure config.txt:
num-cpu (number)
scheduler (fcfs/rr)
quantum-cycles (number)
batch-process-freq (number)
min-ins (number)
max-ins (number)
delays-per-exec (number)

g++ -std=c++11 -pthread main.cpp process.cpp scheduler.cpp -o main