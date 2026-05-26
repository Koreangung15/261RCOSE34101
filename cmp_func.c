#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "color.h"
#include "mem_alloc.h"
#include "random_distribution.h"
#include "cmp_func.h"

int p_info_compare(const void *a, const void *b) {
    Process_info* p_a = *(Process_info**) a;
    Process_info* p_b = *(Process_info**) b;

    if (p_a == NULL && p_b == NULL) return 0;
    if (p_a == NULL) return 1;
    if (p_b == NULL) return -1;
    int p_info_arrive_a = p_a -> arrival_time;
    int p_info_arrive_b = p_b -> arrival_time;

    int id1 = p_a -> pid;
    int id2 = p_b -> pid;

    if (p_info_arrive_a != p_info_arrive_b) return p_info_arrive_a > p_info_arrive_b ? 1 : -1;
    if (id1 != id2) return id1 > id2 ? 1 : -1;
    return 0;
}

int fcfs_cmp(const void *a, const void *b){
    Process* p_a = *(Process**) a;
    Process* p_b = *(Process**) b;

    if (p_a == NULL && p_b == NULL) return 0;
    if (p_a == NULL) return 1;
    if (p_b == NULL) return -1;

    int a_t1 = p_a -> p_info_ptr -> arrival_time;
    int a_t2 = p_b -> p_info_ptr -> arrival_time;
    int id1 = p_a -> p_info_ptr -> pid;
    int id2 = p_b -> p_info_ptr -> pid;

    if (a_t1 != a_t2) return (a_t1 > a_t2) ? 1 : -1;
    if (id1 != id2) return (id1 > id2) ? 1 : -1;
    return 0;
}

int rr_fcfs_cmp(const void *a, const void *b){
    Process* p_a = *(Process**) a;
    Process* p_b = *(Process**) b;

    if (p_a == NULL && p_b == NULL) return 0;
    if (p_a == NULL) return 1;
    if (p_b == NULL) return -1;

    int a_t1 = p_a -> current_state.current_arrival_time;
    int a_t2 = p_b -> current_state.current_arrival_time;
    
    int from_cpu1 = p_a -> from_cpu;
    int from_cpu2 = p_b -> from_cpu;

    int id1 = p_a -> p_info_ptr -> pid;
    int id2 = p_b -> p_info_ptr -> pid;

    if (a_t1 != a_t2) return (a_t1 > a_t2) ? 1 : -1;
    if (from_cpu1 != from_cpu2) return (from_cpu1 > from_cpu2) ? 1 : -1;
    if (id1 != id2) return (id1 > id2) ? 1 : -1;
    return 0;
}

int sjf_cmp(const void *a, const void *b){
    Process* p_a = *(Process**) a;
    Process* p_b = *(Process**) b;

    if (p_a == NULL && p_b == NULL) return 0;
    if (p_a == NULL) return 1;
    if (p_b == NULL) return -1;

    int b_t1 = p_a -> current_state.current_burst_time;
    int b_t2 = p_b -> current_state.current_burst_time;
    int id1 = p_a -> p_info_ptr -> pid;
    int id2 = p_b -> p_info_ptr -> pid;

    if (b_t1 != b_t2) return (b_t1 > b_t2) ? 1 : -1;
    if (id1 != id2) return (id1 > id2) ? 1 : -1;
    return 0;
}

int rr_sjf_cmp(const void *a, const void *b){
    Process* p_a = *(Process**) a;
    Process* p_b = *(Process**) b;

    if (p_a == NULL && p_b == NULL) return 0;
    if (p_a == NULL) return 1;
    if (p_b == NULL) return -1;

    int b_t1 = p_a -> current_state.current_burst_time;
    int b_t2 = p_b -> current_state.current_burst_time;

    int from_cpu1 = p_a -> from_cpu;
    int from_cpu2 = p_b -> from_cpu;

    int id1 = p_a -> p_info_ptr -> pid;
    int id2 = p_b -> p_info_ptr -> pid;

    if (b_t1 != b_t2) return (b_t1 > b_t2) ? 1 : -1;
    if (from_cpu1 != from_cpu2) return (from_cpu1 > from_cpu2) ? 1 : -1;
    if (id1 != id2) return (id1 > id2) ? 1 : -1;
    return 0;
}

int priority_cmp(const void *a, const void *b){
    Process* p_a = *(Process**) a;
    Process* p_b = *(Process**) b;

    if (p_a == NULL && p_b == NULL) return 0;
    if (p_a == NULL) return 1;
    if (p_b == NULL) return -1;

    int prior_1 = p_a -> current_state.current_priority;
    int prior_2 = p_b -> current_state.current_priority;
    int id1 = p_a -> p_info_ptr -> pid;
    int id2 = p_b -> p_info_ptr -> pid;

    if (prior_1 != prior_2) return (prior_1 > prior_2) ? 1 : -1;
    if (id1 != id2) return (id1 > id2) ? 1 : -1;
    return 0;
}

int rr_priority_cmp(const void *a, const void *b){
    Process* p_a = *(Process**) a;
    Process* p_b = *(Process**) b;

    if (p_a == NULL && p_b == NULL) return 0;
    if (p_a == NULL) return 1;
    if (p_b == NULL) return -1;

    int prior_1 = p_a -> current_state.current_priority;
    int prior_2 = p_b -> current_state.current_priority;

    int from_cpu1 = p_a -> from_cpu;
    int from_cpu2 = p_b -> from_cpu;

    int id1 = p_a -> p_info_ptr -> pid;
    int id2 = p_b -> p_info_ptr -> pid;

    if (prior_1 != prior_2) return (prior_1 > prior_2) ? 1 : -1;
    if (from_cpu1 != from_cpu2) return (from_cpu1 > from_cpu2) ? 1 : -1;
    if (id1 != id2) return (id1 > id2) ? 1 : -1;
    return 0;
}