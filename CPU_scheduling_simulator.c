#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define max_Process_count 20
#define min_pid 1000
#define max_pid 3000
#define max_k 3 // max_burst_interval_size is equal to 2 * max_k + 1
#define max_arrival 500
#define CPU_min 1
#define CPU_max 10
#define long_CPU_min 90
#define long_CPU_max 150
#define IO_min 50
#define IO_max 100
#define max_priority 20

typedef struct{
    int pid;
    int arrival_time;
    int burst_interval[2*max_k + 1];
    int burst_interval_size;
    int priority;
} Process_info;

int pid_used[max_pid - min_pid + 1] = {0,};
int PCB_size;
Process_info** PCB;

int p_info_compare(const void *a, const void *b) {
    Process_info* p_a = *(Process_info**) a;
    Process_info* p_b = *(Process_info**) b;

    return p_a -> arrival_time - p_b -> arrival_time;
}

void process_random_generator(){
    int N = (rand() % max_Process_count) + 1;
    PCB_size = N;

    PCB = (Process_info**) malloc(sizeof(Process_info*) * N);
    if (PCB == NULL) {
        fprintf(stderr, "Memory Allocation failed \n");
        return;
    }
    for(int i = 0; i < N; i++){
        PCB[i] = (Process_info*) malloc(sizeof(Process_info));
        if (PCB[i] == NULL) {
            fprintf(stderr, "Memory Allocation failed \n");
            return;
        }

        int pid_idx;
        do {
            pid_idx = rand() % (max_pid - min_pid + 1);
        } while(pid_used[pid_idx] == 1);

        pid_used[pid_idx] = 1;
        PCB[i] -> pid = pid_idx + min_pid;
        PCB[i] -> arrival_time = rand() % (max_arrival + 1);
        int k = rand() % (max_k + 1);

        for(int j = 0; j < 2*k + 1; j++){
            if (j % 2 == 0) {
                if ((rand() % 10) < 8) {
                    PCB[i] -> burst_interval[j] = rand() % (CPU_max - CPU_min + 1) + CPU_min;
                }
                else {
                    PCB[i] -> burst_interval[j] = rand() % (long_CPU_max - long_CPU_min + 1) + long_CPU_min;
                }
            }
            else {
                PCB[i] -> burst_interval[j] = rand() % (IO_max - IO_min + 1) + IO_min;
            }
        }
        PCB[i] -> burst_interval_size = 2*k + 1;
        PCB[i] -> priority = rand() % max_priority;
    }
    qsort(PCB, N, sizeof(Process_info*), p_info_compare);

    for (int i = 0; i < N; i++) {
        printf("%d %d\n", PCB[i] -> pid, PCB[i] -> arrival_time);
    }
}

void cleanup() {
    for (int i = 0; i < PCB_size; i++) {
        free(PCB[i]);
    }
    free(PCB);
}

int main() {

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((unsigned int)ts.tv_nsec);

    process_random_generator();
    cleanup();
    return 0;
}