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
#define QUITVAL 2


/*=============================================================================
* classes/structs declarations
=============================================================================*/





/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[CMD_LENGTH_MAX];



/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
    char _cmd[CMD_LENGTH_MAX];
    struct sigaction sa = { .sa_handler = &sigintHandler };
    sigaction(CTRLZ || CTRLC   , &sa, NULL);  //TODO WHERE THIS GOES?!
    while(1) {
        printf("smash > ");
        fgets(_line, CMD_LENGTH_MAX, stdin);
        strcpy(_cmd, _line);
        //execute command
        cmd cmd_after_parse = parseCmdExample(_cmd);

        if (cmd_after_parse.internal) { //internal
            if(cmd_after_parse.bg == 0 ){ //internal-fg
                int output = command_selector(cmd_after_parse);
                if (output == QUITVAL ){
                    break; //we end the program
                }
            }
            else { //internal-bg
                int pid_internal_bg = my_system_call(1); // FORK
                if (pid_internal_bg==0){
                    setpgrp();
                    command_selector(cmd_after_parse);
                    current_job_index = (current_job_index>bg_internal_job.JOB_ID) ? bg_internal_job.JOB_ID : current_job_index ;
                    jobs_list[bg_internal_job.JOB_ID] = NULL ;
                }
                else { //father process
                    job bg_internal_job;
                    bg_internal_job.JOB_ID = current_job_index ;
                    current_job_index++;
                    bg_internal_job.PID = pid_internal_bg;
                    bg_internal_job.cmd = cmd_after_parse;
                    bg_internal_job.state = JOB_STATE_BG ;
                    jobs_list[bg_internal_job.JOB_ID]= bg_internal_job ;
                    bg_internal_job.time = time() ;


                }

            }


        }// if we are internal - from command list
        else //we are external
        {

            if(cmd_after_parse.bg) // external-bg
            {
                int pid_bg = my_system_call(1); // FORK
                if (pid_bg == 0 ) //if son - run_program in a new proc
                {
                    setpgrp();
                    job bg_external_job;
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
                    bg_internal_job.time = time() ;

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
                    my_system_call(SYS_WAITPID,pid_fg); //TODO verify
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
