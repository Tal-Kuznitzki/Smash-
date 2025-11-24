//smash.c

/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "signals.h"

#define JOB_STATE_FG 1
#define JOB_STATE_BG 2
#define JOB_STATE_STP 3


/*=============================================================================
* classes/structs declarations
=============================================================================*/
typedef struct job {
    int PID;
    int JOB_ID;
    char cmd[80];
    int state; // 1 fg  2 bg 3 stopped
} job ;
//TODO add structs to the H file!

typedef struct cmd {
    char cmd[80];
    char alias[80];
    //TODO maybe add a pointer to the cmd ??
} cmd ;




int command_selector(char[80] cmd_after_parse){
    char[80] cmd_args;
    switch (cmd_after_parse) {
        case cmd_DB[0].alias :
            showpid();
            break;
        case cmd_DB[1].alias :
            pwd();
            break;
        case cmd_DB[2].alias :
            cd();
            break;
        case cmd_DB[3].alias :
            jobs();
            break;
        case cmd_DB[4].alias :
            kill();
            break;
        case cmd_DB[5].alias :
            //TODO: add check at parser for #args, and pass -1 if no args
            fg();
            break;
        case cmd_DB[6].alias :
            bg();
            break;
        case cmd_DB[7].alias :
            quit();
            break;
        case cmd_DB[8].alias :
            diff();
            break;
        case cmd_DB[9].alias :
            alias();
            break;
        case cmd_DB[10].alias :
            unalias();
            break;
    }


}


/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[CMD_LENGTH_MAX];
job jobs_list[100];
current_job_index=0; // TODO update accordingly

cmd cmd_DB[11];

/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
    char _cmd[CMD_LENGTH_MAX];
    struct sigaction sa = { .sa_handler = &sigintHandler };
    sigaction(CTRLZ || CTRLC  , &sa, NULL);  //TODO WHERE THIS GOES?!
    while(1) {
        printf("smash > ");
        fgets(_line, CMD_LENGTH_MAX, stdin);
        strcpy(_cmd, _line);
        //execute command

        // TODO: input parser !!!
        /*
         *
         *
         */

        cmd_after_parse = 1 ;


        if () { //internal
            if(){ //internal-fg
                command_selector(cmd_after_parse);
            }
            else { //internal-bg
                int pid_internal_bg = my_system_call(1); // FORK
                if (pid_internal_bg==0){
                    command_selector(cmd_after_parse);
                    current_job_index = (current_job_index>bg_internal_job.JOB_ID) ? bg_internal_job.JOB_ID : current_job_index ;
                    jobs_list[bg_internal_job.JOB_ID] = NULL ;
                    exit(0);
                }
                else { //father process
                    job bg_internal_job;
                    bg_internal_job.JOB_ID = current_job_index ;
                    current_job_index++;
                    bg_internal_job.PID = pid_internal_bg;
                    bg_internal_job.cmd = cmd_after_parse;
                    bg_internal_job.state = JOB_STATE_BG ;

                    jobs_list[bg_internal_job.JOB_ID]= bg_internal_job ;

                }

            }


        }// if we are internal - from command list
        else //we are external
        {

            if(inbackground) // external-bg
            {
                int pid_bg = my_system_call(1); // FORK
                if (pid_bg == 0 ) //if son - run_program in a new proc
                {
                    my_system_call(2,cmd_after_parse);
                    //TODO ERROR CHAINING TO OUTSIDE
                    current_job_index = (current_job_index>bg_external_job.JOB_ID) ? bg_external_job.JOB_ID : current_job_index ;
                    jobs_list[bg_external_job.JOB_ID] = NULL ;
                    exit(0);
                }
                else // if father process
                {
                    job bg_external_job;
                    bg_external_job.JOB_ID = current_job_index ;
                    current_job_index++;
                    bg_external_job.PID = pid_bg;
                    bg_external_job.cmd = cmd_after_parse;
                    bg_external_job.state = JOB_STATE_BG ;

                    jobs_list[bg_external_job.JOB_ID]= bg_external_job ;

                }









            }

            else{ // external-fg
                int pid_fg = my_system_call(1); // FORK
                if (pid_fg == 0 ) //if son - run_program in a new proc
                {
                    my_system_call(2,cmd_after_parse);
                    //TODO ERROR CHAINING TO OUTSIDE
                    exit(0);
                }
                else // if father process
                {
                    job fg_external_job;
                    fg_external_job.JOB_ID = current_job_index ;
                    current_job_index++;
                    fg_external_job.PID = pid_fg;
                    fg_external_job.cmd = cmd_after_parse;
                    fg_external_job.state = JOB_STATE_FG ;

                    jobs_list[fg_external_job.JOB_ID]= fg_external_job ;
                    waitpid(pid_fg);
                    current_job_index = (current_job_index>fg_external_job.JOB_ID) ? fg_external_job.JOB_ID : current_job_index ;
                    jobs_list[fg_external_job.JOB_ID] = NULL ;
                }



            }




        }
















        //initialize buffers for next command
        _line[0] = '\0';
        _cmd[0] = '\0';
    }

    return 0;
}
