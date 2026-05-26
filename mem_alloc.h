#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

void cleanup();
void PCB_cleanup();
void proc_cleanup();
void cpu_cleanup();
void io_cleanup();
void rq_cleanup();
void wq_cleanup();

void PCB_allocation();
void proc_allocation();
void cpu_allocation();
void io_allocation();
void rq_allocation();
void wq_allocation();

#endif