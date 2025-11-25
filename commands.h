#ifndef COMMANDS_H
#define COMMANDS_H
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <stdio.h>

#define CMD_LENGTH_MAX 120
#define ARGS_NUM_MAX 20
#define JOBS_NUM_MAX 100

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




typedef struct job {
    int PID;
    int JOB_ID;
    char cmd[80];
    int state; // 1 fg  2 bg 3 stopped
    int time;
} job ;
//TODO add structs to the H file!

typedef struct cmd {
    char* cmd[80];
    int nargs;
    char* args[ARGS_NUM_MAX]={0};
    int bg; //1 - bg 0 - fg
    int internal=0 ; // 1 internal  0 -external
    //TODO maybe add a pointer to the cmd ??
} cmd ;

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
typedef struct job {
    int PID;
    int JOB_ID;
    char cmd[80];
    int state; // 1 fg, 2 bg, 3 stopped
} job;

cmd parseCommandExample(char* line);
int command_selector(cmd cmd_after_parse);
void perrorSmash(const char* cmd, const char* msg);
int diff(char* args[MAX_ARGS],int nargs);
int  fg(job *jobs, int job_id);
int quit(int nargs ,char* arg);
int showpid(cmd cmd_obj);
int pwd(cmd cmd_obj);
int kill(cmd cmd_obj, int signum, int job_id);
int cd (cmd cmd_obj, char* path);
int jobs(cmd cmd_obj);





#endif //COMMANDS_H