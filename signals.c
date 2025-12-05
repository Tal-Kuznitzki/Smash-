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
    pid_to_sig=job_to_fg_pid ;
    if (sig==CTRLZ) {    // we got CTRLZ
        perrorSmash("smash","caught CTRL+Z");
        if (pid_to_sig>0){
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
            sprintf(msg,"process %d was stopped ",pid_to_sig);
            perrorSmash("smash",msg);
        }
    }
    else if (sig==CTRLC){
        perrorSmash("smash","caught CTRL+C");
        if (pid_to_sig>=0) {
            my_system_call(SYS_KILL, pid_to_sig, SIGKILL);
            //current_job_index = (current_job_index>job_id_to_sig) ? job_id_to_sig : current_job_index ;
            // jobs_list[job_id_to_sig] = NULL;
            char msg[CMD_LENGTH_MAX];
            sprintf(msg,"process %d was killed ",pid_to_sig);
            perrorSmash("smash",msg);
        }
        else{
            perrorSmash("smash","ctrl-C failed"); //TODO verify if need
        }

    }

}
