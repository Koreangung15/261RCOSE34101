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

#define SEGMENTS_PER_LINE 20 // 한 줄에 표시할 간트 차트 박스 개수

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

    printf("\nProcess Time Checking\n");
    printf(ANSI_BOLD "%6s | %6s | %6s | %12s | %12s | %12s | %12s \n" ANSI_RESET, "PID", "CPU", "IO", "READY_QUEUE", "WAIT_QUEUE", "TURN_AROUND", "WAITING_TIME");
    printf("-----------------------------------------------------------------------------------------\n");

    for(int i = 0; i < PCB_size; i++){
        printf("%6d | %6d | %6d | %12d | %12d | %12d | %12d \n", 
            proc[i] -> p_info_ptr -> pid,
            cpu_running_time[i],
            io_running_time[i],
            waiting_time_in_ready[i],
            waiting_time_in_waiting[i],
            waiting_time_in_ready[i] + cpu_running_time[i],
            waiting_time_in_ready[i]
        );
    }
    printf("\n");

    printf(ANSI_BOLD_CYAN "=== Gantt Chart ===" ANSI_RESET "\n");

    for(int i = 0; i < cpu_count; i++){
        interval_len = 0;
        int left = 0;
        int right = 1;
        printf(ANSI_BOLD_YELLOW "\n[ CPU %d ]" ANSI_RESET "\n", i);
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

        // 줄바꿈 로직 적용
        for (int line_start = 0; line_start < interval_len; line_start += SEGMENTS_PER_LINE) {
            int line_end = (line_start + SEGMENTS_PER_LINE < interval_len) ? line_start + SEGMENTS_PER_LINE : interval_len;

            // 상단 테두리
            printf("    +");
            for (int j = line_start; j < line_end; j++) printf("------+");
            printf("\n");

            // PID/내용 (중간 패딩 제거하여 높이 축소)
            printf("    |");
            for (int j = line_start; j < line_end; j++) {
                if (interval[j][2] == -1) printf(" idle |");
                else printf(" %-5d|", interval[j][2]);
            }
            printf("\n");

            // 하단 테두리
            printf("    +");
            for (int j = line_start; j < line_end; j++) printf("------+");
            printf("\n");

            // 시간 표시
            printf("    %-7d", interval[line_start][0]);
            for (int j = line_start; j < line_end; j++) printf("%-7d", interval[j][1]);
            printf("\n"); // 각 줄 사이 간격
        }
    }
}