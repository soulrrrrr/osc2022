#include "sched.h"
#include "memory.h"
#include "utils.h"
#include "printf.h"

static Thread init_task = INIT_TASK;
//Thread *current_thread = &(init_task);
Thread *task[NR_TASKS] = {&(init_task), };
int nr_tasks = 1;


extern void run_thread(void);
extern Thread *get_current(void);
extern void enable_irq();
extern void disable_irq();
extern switch_to(void *, void *);


int get_new_pid() {
	Thread* p;
	for (int i = 0; i < NR_TASKS; i++) {
		p = task[i];
		if (p == NULL) {
			return i;
		}	
	}
	return -1;
}

Thread* current_thread() {
	Thread *cur = get_current();
	if (!cur)
		return &init_task;
	return cur;
}

void preempt_disable(void) {
	current_thread()->preempt_count++;
}

void preempt_enable(void) {
	current_thread()->preempt_count--;
}

int thread_create(void *func) {
    Thread *p = malloc(sizeof(Thread));
	printf("thread_create %x\n", p);
    p->priority = 1;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
	p->status = 0;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.x19 = (ulong)func;
    p->cpu_context.lr = (ulong)run_thread;
    //p->cpu_context.sp = (ulong)p + THREAD_SIZE - 16;
	p->kernel_sp = (ulong)malloc(PAGE_SIZE) + PAGE_SIZE - 16;
	p->user_sp = (ulong)malloc(PAGE_SIZE) + PAGE_SIZE - 16;
    p->cpu_context.sp = p->kernel_sp; // kernel space

    int pid = get_new_pid();
    task[pid] = p;
	p->pid = pid;
    preempt_enable();
    return pid;
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
	//debug("now thread", current_thread());
	//debug("next thread", task[next]);
    if (current_thread() != task[next]) {
		//printf("[scheduler] next pid: %d\n", next);
		Thread *prev = current_thread();
		//current_thread = task[next];
	    switch_to(prev, task[next]);
	}
	preempt_enable();
}

void schedule() {
	current_thread()->counter = 0;
	_schedule();
}

void kill_zombies() {
	Thread* p;
	for (int i = 1; i < NR_TASKS; i++) { // pick biggest c value
		p = task[i];
		if (p && p->state == TASK_ZOMBIE) {
			free((void *)(p->kernel_sp & ~(PAGE_SIZE-1)));
			free((void *)(p->user_sp & ~(PAGE_SIZE-1)));
			free(p);
			task[i] = NULL;
		}
	}
}

void idle() {
    while(1) {
        kill_zombies(); // reclaim threads marked as DEAD
        schedule(); // switch to any other runnable thread
    }
}


void timer_tick() {
    current_thread()->counter--;
    if (current_thread()->counter>0 || current_thread()->preempt_count > 0) {
		return;
	}
	current_thread()->counter=0;
	enable_irq();
	schedule();
	disable_irq();
}

void end_thread(void) {
	current_thread()->state = TASK_ZOMBIE;
	schedule();
}

void task_init(void) {
	//Thread *p = current_thread();
	//asm volatile ("mrs %0, sp_el1":"=r"(p->kernel_sp));
	uint64_t init_task_addr = (uint64_t)&init_task;
	asm volatile ("msr tpidr_el1, %0"::"r"(init_task_addr));
	uint64_t sp_el0 = 0;
	asm volatile ("msr sp_el0, %0"::"r"(sp_el0));
}