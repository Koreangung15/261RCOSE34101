#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "color.h"
#include "simulation.h"
#include "cmp_func.h"
#include "graph.h"

int get_safe_int(const char* prompt, int min_val, int max_val) {
    int value;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &value) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        if (value >= min_val && value <= max_val) {
            return value;
        } else {
            printf("Out of range. Please enter a value between %d and %d.\n", min_val, max_val);
        }
    }
}
int main() {
    printf("\033[H\033[J");
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    srand((unsigned int)(ts.tv_nsec));
    
    PCB_allocation();
    proc_allocation();
    cpu_allocation();
    io_allocation();
    rq_allocation();
    wq_allocation();
    int new_process = 1;
    while(new_process){
        int new_algorithm = 1;
        process_random_generator();
        while (new_algorithm){
            int rq_s;
            int alg;
            printf("\n==================================================\n");
            printf(" Select Resource Situation\n");
            printf("==================================================\n");
            printf(" 0: Single Core Processor, Single Queue\n");
            printf(" 1: Single Core Processor, Multilevel Queue             %d tier Ready Queue (Built_in_Scheduling_Criteria)\n", MAX_RQ_COUNT);
            printf(" 2: Single Core Processor, Multilevel Feedback Queue    %d tier Ready Queue (Built_in_Scheduling_Criteria)\n", MAX_RQ_COUNT);
            printf(" 3: Multi Core Processor, Common Queue\n");
            printf(" 4: Multi Core Processor, Own Queue                     %d Ready Queue\n", MAX_RQ_COUNT);
            printf("--------------------------------------------------\n");
            
            rq_s = get_safe_int(">> Enter Selection (0-4): ", 0, 4);
            if (rq_s == 0 || rq_s == 3) {
                printf("\n==================================================\n");
                printf("Select Scheduling Algorithm\n");
                printf("==================================================\n");
                printf(" 0: FCFS\n");
                printf(" 1: SJF\n");
                printf(" 2: PRIORITY\n");
                printf(" 3: Round Robin\n");
                printf(" 4: PREEMPTIVE_SJF\n");
                printf(" 5: PREEMPTIVE_PRIORITY\n");
                printf(" 6: PRIORITY_AGING\n");
                printf(" 7: PREEMPTIVE_PRIORITY_AGING\n");
                printf("--------------------------------------------------\n");
                alg = get_safe_int(">> Enter Selection (0-7): ", 0, 7);

                if (alg == 0 || alg == 1 || alg == 2 || alg == 4 || alg == 5) {
                    if (alg == 4 || alg == 5) {
                        rqconfigs[0].preemptive = 1;
                    }
                    else{
                        rqconfigs[0].preemptive = 0;
                    }

                    if (alg == 0) {
                        rqconfigs[0].compare = fcfs_cmp;
                    }
                    else if (alg == 1 || alg == 4) {
                        rqconfigs[0].compare = sjf_cmp;
                    }
                    else if (alg == 2 || alg == 5) {
                        rqconfigs[0].compare = priority_cmp;
                    }
                    rqconfigs[0].aging_interval = 0;
                    rqconfigs[0].time_quantum = 0;
                }
                else if (alg == 3) {
                    int timequantum = get_safe_int(">> Enter time_quantum (1 ~ 20) :", 1, 20);
                    rqconfigs[0].preemptive = 0;
                    rqconfigs[0].compare = rr_fcfs_cmp;
                    rqconfigs[0].time_quantum = timequantum;
                    rqconfigs[0].aging_interval = 0;
                }
                else {
                    int aginginterval = get_safe_int(">> Enter Aging Interval (-1 per tick) (5 ~ 20)", 5, 20);
                    if (alg == 7) {
                        rqconfigs[0].preemptive = 1;
                    }
                    else {
                        rqconfigs[0].preemptive = 0;
                    }
                    rqconfigs[0].compare = priority_cmp;
                    rqconfigs[0].time_quantum = 0;
                    rqconfigs[0].aging_interval = aginginterval;
                }

            }
            else if (rq_s == 4) {
                for(int i = 0; i < MAX_RQ_COUNT;i++){
                    printf("\n==================================================\n");
                    printf("Ready Queue %d: Select Scheduling Algorithm\n", i);
                    printf("==================================================\n");
                    printf(" 0: FCFS\n");
                    printf(" 1: SJF\n");
                    printf(" 2: PRIORITY\n");
                    printf(" 3: Round Robin\n");
                    printf(" 4: PREEMPTIVE_SJF\n");
                    printf(" 5: PREEMPTIVE_PRIORITY\n");
                    printf(" 6: PRIORITY_AGING\n");
                    printf(" 7: PREEMPTIVE_PRIORITY_AGING\n");
                    printf("--------------------------------------------------\n");
                    alg = get_safe_int(">> Enter Selection (0-7): ", 0, 7);

                    if (alg == 0 || alg == 1 || alg == 2 || alg == 4 || alg == 5) {
                        if (alg == 4 || alg == 5) {
                            rqconfigs[i].preemptive = 1;
                        }
                        else{
                            rqconfigs[i].preemptive = 0;
                        }

                        if (alg == 0) {
                            rqconfigs[i].compare = fcfs_cmp;
                        }
                        else if (alg == 1 || alg == 4) {
                            rqconfigs[i].compare = sjf_cmp;
                        }
                        else if (alg == 2 || alg == 5) {
                            rqconfigs[i].compare = priority_cmp;
                        }
                        rqconfigs[i].aging_interval = 0;
                        rqconfigs[i].time_quantum = 0;
                    }
                    else if (alg == 3) {
                        int timequantum = get_safe_int(">> Enter time_quantum (1 ~ 20) :", 1, 20);
                        rqconfigs[i].preemptive = 0;
                        rqconfigs[i].compare = rr_fcfs_cmp;
                        rqconfigs[i].time_quantum = timequantum;
                        rqconfigs[i].aging_interval = 0;
                    }
                    else {
                        int aginginterval = get_safe_int(">> Enter Aging Interval (-1 per tick) (5 ~ 20)", 5, 20);
                        if (alg == 7) {
                            rqconfigs[i].preemptive = 1;
                        }
                        else {
                            rqconfigs[i].preemptive = 0;
                        }
                        rqconfigs[i].compare = priority_cmp;
                        rqconfigs[i].time_quantum = 0;
                        rqconfigs[i].aging_interval = aginginterval;
                    }
                }
            }
            config((rq_situation) rq_s, NONE_ALG);
            simulate((rq_situation) rq_s);

            graph();
            FILE* fp = fopen("result.txt", "w");
            file_PCB(fp);
            file_graph(fp);
            fclose(fp);

            printf("\nWould you like to try another algorithm?\n");
            int yes = get_safe_int(">> (0: Yes, 1: New Process Stage): ", 0, 1);
            if(yes != 0){
                new_algorithm = 0;
            }
        }
        printf("Would you like to try other new processes? (0: Yes, 1: Exit)\n");
        int yes = get_safe_int(">> (0: Yes, 1: Exit): ", 0, 1);
        if(yes != 0){
            new_process = 0;
        }
    }
    cleanup();

    printf("Completed \n");
    return 0;
}