configure config.txt:
num-cpu (number)
scheduler (fcfs/rr)
quantum-cycles (number)
batch-process-freq (number)
min-ins (number)
max-ins (number)
delays-per-exec (number)
max-overall-mem (number)
mem-per-frame (number)
mem-per-proc (number)

Run the following:
1. g++ -std=c++14 -pthread main.cpp process.cpp scheduler.cpp -o main
2. main
