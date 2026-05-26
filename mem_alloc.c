#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "mem_alloc.h"

void PCB_cleanup() {
    for (int i = 0; i < MAX_PROCESS_COUNT; i++) {
        if (PCB[i] != NULL) {
            free(PCB[i]);
            PCB[i] = NULL;
        }
    }
}

void proc_cleanup() {
    for (int i = 0; i < MAX_PROCESS_COUNT; i++){
        if (proc[i] != NULL){
            free(proc[i]);
            proc[i] = NULL;
        }
    }
}

void cpu_cleanup() {
    for (int i = 0; i < MAX_CPU_COUNT; i++){
        if (cpu[i] != NULL) {
            if (cpu[i] -> base != NULL) {
                free(cpu[i] -> base);
            }
            free(cpu[i]);
            cpu[i] = NULL;
        }
    }
}

void io_cleanup() {
    for (int i = 0; i < MAX_IO_WQ_COUNT; i++){
        if (io[i] != NULL) {
            if (io[i] -> base != NULL) {
                free(io[i] -> base);
            }
            free(io[i]);
            io[i] = NULL;
        }
    }
}
void rq_cleanup() {
    for (int i = 0; i < MAX_RQ_COUNT; i++){
        if (rq[i] != NULL) {
            if (rq[i] -> base != NULL) {
                free(rq[i] -> base);
            }
            free(rq[i]);
            rq[i] = NULL;
        }
    }
}
void wq_cleanup() {
    for (int i = 0; i < MAX_RQ_COUNT; i++){
        if (wq[i] != NULL) {
            if (wq[i] -> base != NULL) {
                free(wq[i] -> base);
            }
            free(wq[i]);
            wq[i] = NULL;
        }
    }
}

void cleanup(){
    PCB_cleanup();
    proc_cleanup();
    cpu_cleanup();
    io_cleanup();
    rq_cleanup();
    wq_cleanup();
}

void PCB_allocation() {
    for(int i = 0; i < MAX_PROCESS_COUNT; i++){
        if (PCB[i] != NULL) free(PCB[i]); // 안전을 위한 중복 할당 방지
        PCB[i] = (Process_info*) malloc(sizeof(Process_info));
        if (PCB[i] == NULL) {
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();
            exit(1);
        }
    }
}

void proc_allocation() {
    for(int i = 0; i < MAX_PROCESS_COUNT; i++){
        if (proc[i] != NULL) free(proc[i]);
        proc[i] = (Process*) malloc(sizeof(Process));

        if (proc[i] == NULL) {
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();

            exit(1);
        }
    }
}

void cpu_allocation() {
    for (int i = 0; i < MAX_CPU_COUNT; i++){
        cpu[i] = (CPU_resource*) malloc(sizeof(CPU_resource));
        if (cpu[i] == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();

            exit(1);
        }
        cpu[i] -> base = (Resource*) malloc(sizeof(Resource));
        if (cpu[i] -> base == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();
            
            exit(1);
        }
    }
}

void io_allocation() {
    for (int i = 0; i < MAX_IO_WQ_COUNT; i++){
        io[i] = (IO_resource*) malloc(sizeof(IO_resource));
        if (io[i] == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();

            exit(1);
        }
        io[i] -> base = (Resource*) malloc(sizeof(Resource));
        if (io[i] -> base == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();
            
            exit(1);
        }
    }
}

void rq_allocation() {
    for (int i = 0; i < MAX_RQ_COUNT; i++){
        rq[i] = (Ready_Queue*) malloc(sizeof(Ready_Queue));
        if (rq[i] == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();

            exit(1);
        }
        rq[i] -> base = (Resource*) malloc(sizeof(Resource));
        if (rq[i] -> base == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();
            
            exit(1);
        }
    }
}

void wq_allocation() {
    for (int i = 0; i < MAX_IO_WQ_COUNT; i++){
        wq[i] = (Waiting_Queue*) malloc(sizeof(Waiting_Queue));
        if (wq[i] == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();

            exit(1);
        }
        wq[i] -> base = (Resource*) malloc(sizeof(Resource));
        if (wq[i] -> base == NULL){
            fprintf(stderr, "Memory Allocation failed \n");
            cleanup();
            
            exit(1);
        }
    }
}