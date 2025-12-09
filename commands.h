#ifndef COMMANDS_H
#define COMMANDS_H
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <stddef.h>
#include <errno.h>
#include "my_system_call.h"
#include <string.h>
#include <stdbool.h>

#define CMD_LENGTH_MAX 80
#define ARGS_NUM_MAX 20
#define JOBS_NUM_MAX 100

#define SYS_FORK     1
#define SYS_EXECVP   2
#define SYS_WAITPID  3
#define SYS_SIGNAL   4
#define SYS_KILL     5
#define SYS_PIPE     6
#define SYS_READ     7
#define SYS_WRITE    8
#define SYS_OPEN     9
#define SYS_CLOSE    10
#define DIFFERENT 1
#define IDENTICAL 0
#define EOF_READ 0
#define QUITVAL -2
#define ERROR -1
#define SIGSTP 20

#define  SIGSTOP 19
#define SIGCONT 18
/*=============================================================================
* error handling - some useful macros and examples of error handling,
* feel free to not use any of this
=============================================================================*/
#define ERROR_EXIT(msg) \
    do { \
        fprintf(stderr, "%s: %d\n%s", __FILE__, __LINE__, msg); \
        exit(1); \
    } while(0);

static inline void* _validatedMalloc(size_t size)
{
    void* ptr = malloc(size);
    if(!ptr) ERROR_EXIT("malloc");
    return ptr;
}

// example usage:
// char* bufffer = MALLOC_VALIDATED(char, MAX_LINE_SIZE);
// which automatically includes error handling
#define MALLOC_VALIDATED(type, size) \
    ((type*)_validatedMalloc((size)))






typedef struct cmd {
    char cmd[CMD_LENGTH_MAX];
    int nargs;
    char* args[ARGS_NUM_MAX];
    int bg; //1 - bg 0 - fg
    int internal ; // 1 internal  0 -external
    char cmd_full[CMD_LENGTH_MAX];
    //TODO maybe add a pointer to the cmd ??
} cmd ;
typedef struct job {
    int PID;
    int JOB_ID;
    char cmd[80];
    int state; // 1 fg  2 bg 3 stopped
    time_t time;
    char cmd_full[CMD_LENGTH_MAX];
} job ;

typedef struct list {
    cmd og_cmd_list[ARGS_NUM_MAX];
    char alias[80];
    struct list* next;
    struct list* prev;
} list;

/*=============================================================================
* error definitions
=============================================================================*/
typedef enum  {
    INVALID_COMMAND = 0,
    //feel free to add more values here or delete this
} ParsingError;

typedef enum {
    SMASH_SUCCESS = 0,
    SMASH_QUIT,
    SMASH_FAIL
    //feel free to add more values here or delete this
} CommandResult;

/*=============================================================================
* global functions
=============================================================================*/
cmd* parseCommandExample(char* line);
int command_selector(cmd cmd_after_parse);
void perrorSmash(const char* cmd, const char* msg);
int diff(cmd cmd_obj);
int fg(cmd cmd_obj);
int bg(cmd cmd_obj);
int quit(cmd cmd_obj);
int showpid(cmd cmd_obj);
int pwd(cmd cmd_obj);
int my_kill(cmd cmd_obj);
int cd (cmd cmd_obj);
int jobs(cmd cmd_obj);

int job_to_fg_pid;
cmd last_fg_cmd;

job jobs_list[JOBS_NUM_MAX];
int current_job_index;
cmd cmd_list[ARGS_NUM_MAX];
job job_to_be_stopped;
list* head_alias_list;
int smash_pid;
int curr_fg_pid;
#endif //COMMANDS_H
