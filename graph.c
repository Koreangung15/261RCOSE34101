#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "config.h"
#include "simulation.h"
#include "graph.h"
#include "color.h"

int waiting_time_in_ready[MAX_PROCESS_COUNT];
int waiting_time_in_waiting[MAX_PROCESS_COUNT];
int cpu_running_time[MAX_PROCESS_COUNT];
int io_running_time[MAX_PROCESS_COUNT];

int interval[MAX_TIME_LINE][3];
int interval_len;
void graph(){
    for(int i = 0; i < PCB_size; i++){
        waiting_time_in_ready[i] = 0;
        waiting_time_in_waiting[i] = 0;
        cpu_running_time[i] = 0;
        io_running_time[i] = 0;
    }

    for(int i = 0; i < PCB_size; i++){
        for(int t = 0; t < current_time; t++){
            switch(proc[i] -> chart[t].name){
                case CPU_RUNNING:
                    cpu_running_time[i]++;
                    break;
                case IO_RUNNING:
                    io_running_time[i]++;
                    break;
                case READY:
                    waiting_time_in_ready[i]++;
                    break;
                case WAITING:
                    waiting_time_in_waiting[i]++;
                    break;
            }
        }
    }

    printf("\nEvaluation\n");
    printf(ANSI_BOLD "%6s | %6s | %6s | %12s | %12s \n" ANSI_RESET, "PID", "CPU", "IO", "READY_QUEUE", "WAIT_QUEUE");
    printf("-----------------------------------------------------------------------------------------\n");

    for(int i = 0; i < PCB_size; i++){
        printf(ANSI_BOLD "%6d | %6d | %6d | %12d | %12d " ANSI_RESET "\n", 
            proc[i] -> p_info_ptr -> pid,
            cpu_running_time[i],
            io_running_time[i],
            waiting_time_in_ready[i],
            waiting_time_in_waiting[i]
        );
    }
    int total_cpu_time = 0;
    int total_io_time = 0;
    int total_rq_time = 0;
    int total_wq_time = 0;

    for(int i = 0; i < PCB_size; i++){
        total_cpu_time += cpu_running_time[i];
        total_io_time += io_running_time[i];
        total_rq_time += waiting_time_in_ready[i];
        total_wq_time += waiting_time_in_waiting[i];
    }
    printf("-----------------------------------------------------------------------------------------\n");
    printf(ANSI_BOLD "%7s| %6.3f | %6.3f | %12.3f | %12.3f " ANSI_RESET "\n",
        "Average",
        (float) total_cpu_time / (float) PCB_size,
        (float) total_io_time / (float) PCB_size,
        (float) total_rq_time / (float) PCB_size,
        (float) total_wq_time / (float) PCB_size
    );
    printf("\n\n");

    for(int i = 0; i < cpu_count; i++){
        interval_len = 0;
        int left = 0;
        int right = 1;
        printf(ANSI_BOLD_YELLOW "\n[ CPU %d ]" ANSI_RESET "\n", i);
        printf(ANSI_BOLD_CYAN "CPU utilization: %.3f" ANSI_RESET "\n", (float)(cpu[i] -> runtime) / (float)(current_time));
        while(right < current_time){
            if (cpu[i] -> chart[left] != cpu[i] -> chart[right]) {
                interval[interval_len][0] = left;
                interval[interval_len][1] = right;
                interval[interval_len][2] = cpu[i] -> chart[left];
                interval_len++;
                left = right;
            }
            right++;
        }

        interval[interval_len][0] = left;
        interval[interval_len][1] = right;
        interval[interval_len][2] = cpu[i] -> chart[left];
        interval_len++;

        int start = 0;
        while (start < interval_len){
            printf("+");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                printf("------+");
            }
            printf("\n");
            printf("|");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                interval[t][2] != -1 ? printf(" %-5d|", interval[t][2]) : printf(ANSI_BOLD_RED " IDLE " ANSI_RESET "|");
            }
            printf("\n");
            printf("+");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                printf("------+");
            }
            printf("\n");
            printf("%-7d", interval[start][0]);
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                printf("%-7d", interval[t][1]);
            }
            printf("\n");

            start += GANTT_CHART_LENGTH;
        }
    }
}

void file_PCB(FILE* fp) {
    fprintf(fp, "%-6s | %-12s | %-10s | %-12s | %s ", "PID", "Arrival Time", "Prioriy", "Type", "Burst Time");
    fprintf(fp, "( CPU, IO(io_type) )\n" );
    fprintf(fp, "------------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < PCB_size; i++) {
        fprintf(fp, "%-6d | %-12d | %-10d | %-12s | ", PCB[i] -> pid, PCB[i] -> arrival_time, PCB[i] -> priority, PCB[i] -> type == CPU_BOUND ? "CPU bound" : "IO bound");
        for (int j = 0; j < PCB[i] -> burst_interval_size; j++){
            switch (PCB[i] -> burst_interval[j].type){
                case 0:
                    fprintf(fp, "%d", PCB[i] -> burst_interval[j].time);
                    break;
                default:
                    fprintf(fp, "%d(%d)", PCB[i] -> burst_interval[j].time, PCB[i] -> burst_interval[j].type - 1);
                    break;
            }
            if (j == PCB[i] -> burst_interval_size - 1) {
                fprintf(fp, "\n");
            }
            else{
                fprintf(fp, " -> ");
            }
        }
    }
}

void file_graph(FILE* fp) {
    for(int i = 0; i < PCB_size; i++){
        waiting_time_in_ready[i] = 0;
        waiting_time_in_waiting[i] = 0;
        cpu_running_time[i] = 0;
        io_running_time[i] = 0;
    }

    for(int i = 0; i < PCB_size; i++){
        for(int t = 0; t < current_time; t++){
            switch(proc[i] -> chart[t].name){
                case CPU_RUNNING:
                    cpu_running_time[i]++;
                    break;
                case IO_RUNNING:
                    io_running_time[i]++;
                    break;
                case READY:
                    waiting_time_in_ready[i]++;
                    break;
                case WAITING:
                    waiting_time_in_waiting[i]++;
                    break;
            }
        }
    }

    fprintf(fp, "\nEvaluation\n");
    fprintf(fp, "%6s | %6s | %6s | %12s | %12s \n", "PID", "CPU", "IO", "READY_QUEUE", "WAIT_QUEUE");
    fprintf(fp, "-----------------------------------------------------------------------------------------\n");

    for(int i = 0; i < PCB_size; i++){
        fprintf(fp, "%6d | %6d | %6d | %12d | %12d \n", 
            proc[i] -> p_info_ptr -> pid, 
            cpu_running_time[i], 
            io_running_time[i], 
            waiting_time_in_ready[i], 
            waiting_time_in_waiting[i]
        );
    }

    int total_cpu_time = 0;
    int total_io_time = 0;
    int total_rq_time = 0;
    int total_wq_time = 0;

    for(int i = 0; i < PCB_size; i++){
        total_cpu_time += cpu_running_time[i];
        total_io_time += io_running_time[i];
        total_rq_time += waiting_time_in_ready[i];
        total_wq_time += waiting_time_in_waiting[i];
    }
    fprintf(fp, "-----------------------------------------------------------------------------------------\n");
    fprintf(fp, "%7s| %6.3f | %6.3f | %12.3f | %12.3f \n",
        "Average",
        (float) total_cpu_time / (float) PCB_size,
        (float) total_io_time / (float) PCB_size,
        (float) total_rq_time / (float) PCB_size,
        (float) total_wq_time / (float) PCB_size
    );
    fprintf(fp, "\n\n");

    for(int i = 0; i < cpu_count; i++){
        interval_len = 0;
        int left = 0;
        int right = 1;
        fprintf(fp, "\n[ CPU %d ]\n", i);
        fprintf(fp, "CPU utilization: %.3f\n", (float)cpu[i] -> runtime / (float)(current_time));
        while(right < current_time){
            if (cpu[i] -> chart[left] != cpu[i] -> chart[right]) {
                interval[interval_len][0] = left;
                interval[interval_len][1] = right;
                interval[interval_len][2] = cpu[i] -> chart[left];
                interval_len++;
                left = right;
            }
            right++;
        }

        interval[interval_len][0] = left;
        interval[interval_len][1] = right;
        interval[interval_len][2] = cpu[i] -> chart[left];
        interval_len++;

        int start = 0;
        while (start < interval_len){
            fprintf(fp, "+");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                fprintf(fp, "------+");
            }
            fprintf(fp, "\n");
            fprintf(fp, "|");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                interval[t][2] != -1 ? fprintf(fp, " %-5d|", interval[t][2]) : fprintf(fp, " IDLE |");
            }
            fprintf(fp, "\n");
            fprintf(fp, "+");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                fprintf(fp, "------+");
            }
            fprintf(fp, "\n");
            fprintf(fp, "%-7d", interval[start][0]);
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                fprintf(fp, "%-7d", interval[t][1]);
            }
            fprintf(fp, "\n");

            start += GANTT_CHART_LENGTH;
        }
    }

    for(int i = 0; i < io_wq_count; i++){
        interval_len = 0;
        int left = 0;
        int right = 1;
        fprintf(fp, "\n[ IO %d ]\n", i);
        fprintf(fp, "IO utilization: %.3f\n", (float)io[i] -> runtime / (float)(current_time));
        while(right < current_time){
            if (io[i] -> chart[left] != io[i] -> chart[right]) {
                interval[interval_len][0] = left;
                interval[interval_len][1] = right;
                interval[interval_len][2] = io[i] -> chart[left];
                interval_len++;
                left = right;
            }
            right++;
        }

        interval[interval_len][0] = left;
        interval[interval_len][1] = right;
        interval[interval_len][2] = io[i] -> chart[left];
        interval_len++;

        int start = 0;
        while (start < interval_len){
            fprintf(fp, "+");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                fprintf(fp, "------+");
            }
            fprintf(fp, "\n");
            fprintf(fp, "|");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                interval[t][2] != -1 ? fprintf(fp, " %-5d|", interval[t][2]) : fprintf(fp, " IDLE |");
            }
            fprintf(fp, "\n");
            fprintf(fp, "+");
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                fprintf(fp, "------+");
            }
            fprintf(fp, "\n");
            fprintf(fp, "%-7d", interval[start][0]);
            for(int t = start; t < ((start + GANTT_CHART_LENGTH) <= interval_len ? (start + GANTT_CHART_LENGTH) : interval_len); t++){
                fprintf(fp, "%-7d", interval[t][1]);
            }
            fprintf(fp, "\n");

            start += GANTT_CHART_LENGTH;
        }
    }
}