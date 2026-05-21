#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"
#include "color.h"
#include "random_distribution.h"

int pid_candidate[pid_max - pid_min + 1] = {0,};
int PCB_size = 0;
Process_info** PCB = NULL;
ProcConfig configs[] = {
    [CPU_bound] = {15, 20, 40, 0, 1},
    [IO_bound] = {5, 40, 60, 3, 4},
};
void pid_shuffle() {
    for(int i = 0; i <= pid_max - pid_min; i++) {
        pid_candidate[i] = pid_min + i;
    }

    for(int i = pid_max - pid_min; i > 0; i--) {
        int j = uniform_dist(0, i);
        
        int temp = pid_candidate[j];
        pid_candidate[j] = pid_candidate[i];
        pid_candidate[i] = temp;
    }
}
void PCB_cleanup() {
    if (PCB != NULL) {
        for (int i = 0; i < PCB_size; i++) {
            if (PCB[i] != NULL) {
                free(PCB[i]);
            }
        }
        free(PCB);
    }
}

int p_info_compare(const void *a, const void *b) {
    Process_info* p_a = *(Process_info**) a;
    Process_info* p_b = *(Process_info**) b;

    return p_a -> arrival_time - p_b -> arrival_time;
}

void process_random_generator(){
    int N = uniform_dist(min_Process_count, max_Process_count);
    pid_shuffle();
    PCB_size = N;

    PCB = (Process_info**) calloc(N, sizeof(Process_info*));
    if (PCB == NULL) {
        fprintf(stderr, "Memory Allocation failed \n");
        return;
    }
    for(int i = 0; i < N; i++){
        PCB[i] = (Process_info*) malloc(sizeof(Process_info));
        if (PCB[i] == NULL) {
            fprintf(stderr, "Memory Allocation failed \n");
            PCB_cleanup();
            return;
        }

        PCB[i] -> pid = pid_candidate[i];
        PCB[i] -> arrival_time = uniform_dist(min_arrival, max_arrival);
        PCB[i] -> priority = uniform_dist(0, N - 1);
        Process_type type = uniform_dist(1, 100) <= portion_CPU_bound ? CPU_bound : IO_bound;
        PCB[i] -> type = type;
        ProcConfig* config = &configs[type];

        PCB[i] -> burst_interval_size = 2 * uniform_dist(config -> cycle_min, config -> cycle_max <= io_cycle_max ? config-> cycle_max : io_cycle_max) + 1;
        for(int j = 0; j < PCB[i] -> burst_interval_size; j++){
            if (j % 2 == 0) {
                PCB[i] -> burst_interval[j] = geometric_dist(config -> cpu_mean);
            }
            else { 
                PCB[i] -> burst_interval[j] = uniform_dist(config -> io_min, config -> io_max);
            }
        }
    }

    qsort(PCB, N, sizeof(Process_info*), p_info_compare);

    printf(ANSI_GREEN "Process Information was generated successfully!" ANSI_RESET "\n");
    printf(ANSI_YELLOW "Number of Processes: " ANSI_RESET "%d\n", N);
    printf(ANSI_BOLD "%-6s | %-12s | %-10s | %-12s | %s " ANSI_RESET, "PID", "Arrival Time", "Prioriy", "Type", "Burst Time");
    printf(
        ANSI_BOLD "(" 
        ANSI_BOLD_BLUE "CPU" 
        ANSI_BOLD_WHITE ", " 
        ANSI_BOLD_RED "I/O" 
        ANSI_BOLD_WHITE ")"
        ANSI_RESET "\n"
    );
    printf(ANSI_BOLD_WHITE "--------------------------------------------------------------------" ANSI_RESET "\n");
    for (int i = 0; i < N; i++) {
        printf(ANSI_BOLD "%-6d | %-12d | %-10d | %-12s |" ANSI_RESET, PCB[i] -> pid, PCB[i] -> arrival_time, PCB[i] -> priority, PCB[i] -> type == CPU_bound ? "CPU bound" : "IO bound");
        int last_busrt_index = PCB[i] -> burst_interval_size - 1;
        for (int j = 0; j < PCB[i] -> burst_interval_size / 2; j++) {
            printf(ANSI_BOLD_BLUE " %d " ANSI_RESET, PCB[i] -> burst_interval[2*j]);
            printf(ANSI_BOLD "->" ANSI_RESET);
            printf(ANSI_BOLD_RED " %d " ANSI_RESET, PCB[i] -> burst_interval[2*j + 1]);
            printf(ANSI_BOLD "->" ANSI_RESET);
        }
        printf(ANSI_BOLD_BLUE " %d" ANSI_RESET "\n", PCB[i] -> burst_interval[last_busrt_index]);
    }
}