#ifndef _SCHED_H
#define _SCHED_H

int thread_create(void *func);
void preempt_disable(void);
void preempt_enable(void);
void _schedule();
void schedule();

void idle();
void timer_tick();
void kill_zombies();

struct cpu_context {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long lr;
	unsigned long sp;
};

struct Thread {
	struct cpu_context cpu_context;
	int state;	
	int counter;
	int priority;
	int preempt_count;
	int pid;
	int magic;
};

typedef struct Thread Thread;

#define INIT_TASK \
/*cpu_context*/	{ {0,0,0,0,0,0,0,0,0,0,0,0,0}, \
/* state etc */	0,0,1,0,0,0 \
}
#define NR_TASKS 64
#define THREAD_SIZE 4096

#define TASK_RUNNING 0
#define TASK_FINISHED 1

#endif //_SCHED_H