#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "simulation.h"

int current_time = 0;

int time_quantum_expired(CPU_resource* cpu_ptr){
    return cpu_ptr -> remain_time_quantum == 0;
}

int terminated(Process* proc_ptr){
    return burst_expired(proc_ptr) && proc_ptr -> current_state.current_burst_idx == proc_ptr -> p_info_ptr -> burst_interval_size - 1;
}

int burst_expired(Process* proc_ptr){
    return proc_ptr -> current_state.current_burst_time == 0;
}

void insert_new_process(Process* proc_ptr, Ready_Queue* rq_ptr) {
    if (rq_ptr -> base -> current_process_count >= MAX_PROCESS_COUNT) {
        fprintf(stderr, "[%d]: Ready Queue Insertion Failed! \n", current_time);
        cleanup();
        exit(1);
    }

    int inserted = 0;
    for(int i = rq_ptr -> base -> current_process_count - 1; i >= 0; i--){
        if (rq_ptr -> compare(&proc_ptr, &(rq_ptr -> base -> items[i])) < 0) {
            rq_ptr -> base -> items[i + 1] = rq_ptr -> base -> items[i];
        }
        else {
            rq_ptr -> base -> items[i + 1] = proc_ptr;
            inserted++;
            break;
        }
    }
    if (!inserted){
        rq_ptr -> base -> items[0] = proc_ptr;
    }

    rq_ptr -> base -> current_process_count++;
}
void to_ready(Process* proc_ptr, rq_situation rq_s, int flag) {
    proc_ptr -> current_state.name = READY;
    proc_ptr -> current_state.resource_type = READY_QUEUE;
    proc_ptr -> current_state.current_arrival_time = current_time;

    if (proc_ptr -> from_cpu) {
        if (rq_s == SCP_MLFQ){
            int tier = proc_ptr -> current_state.current_priority / priority_interval_per_tier;
            int new_priority = (tier + 2) * priority_interval_per_tier - 1;
            proc_ptr -> current_state.current_priority = new_priority <= MAX_PRIORITY - 1 ? new_priority : MAX_PRIORITY - 1;
        }
        else{
            proc_ptr -> current_state.current_priority = proc_ptr -> p_info_ptr -> priority;
        }
    }
    Ready_Queue* rq_ptr = NULL;
    if (rq_s == SCP_SINGLE_QUEUE || rq_s == MCP_COMMON_QUEUE) {
        rq_ptr = rq[0];
        proc_ptr -> current_state.resource_idx = 0;
    }
    else if (rq_s == SCP_MLQ || rq_s == SCP_MLFQ) {
        rq_ptr = rq[proc_ptr -> current_state.current_priority / priority_interval_per_tier];
        proc_ptr -> current_state.resource_idx = proc_ptr -> current_state.current_priority / priority_interval_per_tier;
    }
    else if (rq_s == MCP_OWN_QUEUE){
        int cpu_idx = flag;
        if (cpu_idx < 0){
            // load balancing
            int min_count = rq[0] -> base -> current_process_count;
            cpu_idx = 0;

            for(int j = 1; j < rq_count; j++){
                if (min_count > rq[j] -> base -> current_process_count){
                    min_count = rq[j] -> base -> current_process_count;
                    cpu_idx = j;
                }
            }
        }
        
        rq_ptr = rq[cpu_idx];
        proc_ptr -> current_state.resource_idx = cpu_idx;
    }
    if (flag == -2){
        rq_ptr -> io_interrupted = 1;
    }
    
    insert_new_process(proc_ptr, rq_ptr);
}


void tick_flow(rq_situation rq_s){
    for(int i = 0; i < PCB_size; i++){
        proc[i] -> from_cpu = 0;
    }
    for(int i = 0; i < cpu_count; i++){
        if(cpu[i] -> base -> current_process_count == 1){
            cpu[i] -> base -> items[0] -> current_state.current_burst_time--;

            cpu[i] -> remain_time_quantum -= cpu[i] -> remain_time_quantum > 0 ? 1 : 0;
        }
    }

    for(int i = 0; i < io_wq_count; i++){
        if(io[i] -> base -> current_process_count == 1){
            io[i] -> base -> items[0] -> current_state.current_burst_time--;
        }
    }

    for (int i = 0; i < rq_count; i++){
        rq[i] -> io_interrupted = 0;
    }
    if (rq_s == SCP_SINGLE_QUEUE || rq_s == MCP_COMMON_QUEUE || rq_s == MCP_OWN_QUEUE){
        for(int i = 0; i < rq_count; i++){
            if (rq[i] -> aging_interval > 0){
                for(int j = 0; j < rq[i] -> base -> current_process_count; j++){
                    int existed_time = current_time - rq[i] -> base -> items[j] -> current_state.current_arrival_time;

                    if(existed_time > 0 && existed_time % rq[i] -> aging_interval == 0 && rq[i] -> base -> items[j] -> current_state.current_priority > 0){
                        rq[i] -> base -> items[j] -> current_state.current_priority--;   
                    }
                }
            }
            if (rq[i] -> base -> current_process_count > 0) {
                qsort(rq[i] -> base -> items, rq[i] -> base -> current_process_count, sizeof(Process*), rq[i] -> compare);
            }   
        }
    }
    else if (rq_s == SCP_MLQ || rq_s == SCP_MLFQ) {
        for (int i = 0; i < rq_count; i++){
            if (rq[i] -> aging_interval > 0){
                for(int j = 0; j < rq[i] -> base -> current_process_count; j++){
                    int existed_time = current_time - rq[i] -> base -> items[j] -> current_state.current_arrival_time;

                    if(existed_time > 0 && existed_time % rq[i] -> aging_interval == 0 && rq[i] -> base -> items[j] -> current_state.current_priority > 0){
                        rq[i] -> base -> items[j] -> current_state.current_priority--;   
                    }
                }
            }
        }

        for (int i = 0; i < rq_count; i++){
            for(int j = 0; j < rq[i] -> base -> current_process_count; j++){
                if (rq[i] -> base -> items[j] -> current_state.current_priority / priority_interval_per_tier != i) {

                    Process* move_proc = rq[i] -> base -> items[j];

                    for(int k = j; k < rq[i] -> base -> current_process_count - 1; k++){
                        rq[i] -> base -> items[k] = rq[i] -> base -> items[k + 1];
                    }
                    to_ready(move_proc, rq_s, -1);
                    rq[i] -> base -> items[rq[i] -> base -> current_process_count - 1] = NULL;
                    rq[i] -> base -> current_process_count--;
                    j--;
                }
            }
        }
    }
}

void scan_io_resource(rq_situation rq_s){
    for(int i = 0; i < io_wq_count; i++){
        if (io[i] -> base -> current_process_count == 1) {
            Process* proc_ptr = io[i] -> base -> items[0];
            if (burst_expired(proc_ptr)) {
                proc_ptr -> current_state.current_burst_idx++;
                proc_ptr -> current_state.current_burst_time = proc_ptr -> p_info_ptr -> burst_interval[proc_ptr -> current_state.current_burst_idx].time;
                
                to_ready(proc_ptr, rq_s, -2);

                io[i] -> base -> items[0] = NULL;
                io[i] -> base -> current_process_count = 0;
            }
        }       
    }
}


void scan_cpu_resource(rq_situation rq_s){

    // Rollback Beausce of IO Interrupt
    /*
    if (rq_s == SCP_SINGLE_QUEUE || rq_s == SCP_MLQ || rq_s == SCP_MLFQ){
        for(int i = 0; i < rq_count; i++){
            if (rq[i] -> io_interrupted && cpu[0] -> base -> current_process_count == 1){
                cpu[0] -> base -> items[0] -> current_state.current_burst_time++;
                cpu[0] -> remain_time_quantum += cpu[0] -> remain_time_quantum == -1 ? 0 : 1;
                break;
            }
        }
    }
    else if (rq_s == MCP_COMMON_QUEUE) {
        int victim_idx = -1;
        for(int i = 0; i < cpu_count; i++){
            if (cpu[i] -> base -> current_process_count == 0) {
                victim_idx = i;
                break;
            }
            else{
                if (victim_idx == -1) {
                    victim_idx = i;
                }
                else{
                    if (rq[0] -> compare(&cpu[victim_idx] -> base -> items[0], &cpu[i] -> base -> items[0]) < 0){
                        victim_idx = i;
                    }
                }
            }
        }
        if (cpu[victim_idx] -> base -> current_process_count == 1) {
            cpu[victim_idx] -> base -> items[0] -> current_state.current_burst_time++;
            cpu[victim_idx] -> remain_time_quantum += cpu[victim_idx] -> remain_time_quantum == -1 ? 0 : 1;
        }
    }
    */
    for(int i = 0; i < cpu_count; i++){
        if (cpu[i] -> base -> current_process_count == 1) {
            Process* proc_ptr = cpu[i] -> base -> items[0];

            if (terminated(proc_ptr)) {
                proc_ptr -> current_state.name = TERMINATED;
                proc_ptr -> current_state.resource_type = NONE;
                proc_ptr -> current_state.resource_idx = 0;
                proc_ptr -> current_state.current_arrival_time = current_time;

                cpu[i] -> base -> items[0] = NULL;
                cpu[i] -> base -> current_process_count = 0;
            }
            else if (burst_expired(proc_ptr)){
                proc_ptr -> current_state.name = WAITING;
                proc_ptr -> current_state.current_burst_idx++;

                Burst next_burst = proc_ptr -> p_info_ptr -> burst_interval[proc_ptr -> current_state.current_burst_idx];

                proc_ptr -> current_state.resource_type = WAITING_QUEUE;
                proc_ptr -> current_state.current_arrival_time = current_time;
                proc_ptr -> current_state.current_burst_time = next_burst.time;
                proc_ptr -> current_state.resource_idx = next_burst.type - 1;

                int wq_idx = wq[next_burst.type - 1] -> base -> current_process_count;
                if (wq_idx >= MAX_PROCESS_COUNT) {
                    fprintf(stderr, "[%d]: Waiting Queue Insertion Failed! \n", current_time);
                    cleanup();
                    exit(1);
                }

                wq[next_burst.type - 1] -> base -> items[wq[next_burst.type - 1] -> base -> current_process_count++] = proc_ptr;

                cpu[i] -> base -> items[0] = NULL;
                cpu[i] -> base -> current_process_count = 0;
            }
            else if (time_quantum_expired(cpu[i])){
                proc_ptr -> from_cpu = 1;
                if (rq_s == MCP_OWN_QUEUE) {
                    to_ready(proc_ptr, rq_s, i);
                }
                else{
                    to_ready(proc_ptr, rq_s, -1);
                }

                cpu[i] -> base -> items[0] = NULL;
                cpu[i] -> base -> current_process_count = 0;
            }
        }
    }
}


void scan_wq_resource(){
    for(int i = 0; i < io_wq_count; i++){
        if (io[i] -> base -> current_process_count == 0 && wq[i] -> base -> current_process_count != 0){
            Process* proc_ptr = wq[i] -> base -> items[0];

            proc_ptr -> current_state.current_arrival_time = current_time;
            proc_ptr -> current_state.name = IO_RUNNING;
            proc_ptr -> current_state.resource_type = IO;
            proc_ptr -> current_state.resource_idx = i;
            for(int j = 0; j < wq[i] -> base -> current_process_count - 1; j++){
                wq[i] -> base -> items[j] = wq[i] -> base -> items[j + 1];
            }
            wq[i] -> base -> current_process_count--;
            io[i] -> base -> items[0] = proc_ptr;
            io[i] -> base -> current_process_count = 1;
        }
    }
}

void proc_generate(rq_situation rq_s){
    for(int i = 0; i < PCB_size; i++){
        if (proc[i] -> current_state.name == NOT_GENERATED && proc[i] -> p_info_ptr -> arrival_time == current_time){
            to_ready(proc[i], rq_s, -1);
        }
    }
}


void scan_rq_resource(rq_situation rq_s){
    if (rq_s == SCP_SINGLE_QUEUE || rq_s == MCP_OWN_QUEUE) {
        for(int idx = 0; idx < cpu_count; idx++){
            if (cpu[idx] -> base -> current_process_count == 1) {
                int need_preemption = 0;
                if (rq[idx] -> io_interrupted) {
                    need_preemption = 1;
                }
                else{
                    if (rq[idx] -> preemptive && rq[idx] -> base -> current_process_count > 0) {
                        need_preemption = rq[idx] -> compare(&(cpu[idx] -> base -> items[0]), &(rq[idx] -> base -> items[0])) > 0 ? 1 : 0;
                    }
                }
                if (rq[idx] -> preemptive && rq[idx] -> base -> current_process_count > 0) {
                    need_preemption = rq[idx] -> compare(&(cpu[idx] -> base -> items[0]), &(rq[idx] -> base -> items[0])) > 0 ? 1 : 0;
                }
                
                if (need_preemption){
                    cpu[idx] -> base -> items[0] -> from_cpu = 1;
                    to_ready(cpu[idx] -> base -> items[0], rq_s, idx);
                    cpu[idx] -> base -> items[0] = NULL;
                    cpu[idx] -> base -> current_process_count = 0;
                }
            }
            

            if (cpu[idx] -> base -> current_process_count == 0 && rq[idx] -> base -> current_process_count > 0){
                Process* run_proc = rq[idx] -> base -> items[0];

                run_proc -> current_state.current_arrival_time = current_time;
                run_proc -> current_state.name = CPU_RUNNING;
                run_proc -> current_state.resource_type = CPU;
                run_proc -> current_state.resource_idx = idx;

                cpu[idx] -> base -> items[0] = run_proc;
                cpu[idx] -> base -> current_process_count = 1;

                cpu[idx] -> remain_time_quantum = rq[idx] -> time_quantum == 0 ? -1 : rq[idx] -> time_quantum;

                for(int i = 0; i < rq[idx] -> base -> current_process_count - 1; i++){
                    rq[idx] -> base -> items[i] = rq[idx] -> base -> items[i + 1];
                }
                rq[idx] -> base -> items[rq[idx] -> base -> current_process_count - 1] = NULL;
                rq[idx] -> base -> current_process_count--;
            }
        }
    }
    else if (rq_s == SCP_MLQ || rq_s == SCP_MLFQ) {
        if (cpu[0] -> base -> current_process_count == 1){
            int tier = cpu[0] -> base -> items[0] -> current_state.current_priority / priority_interval_per_tier;
            int need_preemption = 0;
            
            for(int i = 0; i < tier; i++){
                if (rq[i] -> preemptive && rq[i] -> base -> current_process_count > 0) {
                    need_preemption = 1;
                    break;
                }
            }
            if (!need_preemption) {
                for(int i = tier; i < rq_count; i++){
                    if (rq[i] -> io_interrupted){
                        need_preemption = 1;
                        break;
                    }

                    if (i == tier){
                        if (rq[i] -> preemptive && rq[i] -> base -> current_process_count > 0 && rq[i] -> compare(&(cpu[0] -> base -> items[0]), &(rq[i] -> base -> items[0])) > 0) {
                            need_preemption = 1;
                            break;
                        }
                    }
                }
            }

            if (need_preemption){
                cpu[0] -> base -> items[0] -> from_cpu = 1;
                to_ready(cpu[0] -> base -> items[0], rq_s, -1);
                cpu[0] -> base -> items[0] = NULL;
                cpu[0] -> base -> current_process_count = 0;
            }
        }

        if (cpu[0] -> base -> current_process_count == 0){
            for(int i = 0; i < rq_count; i++){
                if (rq[i] -> base -> current_process_count > 0){
                    Process* run_proc = rq[i] -> base -> items[0];

                    run_proc -> current_state.current_arrival_time = current_time;
                    run_proc -> current_state.name = CPU_RUNNING;
                    run_proc -> current_state.resource_type = CPU;
                    run_proc -> current_state.resource_idx = 0;

                    cpu[0] -> base -> items[0] = run_proc;
                    cpu[0] -> base -> current_process_count = 1;

                    cpu[0] -> remain_time_quantum = rq[i] -> time_quantum == 0 ? -1 : rq[i] -> time_quantum;

                    for(int j = 0; j < rq[i] -> base -> current_process_count - 1; j++){
                        rq[i] -> base -> items[j] = rq[i] -> base -> items[j + 1];
                    }
                    rq[i] -> base -> items[rq[i] -> base -> current_process_count - 1] = NULL;
                    rq[i] -> base -> current_process_count--;

                    break;
                }
            }
        }
    }
    else{
        if (rq[0] -> io_interrupted){
            int victim_idx = -1;

            for(int i = 0; i < cpu_count; i++){
                if (cpu[i] -> base -> current_process_count == 0){
                    victim_idx = i;
                    break;
                }
            }

            if (victim_idx == -1){
                victim_idx = 0;
                for(int i = 1; i < cpu_count; i++){
                    if(rq[0] -> compare(&(cpu[victim_idx] -> base -> items[0]), &(cpu[i] -> base -> items[0])) > 0) {
                        victim_idx = i;
                    }
                }
            }
            if (cpu[victim_idx] -> base -> current_process_count == 1){
                cpu[victim_idx] -> base -> items[0] -> from_cpu = 1;
                to_ready(cpu[victim_idx] -> base -> items[0], rq_s, -1);
                cpu[victim_idx] -> base -> items[0] = NULL;
                cpu[victim_idx] -> base -> current_process_count = 0;
            }
        }

        // Find blank CPU and running
        for (int i = 0; i < cpu_count; i++){
            if (rq[0] -> base -> current_process_count == 0){
                break;
            }
            if (cpu[i] -> base -> current_process_count == 0){
                Process* run_proc = rq[0] -> base -> items[0];

                run_proc -> current_state.current_arrival_time = current_time;
                run_proc -> current_state.name = CPU_RUNNING;
                run_proc -> current_state.resource_type = CPU;
                run_proc -> current_state.resource_idx = i;

                cpu[i] -> base -> items[0] = run_proc;
                cpu[i] -> base -> current_process_count = 1;

                cpu[i] -> remain_time_quantum = rq[0] -> time_quantum == 0 ? -1 : rq[0] -> time_quantum;

                for(int j = 0; j < rq[0] -> base -> current_process_count - 1; j++){
                    rq[0] -> base -> items[j] = rq[0] -> base -> items[j + 1];
                }
                rq[0] -> base -> items[rq[0] -> base -> current_process_count - 1] = NULL;
                rq[0] -> base -> current_process_count--;
            }
        }
        // preemption
        if (rq[0] -> preemptive) {
            for(int i = 0; i < cpu_count; i++){
                if (rq[0] -> base -> current_process_count == 0){
                    break;
                }
                

                if (cpu[i] -> base -> current_process_count == 1){
                    if (rq[0] -> compare(&(cpu[i] -> base -> items[0]), &(rq[0] -> base -> items[0])) > 0) {
                        cpu[i] -> base -> items[0] -> from_cpu = 1;
                        to_ready(cpu[i] -> base -> items[0], rq_s, -1);
                        cpu[i] -> base -> items[0] = NULL;
                        cpu[i] -> base -> current_process_count = 0;

                        Process* run_proc = rq[0] -> base -> items[0];

                        run_proc -> current_state.current_arrival_time = current_time;
                        run_proc -> current_state.name = CPU_RUNNING;
                        run_proc -> current_state.resource_type = CPU;
                        run_proc -> current_state.resource_idx = i;

                        cpu[i] -> base -> items[0] = run_proc;
                        cpu[i] -> base -> current_process_count = 1;

                        cpu[i] -> remain_time_quantum = rq[0] -> time_quantum == 0 ? -1 : rq[0] -> time_quantum;

                        for(int j = 0; j < rq[0] -> base -> current_process_count - 1; j++){
                            rq[0] -> base -> items[j] = rq[0] -> base -> items[j + 1];
                        }
                        rq[0] -> base -> items[rq[0] -> base -> current_process_count - 1] = NULL;
                        rq[0] -> base -> current_process_count--;
                    }
                }
                
            }
        }
    }
}

void capture(){
    for(int i = 0; i < PCB_size; i++){
        proc[i] -> chart[current_time].name = proc[i] -> current_state.name;
        proc[i] -> chart[current_time].resource_type = proc[i] -> current_state.resource_type;
        proc[i] -> chart[current_time].resource_idx = proc[i] -> current_state.resource_idx;
        proc[i] -> chart[current_time].current_arrival_time = proc[i] -> current_state.current_arrival_time;
        proc[i] -> chart[current_time].current_burst_idx = proc[i] -> current_state.current_burst_idx;
        proc[i] -> chart[current_time].current_burst_time = proc[i] -> current_state.current_burst_time;
        proc[i] -> chart[current_time].current_priority = proc[i] -> current_state.current_priority;
    }

    for(int i = 0; i < cpu_count; i++){
        cpu[i] -> chart[current_time] = cpu[i] -> base -> current_process_count == 1 ? cpu[i] -> base -> items[0] -> p_info_ptr -> pid : -1;
    }

    for(int i = 0; i < io_wq_count; i++){
        io[i] -> chart[current_time] = io[i] -> base -> current_process_count == 1 ? io[i] -> base -> items[0] -> p_info_ptr -> pid : -1;
    }

    for(int i = 0; i < io_wq_count; i++){
        wq[i] -> process_length_at_time[current_time] = wq[i] -> base -> current_process_count;
        if (wq[i] -> base -> current_process_count > 0) {
            for(int j = 0; j < wq[i] -> base -> current_process_count; j++){
                wq[i] -> chart[current_time][j] = wq[i] -> base -> items[j] -> p_info_ptr -> pid;
            }
        }
    }

    for(int i = 0; i < rq_count; i++){
        rq[i] -> process_length_at_time[current_time] = rq[i] -> base -> current_process_count;
        if (rq[i] -> base -> current_process_count > 0) {
            for(int j = 0; j < rq[i] -> base -> current_process_count; j++){
                rq[i] -> chart[current_time][j] = rq[i] -> base -> items[j] -> p_info_ptr -> pid;
            }
        }
    }
}

void simulate(rq_situation rq_s){
    current_time = 0;
    while(current_time < MAX_TIME_LINE) {
        tick_flow(rq_s);

        scan_io_resource(rq_s);
        scan_cpu_resource(rq_s);
        scan_wq_resource();
        proc_generate(rq_s);
        scan_rq_resource(rq_s);

        capture();

        int all_terminated = 1;
        for(int i = 0; i < PCB_size; i++){
            if (proc[i] -> current_state.name != TERMINATED) {
                all_terminated = 0;
                break;
            }
        }

        if (all_terminated){
            printf("All Process are Termianted %d\n", current_time);
            break;
        }
        current_time++;
    }
}