#ifndef PCB_H
#define PCB_H

#define min_Process_count 5
#define max_Process_count 20
#define pid_min 1000
#define pid_max 2000
#define io_cycle_max 15
#define min_arrival 0
#define max_arrival 500
#define portion_CPU_bound 60
#define max_time_line 10000

typedef enum {
    CPU_bound,
    IO_bound
} Process_type;

typedef struct{
    int cpu_mean;
    int io_min, io_max;
    int cycle_min, cycle_max;
} ProcConfig;

typedef struct{
    int pid;
    int arrival_time;
    int burst_interval[2*io_cycle_max + 1];
    int burst_interval_size;
    int priority;
    Process_type type;
} Process_info;

typedef enum { cpu_running, io_running, ready, waiting } State_name;

typedef struct {
    State_name name;
    int resource_idx;
} State;
typedef struct{
    Process_info* p_info_ptr;
    int current_burst;
    int burst_idx;

    State state_timeline;
} Process;

extern ProcConfig configs[];
extern int pid_candidate[pid_max - pid_min + 1];
extern int PCB_size;
extern Process_info** PCB;

void process_random_generator();
void PCB_cleanup();
void pid_shuffle();

#endif