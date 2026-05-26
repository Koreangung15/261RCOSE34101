#ifndef CMP_FUNC_H
#define CMP_FUNC_H

int p_info_compare(const void *a, const void *b);
int fcfs_cmp(const void *a, const void *b);
int rr_fcfs_cmp(const void *a, const void *b);
int sjf_cmp(const void *a, const void *b);
int rr_sjf_cmp(const void *a, const void *b);
int priority_cmp(const void *a, const void *b);
int rr_priority_cmp(const void *a, const void *b);

#endif