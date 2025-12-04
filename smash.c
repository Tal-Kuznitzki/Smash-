//smash.c
/*=============================================================================
* includes, defines, usings
=============================================================================*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_system_call.h"
#include "commands.h"
#include "signals.h"

#define JOB_STATE_FG 1
#define JOB_STATE_BG 2
#define JOB_STATE_STP 3
#define QUITVAL -2


/*=============================================================================
* classes/structs declarations
=============================================================================*/





/*=============================================================================
* global variables & data structures
=============================================================================*/
char _line[CMD_LENGTH_MAX];
list* head_alias_list = NULL;


/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{

    char _cmd[CMD_LENGTH_MAX];
    struct sigaction sa = { .sa_handler = &sigintHandler };
     //sigaction(CTRLZ || CTRLC   , &sa, NULL);  //TODO WHERE THIS GOES?!
    job_to_fg_pid = -1;
    last_fg_cmd = NULL;
    int end_val=0;
    int external_fg_end_val=0;
    while(1) {
        printf("smash > ");
        fgets(_line, CMD_LENGTH_MAX, stdin);
        strcpy(_cmd, _line);
        //execute command
        cmd* cmd_list_after_parse = parseCommandExample(_cmd);
        int cmd_list_indx=0;
        int retVal_Cmd_tmp=0;
        job bg_internal_job;
        while (&cmd_list_after_parse[cmd_list_indx]!=NULL){// TODO IS OKAY?
             cmd cmd_after_parse=cmd_list_after_parse[cmd_list_indx];

            if (cmd_after_parse.internal) { //internal
                if(cmd_after_parse.bg == 0 ){ //internal-fg
                    last_fg_cmd = cmd_after_parse;
                    int output = command_selector(cmd_after_parse);
                    if (output == QUITVAL ){
                        end_val= QUITVAL;
                        break; //we end the program
                    } else if ( output == ERROR  ){
                        break;
                    }

                }
                else { //internal-bg
                    int pid_internal_bg = my_system_call(1); // FORK
                    if (pid_internal_bg==0){
                        setpgrp();
                        int output = command_selector(cmd_after_parse);  // 0 all good -1 error -2 quit
                        if (output == QUITVAL ){
                            end_val= QUITVAL;
                            break; //we end the program
                        } else if ( output == ERROR  ){
                            break;
                        }
                        current_job_index = (current_job_index>bg_internal_job.JOB_ID) ? bg_internal_job.JOB_ID : current_job_index ;
                        jobs_list[bg_internal_job.JOB_ID] = NULL ;
                    }
                    else { //father process
                        bg_internal_job.JOB_ID = current_job_index ;
                        current_job_index++;
                        bg_internal_job.PID = pid_internal_bg;
                        bg_internal_job.cmd = cmd_after_parse.cmd;
                        bg_internal_job.state = JOB_STATE_BG ;
                        jobs_list[bg_internal_job.JOB_ID]= bg_internal_job ;
                        time(&(bg_internal_job.time)) ;


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
                        time(&(bg_internal_job.time)) ;

                    }
                }

                else{ // external-fg
                    int pid_fg = my_system_call(SYS_FORK); // FORK
                    job_to_fg_pid = pid_fg;
                    last_fg_cmd = cmd_after_parse;
                    if (pid_fg == 0 ) //if son - run_program in a new proc
                    {
                        external_fg_end_val = my_system_call(SYS_EXECVP,cmd_after_parse);
                        //TODO ERROR CHAINING TO OUTSIDE
                        if (external_fg_end_val == ERROR ) exit(ERROR);
                        exit(0);
                    }
                    else // if father process
                    {
                        /*   job fg_external_job;
                        //   fg_external_job.JOB_ID = current_job_index ;
                        //  current_job_index++;
                        //   fg_external_job.PID = pid_fg;
                        // fg_external_job.cmd = cmd_after_parse;
                        //  fg_external_job.state = JOB_STATE_FG ;


                        jobs_list[fg_external_job.JOB_ID]= fg_external_job ;*/
                        my_system_call(SYS_WAITPID,pid_fg); //TODO verify
                        job_to_fg_pid = -1;
                        //  current_job_index = (current_job_index>fg_external_job.JOB_ID) ? fg_external_job.JOB_ID : current_job_index ;
                        //  jobs_list[fg_external_job.JOB_ID] = NULL ;
                        if (external_fg_end_val == ERROR ) break;
                    }

                }
            }
            cmd_list_indx++;
        }
        if (end_val == QUITVAL) break;
        //initialize buffers for next command
        _line[0] = '\0';
        _cmd[0] = '\0';
    }
    return 0;
}



//TODO:
// UPDATE PARSER TO LOOK AT ALIAS LIST
//

//

