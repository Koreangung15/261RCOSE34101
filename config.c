#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "color.h"
#include "random_distribution.h"
#include "mem_alloc.h"
#include "cmp_func.h"

int pid_candidate[PID_MAX - PID_MIN + 1] = {0};
int PCB_size = 0;
Process_info* PCB[MAX_PROCESS_COUNT] = {NULL};
ProcConfig configs[] = {
    [CPU_BOUND] = {CPU_BOUND_CPU_MIN, CPU_BOUND_CPU_MAX, CPU_BOUND_IO_MIN, CPU_BOUND_IO_MAX, CPU_BOUND_IO_CYCLE_MIN, CPU_BOUND_IO_CYCLE_MAX},
    [IO_BOUND] = {IO_BOUND_CPU_MIN, IO_BOUND_CPU_MAX, IO_BOUND_IO_MIN, IO_BOUND_IO_MAX, IO_BOUND_IO_CYCLE_MIN, IO_BOUND_IO_CYCLE_MAX}
};

Process* proc[MAX_PROCESS_COUNT] = {NULL};
CPU_resource* cpu[MAX_CPU_COUNT] = {NULL};
IO_resource* io[MAX_IO_WQ_COUNT] = {NULL};
Ready_Queue* rq[MAX_RQ_COUNT] = {NULL};
Waiting_Queue* wq[MAX_IO_WQ_COUNT] = {NULL};

int cpu_count;
int rq_count;
int io_wq_count;
int priority_interval_per_tier;

void pid_shuffle() {
    for(int i = 0; i <= PID_MAX - PID_MIN; i++) {
        pid_candidate[i] = PID_MIN + i;
    }

    for(int i = PID_MAX - PID_MIN; i > 0; i--) {
        int j = uniform_dist(0, i);
        
        int temp = pid_candidate[j];
        pid_candidate[j] = pid_candidate[i];
        pid_candidate[i] = temp;
    }
}


void process_random_generator(){
    int N = uniform_dist(MIN_PROCESS_COUNT, MAX_PROCESS_COUNT);
    pid_shuffle();
    PCB_size = N;

    PCB_allocation();

    for(int i = 0; i < N; i++){
        PCB[i] -> pid = pid_candidate[i];
        PCB[i] -> arrival_time = uniform_dist(MIN_ARRIVAL, MAX_ARRIVAL);
        PCB[i] -> priority = uniform_dist(0, MAX_PRIORITY - 1);
        Process_type type = uniform_dist(1, 100) <= PORTION_OF_CPU_BOUND ? CPU_BOUND : IO_BOUND;
        PCB[i] -> type = type;
        ProcConfig* config = &configs[type];
        int cycle_min = config -> cycle_min;
        int cycle_max = config -> cycle_max;
        PCB[i] -> burst_interval_size = 2 * uniform_dist(cycle_min, cycle_max) + 1;
        for(int j = 0; j < PCB[i] -> burst_interval_size; j++){
            if (j % 2 == 0) {
                PCB[i] -> burst_interval[j] = (Burst){ 0, uniform_dist(config -> cpu_min, config -> cpu_max) };
            }
            else { 
                PCB[i] -> burst_interval[j] = (Burst){ uniform_dist(1, MAX_IO_WQ_COUNT), uniform_dist(config -> io_min, config -> io_max) };
            }
        }
    }

    qsort(PCB, N, sizeof(Process_info*), p_info_compare);

    printf(ANSI_GREEN "Process Information was generated successfully!" ANSI_RESET "\n");
    printf(ANSI_YELLOW "Number of Processes: " ANSI_RESET "%d\n", N);
    printf(ANSI_BOLD "%-6s | %-12s | %-10s | %-12s | %s " ANSI_RESET, "PID", "Arrival Time", "Prioriy", "Type", "Burst Time");
    printf(ANSI_BOLD "(" ANSI_RESET);
    printf(ANSI_BOLD_BLUE " CPU" ANSI_RESET);
    printf(ANSI_BOLD ", " ANSI_RESET);
    printf(ANSI_BOLD_RED "IO(io_type) " ANSI_RESET);
    printf(ANSI_BOLD ")" ANSI_RESET "\n");
    printf(ANSI_BOLD_WHITE "------------------------------------------------------------------------------------------------------" ANSI_RESET "\n");
    for (int i = 0; i < N; i++) {
        printf(ANSI_BOLD "%-6d | %-12d | %-10d | %-12s | " ANSI_RESET, PCB[i] -> pid, PCB[i] -> arrival_time, PCB[i] -> priority, PCB[i] -> type == CPU_BOUND ? "CPU bound" : "IO bound");
        for (int j = 0; j < PCB[i] -> burst_interval_size; j++){
            switch (PCB[i] -> burst_interval[j].type){
                case 0:
                    printf(ANSI_BOLD_BLUE "%d" ANSI_RESET, PCB[i] -> burst_interval[j].time);
                    break;
                default:
                    printf(ANSI_BOLD_RED "%d(%d)" ANSI_RESET, PCB[i] -> burst_interval[j].time, PCB[i] -> burst_interval[j].type - 1);
                    break;
            }
            if (j == PCB[i] -> burst_interval_size - 1) {
                printf("\n");
            }
            else{
                printf(ANSI_BOLD " -> " ANSI_RESET);
            }
        }
    }
}


void config(rq_situation rq_s, algorithm alg){

    cpu_count = (rq_s == MCP_COMMON_QUEUE || rq_s == MCP_OWN_QUEUE) ? MAX_CPU_COUNT : 1;
    rq_count = (rq_s == SCP_SINGLE_QUEUE || rq_s == MCP_COMMON_QUEUE) ? 1 
    : ((rq_s == SCP_MLQ || rq_s == SCP_MLFQ) ? MAX_TIER_COUNT 
    : MAX_CPU_COUNT);
    io_wq_count = MAX_IO_WQ_COUNT;
    priority_interval_per_tier = (MAX_PRIORITY + MAX_TIER_COUNT - 1) / MAX_TIER_COUNT; // which uses for MLQ and MLFQ

    for(int i = 0; i < PCB_size; i++){
        proc[i] -> p_info_ptr = PCB[i];
        proc[i] -> from_cpu = 0;
        proc[i] -> current_state.name = NOT_GENERATED;
        proc[i] -> current_state.resource_type = NONE;
        proc[i] -> current_state.resource_idx = 0;
        proc[i] -> current_state.current_burst_time = PCB[i] -> burst_interval[0].time;
        proc[i] -> current_state.current_arrival_time = PCB[i] -> arrival_time;
        proc[i] -> current_state.current_priority = PCB[i] -> priority;
        proc[i] -> current_state.current_burst_idx = 0;
        proc[i] -> chart_length = 0;
    }

    for(int i = 0; i < MAX_CPU_COUNT; i++){
        cpu[i] -> base -> current_process_count = 0;
        cpu[i] -> chart_length = 0;
        cpu[i] -> remain_time_quantum = 0;
    }

    for(int i = 0; i < MAX_IO_WQ_COUNT; i++){
        io[i] -> base -> current_process_count = 0;
        io[i] -> chart_length = 0;
    }

    for(int i = 0; i < MAX_IO_WQ_COUNT; i++){
        wq[i] -> base -> current_process_count = 0;
        wq[i] -> chart_length = 0;
        for(int j = 0; j < MAX_TIME_LINE; j++){
            wq[i] -> process_length_at_time[j] = 0;
        }
    }

    for(int i = 0; i < MAX_RQ_COUNT; i++){
        rq[i] -> base -> current_process_count = 0;
        rq[i] -> chart_length = 0;
        rq[i] -> compare = NULL;
        for(int j = 0; j < MAX_TIME_LINE; j++){
            rq[i] -> process_length_at_time[j] = 0;
        }
    }
    if (rq_s == SCP_MLQ || rq_s == SCP_MLFQ){
        if (rq_count >= 3){
            int t_q = 2;
            for(int i = 0; i < rq_count / 3; i++){
                rq[i] -> compare = rr_fcfs_cmp;
                rq[i] -> preemptive = 1;
                rq[i] -> time_quantum = t_q > 16 ? 16 : t_q;
                rq[i] -> aging_interval = rq_s == SCP_MLQ ? 0 : AGING_INTERVAL;
                t_q *= 2;
            }
            for(int i = rq_count / 3; i < 2 * rq_count / 3; i++){
                rq[i] -> compare = rr_sjf_cmp;
                rq[i] -> preemptive = 1;
                rq[i] -> time_quantum = t_q > 16 ? 16 : t_q;
                rq[i] -> aging_interval = rq_s == SCP_MLQ ? 0 : AGING_INTERVAL;
                t_q *= 2;
            }
            for(int i = 2 * rq_count / 3; i < rq_count; i++){
                rq[i] -> compare = fcfs_cmp;
                rq[i] -> preemptive = 0;
                rq[i] -> time_quantum = 0;
                rq[i] -> aging_interval = rq_s == SCP_MLQ ? 0 : AGING_INTERVAL;
            }
        }
        else{
            if (rq_count == 1){
                rq[0] -> compare = rr_fcfs_cmp;
                rq[0] -> preemptive = 1;
                rq[0] -> time_quantum = 2;
                rq[0] -> aging_interval = rq_s == SCP_MLQ ? 0 : AGING_INTERVAL;
            }
            else{
                rq[0] -> compare = rr_fcfs_cmp;
                rq[0] -> preemptive = 1;
                rq[0] -> time_quantum = 2;
                rq[0] -> aging_interval = rq_s == SCP_MLQ ? 0 : AGING_INTERVAL;

                rq[1] -> compare = rr_fcfs_cmp;
                rq[1] -> preemptive = 1;
                rq[1] -> time_quantum = 4;
                rq[1] -> aging_interval = rq_s == SCP_MLQ ? 0 : AGING_INTERVAL;
            }
        }
    }
    else{
        int (*cmp_func)(const void *, const void *) = NULL;
        
        if (alg == FCFS) {
            cmp_func = fcfs_cmp;
        }
        else if (alg == SJF || alg == PREEMPTIVE_SJF){
            cmp_func = sjf_cmp;
        }
        else if (alg == RR) {
            cmp_func = rr_fcfs_cmp;
        }
        else if (alg == PRIORITY || alg == PREEMPTIVE_PRIORITY || alg == PRIORITY_AGING || alg == PREEMPTIVE_PRIORITY_AGING){
            cmp_func = priority_cmp;
        }

        int preemptive = (
            alg == PREEMPTIVE_SJF 
            || alg == PREEMPTIVE_PRIORITY 
            || alg == PREEMPTIVE_PRIORITY_AGING
        ) ? 1 : 0;

        int time_quantum = alg == RR ? TIME_QUANTUM : 0;

        int aging_interval = (alg == PRIORITY_AGING || alg == PREEMPTIVE_PRIORITY_AGING) ? AGING_INTERVAL : 0;
        for (int i = 0; i < rq_count; i++){
            rq[i] -> compare = cmp_func;
            rq[i] -> preemptive = preemptive;
            rq[i] -> time_quantum = time_quantum;
            rq[i] -> aging_interval = aging_interval;
        }
    }

    
}