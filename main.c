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

    process_random_generator();
    proc_allocation();
    cpu_allocation();
    io_allocation();
    rq_allocation();
    wq_allocation();

    config(SCP_SINGLE_QUEUE, FCFS);
    simulate(SCP_SINGLE_QUEUE);
    graph();
    cleanup();

    printf("Completed \n");
    return 0;
}