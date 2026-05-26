#ifndef CONFIG_H
#define CONFIG_H


// Linux Key Codes for VT100 sequences
#define KEY_UP    65
#define KEY_DOWN  66
#define KEY_RIGHT 67
#define KEY_LEFT  68
#define KEY_ENTER 10

#define MAX_VAL(a, b) ((a) > (b) ? (a) : (b))
#define MAX_PROCESS_COUNT 20 // Max number of processes, used for array sizing
#define PID_MIN 1000
#define PID_MAX 2000
#define MAX_PRIORITY 20
#define IO_CYCLE_MAX 5 // Max number of IO cycles, used for burst_interval array sizing

#define MAX_TIME_LINE 10000

#define MAX_CPU_COUNT 5
#define MAX_TIER_COUNT 5
#define MAX_RQ_COUNT MAX_VAL(MAX_CPU_COUNT, MAX_TIER_COUNT)
#define MAX_IO_WQ_COUNT 5

typedef struct Burst{
    int type;
    int time;
} Burst;

typedef enum Process_type {CPU_BOUND, IO_BOUND} Process_type;

typedef struct ProcConfig{
    int cpu_min, cpu_max;
    int io_min, io_max;
    int cycle_min, cycle_max;
} ProcConfig;

typedef struct Process_info{
    int pid;
    int arrival_time;
    Burst burst_interval[2*IO_CYCLE_MAX + 1];
    int burst_interval_size;
    int priority;
    Process_type type;
} Process_info;

typedef enum State_name { NOT_GENERATED, CPU_RUNNING, IO_RUNNING, READY, WAITING, TERMINATED } State_name;
typedef enum Resource_type { NONE, CPU, IO, READY_QUEUE, WAITING_QUEUE } Resource_type;

typedef struct State{
    State_name name;
    Resource_type resource_type;
    int resource_idx;

    int current_arrival_time;
    int current_burst_time;
    int current_burst_idx;
    int current_priority;
} State;

typedef struct Process {
    Process_info* p_info_ptr;
    State current_state;
    int from_cpu;

    State chart[MAX_TIME_LINE];
    int chart_length;
} Process;

typedef struct Resource{
    Process* items[MAX_PROCESS_COUNT];
    
    int current_process_count;
} Resource;

typedef struct Ready_Queue{
    Resource* base;
    int chart[MAX_TIME_LINE][MAX_PROCESS_COUNT];
    int process_length_at_time[MAX_TIME_LINE];
    int chart_length;

    int io_interrupted;
    int preemptive;
    int time_quantum;
    int aging_interval;
    int (*compare)(const void *a, const void *b);
}Ready_Queue;

typedef struct Waiting_Queue{
    Resource* base;
    int chart[MAX_TIME_LINE][MAX_PROCESS_COUNT];
    int process_length_at_time[MAX_TIME_LINE];
    int chart_length;
} Waiting_Queue;

typedef struct CPU_resource{
    Resource* base;

    int chart[MAX_TIME_LINE];
    int chart_length;

    int remain_time_quantum;
}CPU_resource;

typedef struct IO_resource{
    Resource* base;

    int chart[MAX_TIME_LINE];
    int chart_length;
}IO_resource;

typedef enum rq_situation { SCP_SINGLE_QUEUE, SCP_MLQ, SCP_MLFQ, MCP_COMMON_QUEUE, MCP_OWN_QUEUE } rq_situation;
typedef enum algorithm {
    NONE_ALG, 
    FCFS, 
    SJF, 
    PRIORITY, 
    RR, 
    PREEMPTIVE_SJF, PREEMPTIVE_PRIORITY, 
    PRIORITY_AGING, PREEMPTIVE_PRIORITY_AGING
} algorithm;

// User-configurable process generation parameters
extern int user_min_process_count;
extern int user_max_process_count;
extern int user_max_priority;
extern int user_min_arrival;
extern int user_max_arrival;

extern int user_cpu_bound_cpu_min, user_cpu_bound_cpu_max;
extern int user_cpu_bound_io_min, user_cpu_bound_io_max;
extern int user_cpu_bound_io_cycle_min, user_cpu_bound_io_cycle_max;

extern int user_io_bound_cpu_min, user_io_bound_cpu_max;
extern int user_io_bound_io_min, user_io_bound_io_max;
extern int user_io_bound_io_cycle_min, user_io_bound_io_cycle_max;

extern int user_portion_of_cpu_bound;

// Global ProcConfig array, initialized dynamically in process_random_generator
extern ProcConfig configs[2];

extern int pid_candidate[PID_MAX - PID_MIN + 1];
extern int PCB_size;
extern Process_info* PCB[MAX_PROCESS_COUNT];

extern Process* proc[MAX_PROCESS_COUNT];

extern CPU_resource* cpu[MAX_CPU_COUNT];
extern IO_resource* io[MAX_IO_WQ_COUNT];
extern Ready_Queue* rq[MAX_RQ_COUNT];
extern Waiting_Queue* wq[MAX_IO_WQ_COUNT];

extern int cpu_count;
extern int rq_count;
extern int io_wq_count;
extern int priority_interval_per_tier;
extern int aging_interval;


void process_random_generator();

void cleanup();
void PCB_cleanup();
void proc_cleanup();
void cpu_cleanup();
void io_cleanup();
void rq_cleanup();
void wq_cleanup();

void PCB_allocation();
void proc_allocation();
void cpu_allocation();
void io_allocation();
void rq_allocation();
void wq_allocation();

void pid_shuffle();
void config(rq_situation rq_s);
void set_rq_params(int index, int alg_choice, int preemptive, int tq, int aging);
void get_process_generation_params();
int select_menu(char* title, char** options, int count);
void set_rq_params_interactive(int rq_idx);
void display_pcb_table();
int getch_linux(void);

#endif