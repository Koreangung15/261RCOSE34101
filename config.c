#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>

#include "config.h"
#include "color.h"
#include "random_distribution.h"
#include "mem_alloc.h"
#include "cmp_func.h"

int pid_candidate[PID_MAX - PID_MIN + 1] = {0};
int PCB_size = 0;
Process_info* PCB[MAX_PROCESS_COUNT] = {NULL}; // MAX_PROCESS_COUNT is now a fixed macro for array sizing

// User-configurable process generation parameters
int user_min_process_count;
int user_max_process_count;
int user_max_priority;
int user_min_arrival;
int user_max_arrival;

int user_cpu_bound_cpu_min, user_cpu_bound_cpu_max;
int user_cpu_bound_io_min, user_cpu_bound_io_max;
int user_cpu_bound_io_cycle_min, user_cpu_bound_io_cycle_max;

int user_io_bound_cpu_min, user_io_bound_cpu_max;
int user_io_bound_io_min, user_io_bound_io_max;
int user_io_bound_io_cycle_min, user_io_bound_io_cycle_max;

int user_portion_of_cpu_bound;

ProcConfig configs[2] = { // Initialized dynamically in process_random_generator
    {0,0,0,0,0,0}, {0,0,0,0,0,0}
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
    // Initialize ProcConfig based on user input
    configs[CPU_BOUND].cpu_min = user_cpu_bound_cpu_min;
    configs[CPU_BOUND].cpu_max = user_cpu_bound_cpu_max;
    configs[CPU_BOUND].io_min = user_cpu_bound_io_min;
    configs[CPU_BOUND].io_max = user_cpu_bound_io_max;
    configs[CPU_BOUND].cycle_min = user_cpu_bound_io_cycle_min;
    configs[CPU_BOUND].cycle_max = user_cpu_bound_io_cycle_max;

    configs[IO_BOUND].cpu_min = user_io_bound_cpu_min;
    configs[IO_BOUND].cpu_max = user_io_bound_cpu_max;
    configs[IO_BOUND].io_min = user_io_bound_io_min;
    configs[IO_BOUND].io_max = user_io_bound_io_max;
    configs[IO_BOUND].cycle_min = user_io_bound_io_cycle_min;
    configs[IO_BOUND].cycle_max = user_io_bound_io_cycle_max;

    int N = uniform_dist(user_min_process_count, user_max_process_count);
    pid_shuffle();
    PCB_size = N;

    if (N > MAX_PROCESS_COUNT) {
        fprintf(stderr, "Error: Number of processes (%d) exceeds MAX_PROCESS_COUNT (%d).\n", N, MAX_PROCESS_COUNT);
        exit(1);
    }

    PCB_allocation();

    for(int i = 0; i < N; i++){
        PCB[i] -> pid = pid_candidate[i]; // PIDs are shuffled from PID_MIN to PID_MAX
        PCB[i] -> arrival_time = uniform_dist(user_min_arrival, user_max_arrival);
        PCB[i] -> priority = uniform_dist(0, user_max_priority - 1);
        Process_type type = uniform_dist(1, 100) <= user_portion_of_cpu_bound ? CPU_BOUND : IO_BOUND;
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

    display_pcb_table();
    printf(ANSI_BOLD_GREEN "\n Process Information was generated successfully! Press any key to continue..." ANSI_RESET "\n");
    getch_linux();
}

void display_pcb_table() {
    if (PCB_size == 0) return;
    printf(ANSI_BOLD_CYAN "=== Generated Process Table (N=%d) ===" ANSI_RESET "\n", PCB_size);
    printf(ANSI_BOLD "%-6s | %-12s | %-10s | %-12s | %s " ANSI_RESET, "PID", "Arrival", "Priority", "Type", "Burst Intervals ");
    printf(ANSI_BOLD "(" ANSI_RESET);
    printf(ANSI_BOLD_BLUE " CPU" ANSI_RESET);
    printf(ANSI_BOLD ", " ANSI_RESET);
    printf(ANSI_BOLD_RED "IO(io_type) " ANSI_RESET);
    printf(ANSI_BOLD ")" ANSI_RESET "\n");
    printf(ANSI_BOLD_WHITE "------------------------------------------------------------------------------------------------------" ANSI_RESET "\n");
    for (int i = 0; i < PCB_size; i++) {
        printf("%-6d | %-12d | %-10d | %-12s | ", PCB[i]->pid, PCB[i]->arrival_time, PCB[i]->priority, PCB[i]->type == CPU_BOUND ? "CPU-bound" : "IO-bound");
        for (int j = 0; j < PCB[i]->burst_interval_size; j++){
            switch (PCB[i]->burst_interval[j].type){
                case 0:
                    printf(ANSI_BOLD_BLUE "%d" ANSI_RESET, PCB[i]->burst_interval[j].time);
                    break;
                default:
                    printf(ANSI_BOLD_RED "%d(%d)" ANSI_RESET, PCB[i]->burst_interval[j].time, PCB[i]->burst_interval[j].type - 1);
                    break;
            }
            if (j == PCB[i]->burst_interval_size - 1) printf("\n");
            else printf(" -> ");
        }
    }
    printf(ANSI_BOLD_WHITE "------------------------------------------------------------------------------------------------------" ANSI_RESET "\n");
}

void config(rq_situation rq_s){

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
}

void set_rq_params(int index, int alg_choice, int preemptive, int tq, int aging) {
    if (index < 0 || index >= MAX_RQ_COUNT) return;
    
    rq[index]->preemptive = preemptive;
    rq[index]->time_quantum = tq;
    rq[index]->aging_interval = aging;
    
    switch(alg_choice) {
        case 1: rq[index]->compare = fcfs_cmp; break;
        case 2: rq[index]->compare = sjf_cmp; break;
        case 3: rq[index]->compare = priority_cmp; break;
        case 4: rq[index]->compare = rr_fcfs_cmp; break;
        case 5: rq[index]->compare = rr_sjf_cmp; break;
        case 6: rq[index]->compare = rr_priority_cmp; break;
        default: rq[index]->compare = fcfs_cmp; break;
    }
}

// Helper function for Linux non-canonical input
int getch_linux(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

int select_menu(char* title, char** options, int count) {
    int cur = 0;
    printf(ANSI_HIDE_CURSOR);
    while (1) {
        printf("\033[H\033[J"); // Clear screen
        display_pcb_table();    // Show processes at the top
        printf(ANSI_BOLD_CYAN "=== %s ===" ANSI_RESET "\n", title);
        printf(" (Use Arrow Keys to move, Enter to select)\n\n");
        for (int i = 0; i < count; i++) {
            if (i == cur) printf(ANSI_BOLD_YELLOW " > %s " ANSI_RESET "\n", options[i]);
            else printf("   %s \n", options[i]);
        }

        int ch = getch_linux();
        if (ch == 27) { // Escape sequence
            if (getch_linux() == 91) {
                int arrow = getch_linux();
                if (arrow == KEY_UP) cur = (cur - 1 + count) % count;
                else if (arrow == KEY_DOWN) cur = (cur + 1) % count;
            }
        } else if (ch == KEY_ENTER) {
            printf(ANSI_SHOW_CURSOR);
            return cur;
        }
    }
}

void set_rq_params_interactive(int rq_idx) {
    int alg = 1, preemp = 0, tq = 0, aging = 0;
    int field_cur = 0;
    char* alg_names[] = {"FCFS", "SJF", "Priority", "RR(FCFS)", "RR(SJF)", "RR(Priority)"};

    printf(ANSI_HIDE_CURSOR);
    while (1) {
        printf("\033[H\033[J");
        display_pcb_table();    // Show processes at the top
        printf(ANSI_BOLD_GREEN "--- Configuration for Ready Queue [%d] ---" ANSI_RESET "\n", rq_idx);
        printf(" (Up/Down: Field, Left/Right: Change Value, Enter: Next/Confirm)\n\n");

        printf("%s Algorithm:      < %s%s%s >\n", field_cur == 0 ? " >" : "  ", 
            field_cur == 0 ? ANSI_HIGHLIGHT : "", alg_names[alg-1], ANSI_RESET);
        
        printf("%s Preemptive:     < %s%s%s >\n", field_cur == 1 ? " >" : "  ", 
            field_cur == 1 ? ANSI_HIGHLIGHT : "", preemp ? "YES" : "NO", ANSI_RESET);
        
        printf("%s Time Quantum:   < %s%d%s >\n", field_cur == 2 ? " >" : "  ", 
            field_cur == 2 ? ANSI_HIGHLIGHT : "", tq, ANSI_RESET);
        
        printf("%s Aging Interval: < %s%d%s >\n", field_cur == 3 ? " >" : "  ", 
            field_cur == 3 ? ANSI_HIGHLIGHT : "", aging, ANSI_RESET);
        
        printf("\n%s [%s DONE %s]\n", field_cur == 4 ? " >" : "  ", 
            field_cur == 4 ? ANSI_HIGHLIGHT : "", ANSI_RESET);

        int ch = getch_linux();
        if (ch == 27) {
            if (getch_linux() == 91) {
                int arrow = getch_linux();
                if (arrow == KEY_UP) field_cur = (field_cur - 1 + 5) % 5;
                else if (arrow == KEY_DOWN) field_cur = (field_cur + 1) % 5;
                else if (arrow == KEY_LEFT) {
                    if (field_cur == 0) alg = (alg - 2 + 6) % 6 + 1;
                    else if (field_cur == 1) preemp = !preemp;
                    else if (field_cur == 2) tq = tq > 0 ? tq - 1 : 0;
                    else if (field_cur == 3) aging = aging > 0 ? aging - 1 : 0;
                }
                else if (arrow == KEY_RIGHT) {
                    if (field_cur == 0) alg = (alg % 6) + 1;
                    else if (field_cur == 1) preemp = !preemp;
                    else if (field_cur == 2) tq++;
                    else if (field_cur == 3) aging++;
                }
            }
        } else if (ch == KEY_ENTER) {
            if (field_cur == 4) break;
            else field_cur = (field_cur + 1) % 5;
        }
    }
    printf(ANSI_SHOW_CURSOR);
    set_rq_params(rq_idx, alg, preemp, tq, aging);
}

void get_process_generation_params() {
    // Default initial values
    user_min_process_count = 5; user_max_process_count = 10;
    user_max_priority = 20;
    user_min_arrival = 0; user_max_arrival = 20;
    user_cpu_bound_cpu_min = 5; user_cpu_bound_cpu_max = 15;
    user_cpu_bound_io_min = 10; user_cpu_bound_io_max = 20;
    user_cpu_bound_io_cycle_min = 0; user_cpu_bound_io_cycle_max = 2;
    user_io_bound_cpu_min = 1; user_io_bound_cpu_max = 5;
    user_io_bound_io_min = 15; user_io_bound_io_max = 25;
    user_io_bound_io_cycle_min = 2; user_io_bound_io_cycle_max = 4;
    user_portion_of_cpu_bound = 60;

    int field_cur = 0;
    const int num_fields = 19; // 18 settings + 1 Done

    printf(ANSI_HIDE_CURSOR);
    while (1) {
        printf("\033[H\033[J");
        printf(ANSI_BOLD_CYAN "=== Global Process Generation Parameters ===" ANSI_RESET "\n");
        printf(" (Up/Down: Move, Left/Right: Change Value, Enter: Next/Finish)\n\n");

        printf("%s Min Process Count:    < %s%d%s >\n", field_cur == 0 ? " >" : "  ", field_cur == 0 ? ANSI_HIGHLIGHT : "", user_min_process_count, ANSI_RESET);
        printf("%s Max Process Count:    < %s%d%s >\n", field_cur == 1 ? " >" : "  ", field_cur == 1 ? ANSI_HIGHLIGHT : "", user_max_process_count, ANSI_RESET);
        printf("%s Max Priority:         < %s%d%s >\n", field_cur == 2 ? " >" : "  ", field_cur == 2 ? ANSI_HIGHLIGHT : "", user_max_priority, ANSI_RESET);
        printf("%s Min Arrival Time:     < %s%d%s >\n", field_cur == 3 ? " >" : "  ", field_cur == 3 ? ANSI_HIGHLIGHT : "", user_min_arrival, ANSI_RESET);
        printf("%s Max Arrival Time:     < %s%d%s >\n", field_cur == 4 ? " >" : "  ", field_cur == 4 ? ANSI_HIGHLIGHT : "", user_max_arrival, ANSI_RESET);
        
        printf("\n [ CPU-Bound ]\n");
        printf("%s CPU Burst Min/Max:    < %s%d%s / %s%d%s >\n", field_cur == 5 || field_cur == 6 ? " >" : "  ", 
            field_cur == 5 ? ANSI_HIGHLIGHT : "", user_cpu_bound_cpu_min, ANSI_RESET,
            field_cur == 6 ? ANSI_HIGHLIGHT : "", user_cpu_bound_cpu_max, ANSI_RESET);
        printf("%s IO Burst Min/Max:     < %s%d%s / %s%d%s >\n", field_cur == 7 || field_cur == 8 ? " >" : "  ", 
            field_cur == 7 ? ANSI_HIGHLIGHT : "", user_cpu_bound_io_min, ANSI_RESET,
            field_cur == 8 ? ANSI_HIGHLIGHT : "", user_cpu_bound_io_max, ANSI_RESET);
        printf("%s IO Cycle Min/Max:     < %s%d%s / %s%d%s >\n", field_cur == 9 || field_cur == 10 ? " >" : "  ", 
            field_cur == 9 ? ANSI_HIGHLIGHT : "", user_cpu_bound_io_cycle_min, ANSI_RESET,
            field_cur == 10 ? ANSI_HIGHLIGHT : "", user_cpu_bound_io_cycle_max, ANSI_RESET);

        printf("\n [ IO-Bound ]\n");
        printf("%s CPU Burst Min/Max:    < %s%d%s / %s%d%s >\n", field_cur == 11 || field_cur == 12 ? " >" : "  ", 
            field_cur == 11 ? ANSI_HIGHLIGHT : "", user_io_bound_cpu_min, ANSI_RESET,
            field_cur == 12 ? ANSI_HIGHLIGHT : "", user_io_bound_cpu_max, ANSI_RESET);
        printf("%s IO Burst Min/Max:     < %s%d%s / %s%d%s >\n", field_cur == 13 || field_cur == 14 ? " >" : "  ", 
            field_cur == 13 ? ANSI_HIGHLIGHT : "", user_io_bound_io_min, ANSI_RESET,
            field_cur == 14 ? ANSI_HIGHLIGHT : "", user_io_bound_io_max, ANSI_RESET);
        printf("%s IO Cycle Min/Max:     < %s%d%s / %s%d%s >\n", field_cur == 15 || field_cur == 16 ? " >" : "  ", 
            field_cur == 15 ? ANSI_HIGHLIGHT : "", user_io_bound_io_cycle_min, ANSI_RESET,
            field_cur == 16 ? ANSI_HIGHLIGHT : "", user_io_bound_io_cycle_max, ANSI_RESET);

        printf("\n%s CPU-Bound Portion (%%): < %s%d %%%s >\n", field_cur == 17 ? " >" : "  ", field_cur == 17 ? ANSI_HIGHLIGHT : "", user_portion_of_cpu_bound, ANSI_RESET);
        printf("\n%s [%s GENERATE PROCESSES %s]\n", field_cur == 18 ? " >" : "  ", field_cur == 18 ? ANSI_HIGHLIGHT : "", ANSI_RESET);

        int ch = getch_linux();
        if (ch == 27) {
            if (getch_linux() == 91) {
                int arrow = getch_linux();
                if (arrow == KEY_UP) field_cur = (field_cur - 1 + num_fields) % num_fields;
                else if (arrow == KEY_DOWN) field_cur = (field_cur + 1) % num_fields;
                else if (arrow == KEY_LEFT) {
                    if (field_cur == 0 && user_min_process_count > 1) user_min_process_count--;
                    else if (field_cur == 1 && user_max_process_count > user_min_process_count) user_max_process_count--;
                    else if (field_cur == 2 && user_max_priority > 1) user_max_priority--;
                    else if (field_cur == 3 && user_min_arrival > 0) user_min_arrival--;
                    else if (field_cur == 4 && user_max_arrival > user_min_arrival) user_max_arrival--;
                    else if (field_cur == 5 && user_cpu_bound_cpu_min > 1) user_cpu_bound_cpu_min--;
                    else if (field_cur == 6 && user_cpu_bound_cpu_max > user_cpu_bound_cpu_min) user_cpu_bound_cpu_max--;
                    else if (field_cur == 7 && user_cpu_bound_io_min > 1) user_cpu_bound_io_min--;
                    else if (field_cur == 8 && user_cpu_bound_io_max > user_cpu_bound_io_min) user_cpu_bound_io_max--;
                    else if (field_cur == 9 && user_cpu_bound_io_cycle_min > 0) user_cpu_bound_io_cycle_min--;
                    else if (field_cur == 10 && user_cpu_bound_io_cycle_max > user_cpu_bound_io_cycle_min) user_cpu_bound_io_cycle_max--;
                    else if (field_cur == 11 && user_io_bound_cpu_min > 1) user_io_bound_cpu_min--;
                    else if (field_cur == 12 && user_io_bound_cpu_max > user_io_bound_cpu_min) user_io_bound_cpu_max--;
                    else if (field_cur == 13 && user_io_bound_io_min > 1) user_io_bound_io_min--;
                    else if (field_cur == 14 && user_io_bound_io_max > user_io_bound_io_min) user_io_bound_io_max--;
                    else if (field_cur == 15 && user_io_bound_io_cycle_min > 0) user_io_bound_io_cycle_min--;
                    else if (field_cur == 16 && user_io_bound_io_cycle_max > user_io_bound_io_cycle_min) user_io_bound_io_cycle_max--;
                    else if (field_cur == 17 && user_portion_of_cpu_bound > 0) user_portion_of_cpu_bound -= 5;
                }
                else if (arrow == KEY_RIGHT) {
                    if (field_cur == 0 && user_min_process_count < user_max_process_count) user_min_process_count++;
                    else if (field_cur == 1 && user_max_process_count < MAX_PROCESS_COUNT) user_max_process_count++;
                    else if (field_cur == 2) user_max_priority++;
                    else if (field_cur == 3 && user_min_arrival < user_max_arrival) user_min_arrival++;
                    else if (field_cur == 4) user_max_arrival++;
                    else if (field_cur == 5 && user_cpu_bound_cpu_min < user_cpu_bound_cpu_max) user_cpu_bound_cpu_min++;
                    else if (field_cur == 6) user_cpu_bound_cpu_max++;
                    else if (field_cur == 7 && user_cpu_bound_io_min < user_cpu_bound_io_max) user_cpu_bound_io_min++;
                    else if (field_cur == 8) user_cpu_bound_io_max++;
                    else if (field_cur == 9 && user_cpu_bound_io_cycle_min < user_cpu_bound_io_cycle_max) user_cpu_bound_io_cycle_min++;
                    else if (field_cur == 10 && user_cpu_bound_io_cycle_max < IO_CYCLE_MAX) user_cpu_bound_io_cycle_max++;
                    else if (field_cur == 11 && user_io_bound_cpu_min < user_io_bound_cpu_max) user_io_bound_cpu_min++;
                    else if (field_cur == 12) user_io_bound_cpu_max++;
                    else if (field_cur == 13 && user_io_bound_io_min < user_io_bound_io_max) user_io_bound_io_min++;
                    else if (field_cur == 14) user_io_bound_io_max++;
                    else if (field_cur == 15 && user_io_bound_io_cycle_min < user_io_bound_io_cycle_max) user_io_bound_io_cycle_min++;
                    else if (field_cur == 16 && user_io_bound_io_cycle_max < IO_CYCLE_MAX) user_io_bound_io_cycle_max++;
                    else if (field_cur == 17 && user_portion_of_cpu_bound < 100) user_portion_of_cpu_bound += 5;
                }
            }
        } else if (ch == KEY_ENTER) {
            if (field_cur == 18) break;
            else field_cur = (field_cur + 1) % num_fields;
        }
    }
    printf(ANSI_SHOW_CURSOR);
}