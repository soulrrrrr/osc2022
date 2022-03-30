int strcmp(char *s1, char *s2);
int hex_to_int(char *p, int size);
void *get_user_program_address();
void shell_input(char *input);
void exception_entry();
void print_core_timer();
void strcpy(char *s1, char *s2);
unsigned long cstr_to_ulong(char *s);
void* simple_malloc(void **now, int size);
void sort_message(int size); // size-1

#define MESSAGE_QUEUE 0x100000
#define MESSAGE_INSERT 0x120000

struct message {
    char content[1024];
    unsigned long seconds;
};