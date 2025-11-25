//commands.c
#ifndef
#include "commands.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>    
#include <unistd.h>


MAX_ERROR_LEN = 30;

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
#define EOF 0



#define O_RDONLY 0
//ERRORs
#define NOARGSVAL -1
#define ERROR -1
#define EISDIR 21

#define JOB_STATE_FG 1
#define JOB_STATE_BG 2
#define JOB_STATE_STP 3


#endif

cmd cmd_obj; // global cmd object to use in all methods
char old_cd[CMD_LENGTH_MAX] = {0};
char current_cd[CMD_LENGTH_MAX] = {0};

//example function for printing errors from internal commands
void perrorSmash(const char* cmd, const char* msg)
{
    fprintf(stderr, "smash error:%s%s%s\n",
            cmd ? cmd : "",
            cmd ? ": " : "",
            msg);
}

//example function for parsing commands
int parseCmdExample(char* line)
{
    char* delimiters = " \t\n"; //parsing should be done by spaces, tabs or newlines
    char* cmd = strtok(line, delimiters); //read strtok documentation - parses string by delimiters
    if(!cmd)
        return INVALID_COMMAND; //this means no tokens were found, most like since command is invalid

    char* args[MAX_ARGS];
    int nargs = 0;
    args[0] = cmd; //first token before spaces/tabs/newlines should be command name
    for(int i = 1; i < MAX_ARGS; i++)
    {
        args[i] = strtok(NULL, delimiters); //first arg NULL -> keep tokenizing from previous call
        if(!args[i])
            break;
        nargs++;
    }
    /*
    At this point cmd contains the command string and the args array contains
    the arguments. You can return them via struct/class, for example in C:
        typedef struct {
            char* cmd;
            char* args[MAX_ARGS];
        } Command;
    Or maybe something more like this:
        typedef struct {
            bool bg;
            char** args;
            int nargs;
        } CmdArgs;
    */

    //Tal note: I assume we will return a struct  (command ) with the cmd, args, nargs and bool bg
}
/*
fg
bg
quit

Alias
Unalias
 */



int diff(char* args[MAX_ARGS],int nargs){
    if (nargs!=2){
        perrorSmash("diff","expected 2 arguments");
        return -1 ; // TODO :VERIFY WHAT RETVAL SHOULD BE ON ERROR
    }
    path1 = args[0];
    path2 = args[1];

    //opening both files;
    fd1 = my_system_call(SYS_OPEN,path1,O_RDONLY);
    fd2 = my_system_call(SYS_OPEN,path2,O_RDONLY);

    if ( ( fd1 == ERROR &&  errno==ENOENT ) || ( fd2 == ERROR &&  errno==ENOENT ) ){
        perrorSmash("diff","expected valid paths for files");
        return -1 ; // TODO :VERIFY WHAT RETVAL SHOULD BE ON ERROR
    }
    const size_t CHUNK_SIZE = 4096;
    char buffer1[CHUNK_SIZE];
    char buffer2[CHUNK_SIZE];
    ssize_t bytes_read1, bytes_read2;

    while (1) {
        bytes_read1 = my_system_call(SYS_READ, fd1, buffer1, CHUNK_SIZE);
        bytes_read2 = my_system_call(SYS_READ, fd2, buffer2, CHUNK_SIZE);

        if ( ( bytes_read1 == ERROR &&  errno==EISDIR ) || ( bytes_read2 == ERROR &&  errno==EISDIR ) ){
            perrorSmash("diff","paths are not files");
            return -1 ;// TODO :VERIFY WHAT RETVAL SHOULD BE ON ERROR
        }
        if (bytes_read1 != bytes_read2) { //different sizes, so different ofc
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            return DIFFERENT;
        }
        if ( memcmp(buffer1, buffer2, bytes_read1) != IDENTICAL ) { //chunks are not identical
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            return DIFFERENT;
        }
        if ( ( bytes_read1 == EOF && bytes_read2 != EOF) || bytes_read2 == EOF && bytes_read1 != EOF) ) { //ended reading one file but not the other.
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            return DIFFERENT;
        }
        if (bytes_read1 == EOF && bytes_read2 == EOF) { //ended reading both files no errors so far
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            return IDENTICAL;
        }

    }
}

int  fg(job *jobs, int job_id) {

    int job_idx_in_jobs = -1;
    if (job_id == NOARGSVAL ){
        int maxId = jobs[0].JOB_ID;
        for (int i = 1; i < size; i++) {
            if (jobs[i].JOB_ID > maxId) {
                maxId = jobs[i].JOB_ID;
                job_idx_in_jobs = i;
            }
        }
    }
    else{
        for (int j = 0; j < JOBS_NUM_MAX ; ++j) {
            if (jobs[j].JOB_ID == job_id ){
                job_idx_in_jobs = j;
                break;
            }
        }

    }

    //now we know job_idx_in_jobs
    //TODO: verify print format !
    print("%s %d", jobs[job_idx_in_jobs].cmd, jobs[job_idx_in_jobs].PID);

    if (jobs[job_idx_in_jobs].state == JOB_STATE_STP){ // if stopped, send it SIGCONT
        jobs[job_idx_in_jobs].state == JOB_STATE_FG;
        my_system_call(SYS_KILL,jobs[job_idx_in_jobs].PID,SIGCONT);
        print("%s", jobs[job_idx_in_jobs].cmd);

    }
    else{
        jobs[job_idx_in_jobs].state == JOB_STATE_FG;

    }







}

int showpid(cmd cmd_obj) {
	parseCmdExample();
	if (cmd_obj.nargs != 0) {
		perrorSmash(const char* "showpid", const char* "expected 0 arguments");
		// fprintf(stderr, "smash error: showpid: expected 0 argument");
		return -1;
	}
	else {
		pid_t smash_pid = getpid();
		printf ("smash pid is %ld\n", (long) smash_pid);
		return smash_pid;
	}
}

int pwd(job job_obj) {
	parseCmdExample();
	if (job_obj.nargs != 0) {
		perrorSmash(const char* "pwd", const char* "expected 0 arguments");
		// fprintf(stderr, "smash error: pwd: expected 0 argument");
		return -1;
	}
	else {
		
		char* pwd = (char*)malloc(CMD_LENGTH_MAX); // is there a clear defenition of how long can it be ??
		if (pwd == NULL) {
			perrorSmash(const char* "pwd", const char* "malloc failed");
			exit(-1);
    	}
		if (getcwd(pwd, CMD_LENGTH_MAX) != NULL) {
        	printf("%s\n", pwd);
			free(pwd);
			return 0;
    	} 
		else {
			perrorSmash(const char* "pwd", const char* "getcwd failed");
        	free(pwd);
			return -1;
		}
	}
}

int kill(job job_obj, int signum, int job_id) {
	parseCmdExample();
	if ((job_obj.nargs != 2) || (job_obj.args[1] < 1) || (job_obj.args[1] > 64) ) { // make sure signum is a valid signum (1<=args[1]<=64)
		perrorSmash(const char* "kill", const char* "invalid arguments");

		return -1;
	}
	for (i=0 ; i < 100 ; i++){
		if (job_id == i && job_list[i] != NULL ){
			my_system_call(5, job_list[i].PID, signum); //send signal
			printf("signal %d was sent to pid %ld", signum, job_list[i].PID); // maybe we should define all global variables in commands.h ??
			return 0;
		}
	}
	char error_msg[MAX_ERROR_LEN] = {0}; // longest message should be no longer than 25
	snprintf(error_msg, MAX_ERROR_LEN, "job id %d does not exist", job_id);

	perrorSmash(const char* "kill", const char* error_msg);
	return -1;
}

int cd (job job_obj, char* path) {

	parseCmdExample();
	if (job_obj.nargs != 1) {
		perrorSmash(const char* "cd", const char* "expected 1 arguments");
		return -1;
	}
	else {
		if (job.args[1] == '-'){
			if (old_cd == NULL) {
				perrorSmash(const char* "cd", const char* "old pwd not set");
				return -1;
    		}
			else {
				printf("%s \n", old_path); // TODO: change the dir
				return 0;
			}
		}
		else if (job.args[1] == '..') {
			char* pwd[CMD_LENGTH_MAX] = {0}; // is there a clear defenition of how long can it be ??
			if (getcwd(pwd, CMD_LENGTH_MAX) != NULL) {
				char* delimiters = "/"; 
				//char path_to_print [CMD_LENGTH_MAX] = {0};
				char* last_dir = strrchr(path, delimiters);
				if (strcmp(*last_dir, path)){
					return 0;
				}
				else {
					*last_dir = '/0';
					printf("pwd\n%s \n", path); // change the dir
					return 0;
				}
				
    		} 

		}
		else {
			DIR* dir = opendir(path);
			if (dir == NULL){ // check if path valid
				if (errno == ENOENT){
					perrorSmash(const char* "cd", const char* "target directory does not exist");
				}

			}
			else if (){

			} // check if path to a dir or a file
			else {
				printf("pwd\n%s\n", path);
				return 0;
			}
		
		}
		
	}

}

int jobs(job job_obj){
	if (job_obj.nargs != 0){
		perrorSmash(const char* "jobs", const char* "expected 0 arguments");
		return -1;	
	}
	else {
		for (i = 0 ; i<100 ; i++){
			if (jobs_list[i] != NULL){
				if (jobs_list[i].state == 3){
					printf("[%d] %s: %ld %d secs (stopped)\n", i, jobs_list[i].cmd, jobs_list[i].PID, jobs_list[i].time)
				}
				else{
					printf("[%d] %s: %ld %d secs\n", i, jobs_list[i].cmd, jobs_list[i].PID, jobs_list[i].time)
				}
			}
		}
		return 0;
	}
}



