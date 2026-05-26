#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "color.h"
#include "simulation.h"
#include "cmp_func.h"
#include "graph.h"
int main() {
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((unsigned int)(ts.tv_nsec));
    get_process_generation_params(); // Get process generation parameters from user
    while(1) {
        printf("Process Data Creation Started. (randomly...)\n");
        process_random_generator();

        int same_process = 1;
        while(same_process) {
            // 시뮬레이션 회차마다 자원 재할당 (PCB는 유지)
            proc_allocation();
            cpu_allocation();
            io_allocation();
            rq_allocation();
            wq_allocation();

            printf("\n--- Start Simulation Configuration ---\n");
            char* states[] = {
                "Single Processor & Single Queue",
                "Single Processor & MultiLevel Queue",
                "Single Processor & MultiLevel Feedback Queue",
                "Multi Processor & Common Queue",
                "Multi Processor & Own Queue"
            };
            int rq_s = select_menu("Choose Resource State", states, 5);

            config((rq_situation)rq_s);

            for (int i = 0; i < rq_count; i++) {
                set_rq_params_interactive(i);
            }

            simulate((rq_situation)rq_s);
            graph();

            printf(ANSI_BOLD_YELLOW "\nSimulation and Evaluation complete. Press any key to see next options..." ANSI_RESET "\n");
            getch_linux();

            // 시뮬레이션 자원만 해제하여 PCB 데이터 보존
            proc_cleanup();
            cpu_cleanup();
            io_cleanup();
            rq_cleanup();
            wq_cleanup();

            char* again_options[] = {"Yes, try another config", "No, generate new processes"};
            int choice = select_menu("Test another configuration with the SAME processes?", again_options, 2);
            if (choice == 1) same_process = 0;
        }

        char* exit_options[] = {"Yes, generate new set", "No, terminate program"};
        int final_choice = select_menu("Do you want to generate a NEW set of processes?", exit_options, 2);
        
        // 메뉴 확인이 끝난 직후(새 프로세스를 만들거나 종료하기 전)에 해제
        PCB_cleanup();
        if (final_choice == 1) break;
    }
    printf("Simulation Program Terminated.\n");
    return 0;
}