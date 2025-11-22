// signals.c
#include "signals.h"
#include <stdio.h>

#define KILL 5
#define CTRLZ 20
#define CTRLC 2
#ifndef
#define JOB_STATE_FG 1
#define JOB_STATE_STP 3
#endif


struct sigaction
{
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*
            , void*);
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
}

int pid_to_sig= -1;
int job_id_to_sig = 0 ;
void sigintHandler(int sig){

//    CTRL Z = 20  CTRL C = 2
    for (int i = 0; i< 100 ; ++i) {
        if (jobs_list[i].state == JOB_STATE_FG){
            pid_to_sig=jobs_list[i].PID;
            job_id_to_sig=i;
            break;
        }
    }


    if (sig==CTRLZ) {    // we got CTRLZ

        printf("smash: caught CTRL+Z");
        if (pid_to_sig>=0){
            my_system_call(KILL,pid_to_sig,sig); // TODO add args
            jobs_list[job_id_to_sig].state=JOB_STATE_STP ;
            //TODO handle error!
            printf("process %d was stopped ",pid_to_sig);

        }


    }
    else if (sig==CTRLC){
        printf("smash: caught CTRL+C");
        if (pid_to_sig>=0) {
            my_system_call(KILL, pid_to_sig, sig); // TODO add args
            current_job_index = (current_job_index>job_id_to_sig) ? job_id_to_sig : current_job_index ;
            jobs_list[job_id_to_sig] = NULL;
            //TODO handle error!

            printf("process %d was killed ",pid_to_sig);
        }

    }

}
