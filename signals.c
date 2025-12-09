// signals.c
#include "signals.h"
#include "commands.h"
#include "my_system_call.h"
#include <stdio.h>
#include <time.h>
#define JOB_STATE_FG 1
#define JOB_STATE_STP 3
#define SIGCHLD 17




int pid_to_sig= -1;
int job_id_to_sig = 0 ;
void sigintHandler(int sig){

//    CTRL Z = 20  CTRL C = 2 SIGCHILD=17
    if (sig == SIGCHLD){
       //TODO fix  my_system_call(SYS_WAITPID,)
    }
/*    for (int i = 0; i< 100 ; ++i) {
        if (jobs_list[i].state == JOB_STATE_FG){
            pid_to_sig=jobs_list[i].PID;
            job_id_to_sig=i;
            break;
        }
    }*/
    pid_to_sig = (job_to_fg_pid==ERROR) ? smash_pid : job_to_fg_pid ;
    if ( sig==CTRLZ && (pid_to_sig!=smash_pid) ) {    // we got CTRLZ
        //perrorSmash("smash","caught CTRL+Z");
        printf("smash: caught CTRL+Z\n");
        if ( ( pid_to_sig>=0 )  ) {
            printf("@PID TO SEND SIG TO: %d\n",pid_to_sig);
            printf("@SIG TO SEND SIG TO: %d\n",sig);
            my_system_call(KILL,pid_to_sig,sig); // TODO add args
            if (current_job_index<JOBS_NUM_MAX){
                strcpy(job_to_be_stopped.cmd,last_fg_cmd.cmd);
                job_to_be_stopped.PID = pid_to_sig;
                job_to_be_stopped.JOB_ID = current_job_index;
                job_to_be_stopped.state = JOB_STATE_STP;
                job_to_be_stopped.time = time(NULL);
                jobs_list[current_job_index] = job_to_be_stopped;
                current_job_index++;
            }
            jobs_list[job_id_to_sig].state=JOB_STATE_STP ;
            //TODO handle error!
            char msg[CMD_LENGTH_MAX];
            sprintf(msg,"process %d was stopped",pid_to_sig);
            printf("%s\n",msg);
        }
    }




    else if (sig==CTRLC){
        printf("smash: caught CTRL+C\n");
        if (pid_to_sig>=0) {
            //current_job_index = (current_job_index>job_id_to_sig) ? job_id_to_sig : current_job_index ;
            // jobs_list[job_id_to_sig] = NULL;
            char msg[CMD_LENGTH_MAX];
            sprintf(msg,"process %d was killed",pid_to_sig);
            //perrorSmash("smash",msg);
            printf("smash: %s\n",msg);
            my_system_call(SYS_KILL, pid_to_sig, SIGKILL);
        }
        else{
            perrorSmash("smash","ctrl-C failed"); //TODO verify if need
        }

    }
    return;

}
void sigchldHandler(int sig) {
    int pid_val;
    int status;
    int options = 1;
    // Use a loop to ensure ALL terminated children are reaped.
    // The loop continues as long as the custom waitpid call successfully reaps a child (returns PID > 0).
    while ( (pid_val = my_system_call(SYS_WAITPID, -1, &status, options)) > 0 ) {
        // 1. Process the reaped child (Cleanup the jobs_list)
        for (int i = 0; i < JOBS_NUM_MAX; i++) {
            if (jobs_list[i].PID == pid_val) {
                // Clear the job entry, marking the slot as available.
                jobs_list[i].PID = ERROR; // Or another defined marker for 'empty'
                // You might also want to set job_list[i].state = 0 or remove time/command info.
                break;
            }
        }
    }

    // Check why the loop terminated.
    // An exit value of -1 with errno == ECHILD is the normal, expected termination
    // when all zombies are cleared.
    if (pid_val == ERROR && errno != ECHILD) {
        // Handle unexpected errors (using safe I/O if possible, or deferring).
        // Since we are assuming a smash-like environment, a direct print to stderr
        // might be acceptable, but is technically unsafe in a strict signal context.
    }
return;
}

