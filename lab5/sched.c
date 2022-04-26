#include "sched.h"
#include "memory.h"
#include "utils.h"

static Thread init_task = INIT_TASK;
Thread *current_thread = &(init_task);
Thread *task[NR_TASKS] = {&(init_task), };
int nr_tasks = 1;

extern void run_thread(void);

void preempt_disable(void) {
	current_thread->preempt_count++;
}

void preempt_enable(void) {
	current_thread->preempt_count--;
}

int thread_create(void *func) {
    Thread *p = malloc(PAGE_SIZE);
	debug("thread_create", p);
    p->priority = 1;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.x19 = func;
    p->cpu_context.lr = (unsigned long)run_thread;
    p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;

    int pid = nr_tasks++;
    task[pid] = p;
	p->pid = pid;
    preempt_enable();
    return 0;
}

void _schedule() {
	preempt_disable();
	int next, c;
	struct Thread* p;
	while (1) {
		c = -1;
		next = 0;
		for (int i = 0; i < NR_TASKS; i++) { // pick biggest c value
			p = task[i];
			if (p && p->state == TASK_RUNNING && p->counter > c) {
				c = p->counter;
				next = i;
			}
		}
		if (c) {
			break;
		}
		for (int i = 0; i < NR_TASKS; i++) {
			p = task[i];
			if (p) {
				p->counter = (p->counter >> 1) + p->priority;
			}
		}
	}
    if (current_thread != task[next]) {
		debug("next pid", next);
		Thread *prev = current_thread;
		current_thread = task[next];
	    switch_to(prev, task[next]);
	}
	preempt_enable();
}

void schedule() {
	current_thread->counter = 0;
	_schedule();
}

void kill_zombies() {

}

void idle() {
    while(1) {
        //kill_zombies(); // reclaim threads marked as DEAD
        schedule(); // switch to any other runnable thread
    }
}

extern void enable_irq();
extern void disable_irq();

void timer_tick() {
    current_thread->counter--;
    if (current_thread->counter>0 || current_thread->preempt_count > 0) {
		return;
	}
	current_thread->counter=0;
	enable_irq();
	schedule();
	disable_irq();
}

void end_thread(void) {
	current_thread->state = TASK_FINISHED;
	schedule();
}