//smash.c
/*=============================================================================
* includes, defines, usings
=============================================================================*/

#define _XOPEN_SOURCE 700//que?
#include <stdlib.h>
#include <string.h>
#include "my_system_call.h"
#include "commands.h"
#include "signals.h"
#include <unistd.h>

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
/*=============================================================================
* main function
=============================================================================*/
int main(int argc, char* argv[])
{
    int current_job_index=0;
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &sigintHandler ;
    sa.sa_flags = SA_RESTART;
    sigaction(CTRLZ, &sa, NULL);  //TODO WHERE THIS GOES?!
    sigaction(CTRLC, &sa, NULL);  //TODO WHERE THIS GOES?!


    job empty_job;
    empty_job.PID = -1;
    empty_job.JOB_ID = -1;
    strcpy(empty_job.cmd, "");
    empty_job.state = -1;
    empty_job.time = -1;


    for (int i = 0; i <JOBS_NUM_MAX ; ++i) {
        jobs_list[i]=empty_job;
    }



    char _cmd[CMD_LENGTH_MAX];


    job_to_fg_pid = -1;
    //some basic setting, NULL-like
    cmd empty_cmd;
    empty_cmd.bg = -1 ;
    empty_cmd.nargs = -1;
    empty_cmd.internal = -1;
    empty_cmd.args[0] = NULL;
    strcpy(empty_cmd.cmd, "");

    last_fg_cmd.bg = -1 ;
    last_fg_cmd.nargs = -1;
    last_fg_cmd.internal = -1;
    last_fg_cmd.args[0]=NULL;
    strcpy(last_fg_cmd.cmd, "");

    int end_val=0;
    int external_fg_end_val=0;
    smash_pid =  getpid();
    while(1) {
        printf("smash > ");
        if (fgets(_line, CMD_LENGTH_MAX, stdin)==NULL ){
            break;
        }
        strcpy(_cmd, _line);
        //execute command
        cmd* cmd_list_after_parse = parseCommandExample(_cmd);
        int cmd_list_indx=0;
        //int retVal_Cmd_tmp=0;
        job bg_internal_job;
        //printf("@@@@@@@ %d @@@@@@@@@\n",cmd_list_after_parse[cmd_list_indx].bg);
        while ( ( cmd_list_after_parse!=NULL )  && ( (cmd_list_after_parse[cmd_list_indx].args[0]!=NULL) && (cmd_list_after_parse[cmd_list_indx].bg != ERROR) )  ){
            // printf("@@ bg val of cmd is : %d\n",cmd_list_after_parse[cmd_list_indx].bg);

            cmd cmd_after_parse=cmd_list_after_parse[cmd_list_indx];
            //    printf("cmd_after_parse.internal %d \n", cmd_after_parse.internal);
            if (cmd_after_parse.internal) { //internal
                //       printf("we are internal\n");
                if(cmd_after_parse.bg == 0 ){ //internal-fg
                    last_fg_cmd = cmd_after_parse;
                    //         printf("before selector\n");
                    int output = command_selector(cmd_after_parse);
                    //        printf("after selector\n");
                    if (output == QUITVAL ){
                        end_val= QUITVAL;
                        break; //we end the program
                    } else if ( output == ERROR  ){
                        while(cmd_list_after_parse[cmd_list_indx].bg!=ERROR) {
                            cmd_list_after_parse[cmd_list_indx].bg=ERROR;
                            cmd_list_indx++;
                        }
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
                        jobs_list[bg_internal_job.JOB_ID].PID = ERROR ;
                    }
                    else { //father process
                        bg_internal_job.JOB_ID = current_job_index ;
                        current_job_index++;
                        smash_pid = pid_internal_bg;
                        bg_internal_job.PID = pid_internal_bg;
                        strcpy(bg_internal_job.cmd,cmd_after_parse.cmd);
                        strcpy(bg_internal_job.cmd_full, cmd_after_parse.cmd_full);
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
                    int pid_bg = my_system_call(SYS_FORK); // FORK
                    if (pid_bg == 0 ) //if son - run_program in a new proc
                    {
                        setpgrp();
                        job bg_external_job;
                        int external_bg_end_val = my_system_call(SYS_EXECVP,cmd_after_parse.cmd,cmd_after_parse.args);
                        if (external_bg_end_val == ERROR ){
                            char msg[CMD_LENGTH_MAX];
                            if (errno == ENOENT ){ // if errno indicated cannot find program
                                sprintf(msg,"cannot find program");
                            }
                            else {
                                sprintf(msg,"invalid command");
                            }
                            perrorSmash(" external",msg);
                            exit(ERROR);
                        }

                        current_job_index = (current_job_index>bg_external_job.JOB_ID) ? bg_external_job.JOB_ID : current_job_index ;
                        jobs_list[bg_external_job.JOB_ID].PID = ERROR ;
                        exit(0);
                    }
                    else // if father process
                    {
                        job bg_external_job;
                        bg_external_job.JOB_ID = current_job_index ;
                        current_job_index++;
                        bg_external_job.PID = pid_bg;
                        strcpy(bg_external_job.cmd,cmd_after_parse.cmd);
                        strcpy(bg_external_job.cmd_full, cmd_after_parse.cmd_full);
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
                        setpgrp();
                        int dev_null = my_system_call(SYS_OPEN,"/dev/null", 1);
                        if (dev_null >= 0) {
                            dup2(dev_null, STDERR_FILENO); // Redirect stderr to /dev/null
                            close(dev_null);
                        }
                        external_fg_end_val = my_system_call(SYS_EXECVP,cmd_after_parse.cmd,cmd_after_parse.args);
                        if (external_fg_end_val == ERROR ){
                            char msg[CMD_LENGTH_MAX];
                            if (errno == ENOENT ){ // if errno indicated cannot find program
                                sprintf(msg,"cannot find program");
                            }
                            else {
                                sprintf(msg,"invalid command");
                            }
                            perrorSmash(" external",msg);
                        }
                        exit(0);
                    }
                    else // if father process
                    {
                        int wait_ret; int wait_status;
                        int EINTR_VAL = 4; // Assuming EINTR is 4, check your system's <errno.h> or use my_system_call(SYS_KILL, 0, 0) to check errno
                        do {
                            // You MUST use WUNTRACED (2) to return when the child is stopped.
                            wait_ret = my_system_call(SYS_WAITPID, pid_fg, &wait_status, 2);
                        } while (wait_ret == ERROR && errno == EINTR_VAL); // Assuming errno is accessible and EINTR is defined

                        if (wait_ret > 0) {
                            // 1. Check if the process was stopped (equivalent to WIFSTOPPED(wait_status))
                            if (((wait_status & 0xFF) == 0x7f) && ((wait_status >> 8) == SIGTSTP))
                            {
                                // The process was stopped by SIGTSTP/SIGSTOP.
                                // The signal handler should have already sent SIGSTOP and printed the messages.
                                // All you need to do here is reset the foreground job tracking.
                                job_to_fg_pid = -1; // Smash regains control

                                // The job is now in the jobs list (if the signal handler successfully updated it).
                                // Since the signal handler is unsafe, you might need to check the job list here.
                                // It's safer to have the signal handler *only* set a volatile flag.

                            }
                                // 2. Check if the process terminated (equivalent to WIFEXITED or WIFSIGNALED)
                            else if ((wait_status & 0x7f) == 0) {

                                // Extract exit code (equivalent to WEXITSTATUS)
                                // Shift right by 8 bits to get the actual return value (e.g., from return X or exit(X))
                                int exit_code = (wait_status & 0xff00) >> 8;

                                if (exit_code != 0) {
                                    // The command ran but failed (e.g., ls -9 returned exit code 2).
                                    // This is where you would stop a "&&" chain if you were implementing it here.
                                    perrorSmash(" external","invalid command");
                                    break;
                                }

                                job_to_fg_pid = -1; // Smash regains control
                            }
                                // 3. Process terminated by a signal (crashed/killed)
                            else {
                                job_to_fg_pid = -1;
                            }
                        }


                        /*   job fg_external_job;
                        //   fg_external_job.JOB_ID = current_job_index ;
                        //  current_job_index++;
                        //   fg_external_job.PID = pid_fg;
                        // fg_external_job.cmd = cmd_after_parse;
                        //  fg_external_job.state = JOB_STATE_FG ;


                        jobs_list[fg_external_job.JOB_ID]= fg_external_job ;*/
                        // my_system_call(SYS_WAITPID,pid_fg); //TODO verify
                        // job_to_fg_pid = -1; // TODO verify
                        //  current_job_index = (current_job_index>fg_external_job.JOB_ID) ? fg_external_job.JOB_ID : current_job_index ;
                        //  jobs_list[fg_external_job.JOB_ID] = NULL ;
                        if (external_fg_end_val == ERROR ) break;
                    }

                }
            }
            // if we got here it means we are done processing the privious command
            // TODO: check with tal about jobs in the bg!!
            cmd_list_after_parse[cmd_list_indx].bg = ERROR; //delete it from the list because it's a global list!
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


