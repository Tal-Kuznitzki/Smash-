#ifndef SIGNALS_H
#define SIGNALS_H

#include "commands.h"
#include <signal.h>
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#define KILL 5
#define CTRLZ 20
#define CTRLC 2
/*=============================================================================
* global functions
=============================================================================*/


struct sigaction
{
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};

int pid_to_sig;
int job_id_to_sig;
job job_to_be_stopped;
void sigintHandler(int sig);



#endif //__SIGNALS_H__