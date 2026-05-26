#ifndef SIMULATION_H
#define SIMULATION_H


extern int current_time;

int time_quantum_expired(CPU_resource* cpu_ptr);
int terminated(Process* proc_ptr);
int burst_expired(Process* proc_ptr);

void insert_new_process(Process* proc_ptr, Ready_Queue* rq_ptr);
void to_ready(Process* proc_ptr, rq_situation rq_s, int flag);

void tick_flow(rq_situation rq_s);
void scan_cpu_resource(rq_situation rq_s);
void scan_io_resource(rq_situation rq_s);
void scan_wq_resource();
void proc_generate(rq_situation rq_s);
void capture();

void simulate(rq_situation rq_s);

#endif