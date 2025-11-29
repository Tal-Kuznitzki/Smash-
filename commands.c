//commands.c

#include "commands.h"
#include "my_system_call.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>    
#include <unistd.h>


#define MAX_ERROR_LEN 30
#define CMD_LENGTH_MAX 120
#define ARGS_NUM_MAX 20
#define JOBS_NUM_MAX 100

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
#define EOF_READ 0

#define GRACE_PERIOD 5
#define CHECK_INTERVAL_SECONDS 1
#define SIGKILL 9
#define SIGTERM 15
#define O_RDONLY 0
//ERRORs
#define NOARGSVAL -1
#define EISDIR 21

#define JOB_STATE_FG 1
#define JOB_STATE_BG 2
#define JOB_STATE_STP 3
const char* cmd_DB[11]= {"showpid","pwd","cd","jobs","kill","fg","bg","quit","diff","alias","unalias" } ;
char old_cd[CMD_LENGTH_MAX] = {0};
char current_cd[CMD_LENGTH_MAX] = {0};

job* jobs_list[100];


int current_job_index=0; // TODO update accordingly



int diff(char* args[ARGS_NUM_MAX],int nargs){
    if (nargs!=2){
        perrorSmash("diff","expected 2 arguments");
        return -1 ; // TODO :VERIFY WHAT RETVAL SHOULD BE ON ERROR
    }
    char* path1 = args[0];
    char* path2 = args[1];

    //opening both files;
    int fd1 = my_system_call(SYS_OPEN,path1,O_RDONLY);
    int fd2 = my_system_call(SYS_OPEN,path2,O_RDONLY);

    if ( ( (fd1 == ERROR) &&  (errno==ENOENT) ) || ( (fd2 == ERROR) &&  (errno==ENOENT) ) ){
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

        if ( ( (bytes_read1 == ERROR) &&  (errno==EISDIR) ) || ( (bytes_read2 == ERROR) &&  (errno==EISDIR) ) ){
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
        if ( ( ( bytes_read1 == EOF_READ ) && (bytes_read2 != EOF_READ ) ) || ( (bytes_read2 == EOF_READ) && (bytes_read1 != EOF_READ) ) ) { //ended reading one file but not the other.
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            return DIFFERENT;
        }
        if ( (bytes_read1 == EOF_READ ) && (bytes_read2 == EOF_READ) ) { //ended reading both files no errors so far
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            return IDENTICAL;
        }

    }
}

int fg(int job_id, int nargs) {

    if (nargs > 1 ){
        perrorSmash("fg","invalid arguments");
        return -1; //TODO error val
    }
    int job_idx_in_jobs = -1;
    if (job_id == NOARGSVAL ){
        int maxId = jobs_list[0]->JOB_ID;
        if ( jobs_list[0] == NULL){// TODO is this the best way to verify empty joblist????
            perrorSmash("fg"," jobs list is empty");
            return ERROR; //TODO error val
        }
        for (int i = 1; i < JOBS_NUM_MAX; i++) {
            if (jobs_list[i]->JOB_ID > maxId) {
                maxId = jobs_list[i]->JOB_ID;
                job_idx_in_jobs = i;
            }
        }
    }
    else{
        for (int j = 0; j < JOBS_NUM_MAX ; ++j) {
            if (jobs_list[j]->JOB_ID == job_id ){
                job_idx_in_jobs = j;
                break;
            }
        }
        // if after the loop we didn't find the job_ID, print err
        if (job_idx_in_jobs == -1){
            char* msg;
            sprintf(msg, "job id %d does not exist", job_id);
            perrorSmash("fg",msg);
            return ERROR; //TODO error val
        }

    }

    //now we know job_idx_in_jobs
    //TODO: verify print format
    printf("%s %d", jobs_list[job_idx_in_jobs]->cmd, jobs_list[job_idx_in_jobs]->PID);
    if (jobs_list[job_idx_in_jobs]->state == JOB_STATE_STP){ // if stopped, send it SIGCONT
        jobs_list[job_idx_in_jobs]->state = JOB_STATE_FG;
        my_system_call(SYS_KILL,jobs_list[job_idx_in_jobs]->PID,SIGCONT);
        printf("%s", jobs_list[job_idx_in_jobs]->cmd);

    }
    else{
        jobs_list[job_idx_in_jobs]->state = JOB_STATE_FG;
    }

    // TODO  actually run the JOB?


}

int bg(int job_id, int nargs){

    if (nargs > 1 ){
        perrorSmash("bg","invalid arguments");
        return ERROR;
    }
    int job_idx_in_jobs = -1;
    if (job_id == NOARGSVAL ){
        int maxId = jobs_list[0]->JOB_ID;
        for (int i = 1; i < JOBS_NUM_MAX; i++) {
            // TODO: MAKE SURE REGARDING NO JOB_ID AND NO JOB IN STOPPED, BECAUSE WE SAID IT IS EQUAL TO MAX, ALSO MAKE SURE ARG FORMAT IS OKAY

            if ( ( ( jobs_list[i]->JOB_ID < 100 )&& (jobs_list[i]->JOB_ID > 100 )  ) &&  (jobs_list[i]->JOB_ID > maxId )   ) {
                maxId = jobs_list[i]->JOB_ID;
                job_idx_in_jobs = i;
            }
        }
        if ( (job_idx_in_jobs !=ERROR) && (jobs_list[job_idx_in_jobs]->state != JOB_STATE_STP )){
            char* msg;
            sprintf(msg, "job id %d is already in background", job_id);
            perrorSmash("bg",msg);
            return ERROR;
        }
    }
    else{
        for (int j = 0; j < JOBS_NUM_MAX ; ++j) {
            if (jobs_list[j]->JOB_ID == job_id ){
                job_idx_in_jobs = j;
                break;
            }
        }

        if (job_idx_in_jobs == ERROR){  // if after the loop we didn't find the job_ID, print err
            char* msg;
            sprintf(msg, "job id %d does not exist", job_id);
            perrorSmash("bg",msg);
            return ERROR;
        }
        else{ //in case we found the JOB_ID, verify it is stopped.

            if (jobs_list[job_idx_in_jobs]->state != JOB_STATE_STP){ //if not stopped, err
                char* msg;
                sprintf(msg, "job id %d is already in background", job_id);
                perrorSmash("bg",msg);
                return ERROR;
            }
        }
    }
    // if alles gut
    printf("%s %d", jobs_list[job_idx_in_jobs]->cmd, jobs_list[job_idx_in_jobs]->PID);
    my_system_call(SYS_KILL,jobs_list[job_idx_in_jobs]->PID,SIGCONT); //sending SIGCONT to the stopped job
    jobs_list[job_idx_in_jobs]->state =  JOB_STATE_BG;
}

int quit(int nargs ,char* arg){// kill the smash
    if (nargs>1){
        perrorSmash("quit","expected 0 or 1 arguments");
        return ERROR;
    }
    if (strcmp(arg,"kill") == 0){
        //kill jobs in order.
        for (int i = 0; i < JOBS_NUM_MAX; ++i) {
            printf("%d %s - ",jobs_list[i]->JOB_ID,jobs_list[i]->cmd);
            my_system_call(SYS_KILL, jobs_list[i]->PID, SIGTERM); //send sigterm
            printf("sending SIGTERM... ");




            // now check status every sec

            int terminated_gracefully = 0;
            for (int elapsed_time = 0; elapsed_time < GRACE_PERIOD; elapsed_time += CHECK_INTERVAL_SECONDS) {
                int status;
                // TODO FIXXXXX int result = my_system_call(SYS_WAITPID, jobs_list[i]->PID, &status,WNOHANG); //send sigterm
                int result = 6;
                if (result == jobs_list[i]->PID) {
                    // Process terminated and was successfully reaped
                   terminated_gracefully = 1;
                    break; // Exit the grace period loop
                } else if (result == 0) {
                    // Process is still running, wait for the interval
                    sleep(CHECK_INTERVAL_SECONDS);
                }
                /* not needed
                 * else if (result == -1 && errno == ECHILD) {
                    // Process likely terminated/reaped before SIGTERM or immediately after
                    terminated_gracefully = 1;
                    break; // Exit the grace period loop
                } else if (result == -1) {
                    perror("waitpid error");
                    terminated_gracefully = 1; // Treat as failure and stop check
                    break;
                }
                 */
            }
            if (!terminated_gracefully){ //process still alive.. KILL!
                my_system_call(SYS_KILL, jobs_list[i]->PID, SIGKILL); //send sigkill
                printf("sending SIGKILL... done");
            }


            printf("\n");
        }
    }
    else{
        perrorSmash("quit","unexpected arguments");
        return ERROR;
    }
    // return error to signal QUITTING
    return QUITVAL;


}

int showpid(cmd cmd_obj) {
    if (cmd_obj.nargs != 0) {
        perrorSmash("showpid","expected 0 arguments");
        // fprintf(stderr, "smash error: showpid: expected 0 argument");
        return -1;
    }
    else {
        pid_t smash_pid = getpid();
        printf ("smash pid is %ld\n", (long) smash_pid);
        return smash_pid;
    }
}

int pwd(cmd cmd_obj) {
    if (cmd_obj.nargs != 0) {
        perrorSmash("pwd","expected 0 arguments");
        // fprintf(stderr, "smash error: pwd: expected 0 argument");
        return -1;
    }
    else {

        char* pwd = (char*)malloc(CMD_LENGTH_MAX); 
        if (pwd == NULL) {
            perrorSmash("pwd","malloc failed");
            exit(-1);
        }
        if (getcwd(pwd, CMD_LENGTH_MAX) != NULL) {
            printf("%s\n", pwd);
            free(pwd);
            return 0;
        }
        else {
            perrorSmash("pwd","getcwd failed");
            free(pwd);
            return -1;
        }
    }
}

int kill(cmd cmd_obj) {
	int signum = atoi(cmd_obj.args[1]);
	int job_id = atoi(cmd_obj.args[2]);
    if ( ( (cmd_obj.nargs ) != 2)  || ( ( atoi(cmd_obj.args[1]) )  < 1) || ( ( atoi(cmd_obj.args[1]) ) > 64) ) { // make sure signum is a valid signum (1<=args[1]<=64)
        perrorSmash("kill","invalid arguments");

        return ERROR;
    }
    for (int i=0 ; i < 100 ; i++){
        if (job_id == i && jobs_list[i] != NULL ){
            my_system_call(5, jobs_list[i]->PID, signum); //send signal
            printf("signal %d was sent to pid %d", signum, jobs_list[i]->PID); // maybe we should define all global variables in commands.h ??
            return 0;
        }
    }
    char error_msg[MAX_ERROR_LEN] = {0}; // longest message should be no longer than 25
    snprintf(error_msg, MAX_ERROR_LEN, "job id %d does not exist", job_id);

    perrorSmash("kill",error_msg);
    return -1;
}

int cd (cmd cmd_obj) {
    if (cmd_obj.nargs != 1) {
        perrorSmash("cd","expected 1 arguments");
        return -1;
    }
    else {
		getcwd(current_cd, CMD_LENGTH_MAX);
		char temp[CMD_LENGTH_MAX];
		char* path = cmd_obj.args[1];

        if (strcmp(cmd_obj.args[1],"-") == 0){
            if (old_cd == NULL) {
                perrorSmash("cd","old pwd not set");
                return -1;
            }
            else {
				chdir(old_cd);
				printf("%s \n", old_cd);
				strcpy(temp, old_cd);
				strcpy(old_cd, current_cd);
				strcpy(current_cd, temp);
                return 0;
            }
        }
        else if (strcmp(cmd_obj.args[1],"..") == 0 ) {
            if (current_cd != NULL) {
                char delimiters = '/';
                //char path_to_print [CMD_LENGTH_MAX] = {0};
                char* last_dir = strrchr(current_cd, delimiters);
                if (strcmp(last_dir, current_cd)){
                    return 0; // do we need to add print of current dir?
                }
                else {
					strcpy(old_cd, current_cd); //the full og path - will be noe old_cd
                    *last_dir = '\0'; //cut the last part, also update current_cd
					chdir(current_cd); //move to home dir
                    printf("pwd\n%s \n", current_cd); 
                    return 0;
                }

            }

        }
        else {
            DIR* dir = opendir(path);
            if (dir == NULL){ // check if path valid
                if (errno == ENOENT){ //if directory doesnt exist
                    perrorSmash("cd","target directory does not exist");
                }
				else if (errno == ENOTDIR) { //if path is of a file
					char error_msg[MAX_ERROR_LEN + CMD_LENGTH_MAX];
					snprintf(error_msg, MAX_ERROR_LEN + CMD_LENGTH_MAX, "%s: not a directory", path);

					perrorSmash("cd",error_msg);

				}

            }
            else {
				chdir(path);
                printf("pwd\n%s\n", path);
				strcpy(old_cd, current_cd);
				strcpy(current_cd, path);
                return 0;
            }

        }

    }

}

int jobs(cmd cmd_obj){
    if (cmd_obj.nargs != 0){
        perrorSmash("jobs","expected 0 arguments");
        return ERROR;
		}
    else {
        for (int i = 0 ; i<100 ; i++){
			time_t curr_time = time.time();
            if (jobs_list[i] != NULL){
				float diff = difftime(curr_time, jobs_list[i]->time);
                if (jobs_list[i]->state == JOB_STATE_STP){
                    printf("[%d] %s: %d %f secs (stopped)\n", i, jobs_list[i]->cmd, jobs_list[i]->PID, diff);
                }
                else{
                    printf("[%d] %s: %d %f secs\n", i, jobs_list[i]->cmd, jobs_list[i]->PID, diff);
                }
            }
        }
        return 0;
    }
}

int alias( char* new_cmd_name, char* new_cmd){


}


int command_selector(cmd cmd_after_parse){


    if ( strcmp(cmd_after_parse.cmd,cmd_DB[0] ) == 0 ){
         return showpid(cmd_after_parse);
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[1] ) == 0  ) {
        return pwd(cmd_after_parse);
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[2] ) == 0  ) {
         return cd(cmd_after_parse);
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[3] ) == 0  ) {
         return jobs(cmd_after_parse);
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[4] ) == 0  ) {
         return kill(cmd_after_parse);
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[5]  ) == 0  ) {
        //TODO: add check at parser for #args, and pass -1 if no args
        //  fg();
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[6] ) == 0  ) {
        //  bg();
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[7] ) == 0 ) {
        // if( quit(...) == QUITVAL) return QUITVAL ; //SIG TO END the program
        // return ERROR;
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[8]  ) == 0 ) {
        //   diff();
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[9] ) == 0  ) {
        //   alias();
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[10] ) == 0  ) {
        //   unalias();
        return 1;
    }

    return 1;

}

//example function for printing errors from internal commands
void perrorSmash(const char* cmd, const char* msg)
{
    fprintf(stderr, "smash error:%s%s%s\n",
            cmd ? cmd : "",
            cmd ? ": " : "",
            msg);
}

//example function for parsing commands
cmd* parseCmdExample(char* line)
{
    cmd cmd_list[ARGS_NUM_MAX]= {0};
	cmd cmd_obj;
	cmd_obj.internal = 0;
    char* delimiters = " \t\n"; //parsing should be done by spaces, tabs or newlines
    cmd_obj.cmd = strtok(line, delimiters); //read strtok documentation - parses string by delimiters
    if(!cmd_obj.cmd)
        return INVALID_COMMAND; //this means no tokens were found, most like since command is invalid

    cmd_obj.nargs = 0;
    cmd_obj.args[0] = cmd_obj.cmd; //first token before spaces/tabs/newlines should be command name
    for(int i = 1; i < MAX_ARGS; i++)
    {
        cmd_obj.args[i] = strtok(NULL, delimiters); //first arg NULL -> keep tokenizing from previous call
        if(!cmd_obj.args[i])
            break;
        cmd_obj.nargs++;
    }

/// cdhello
    int num_cmd=0;
    int start_of_cmd=0;
    int end_of_cmd=-1;
    cmd_list[num_cmd]=cmd_obj; // 1 cmd only
    for (int i = 0; i < ARGS_NUM_MAX; ++i) {
        if( strcmp(cmd_obj.args[i],"&&")  == 0 ){
           cmd cmd_obj_tmp;
            cmd_obj_tmp.bg=0;
           end_of_cmd=i;
            cmd_obj_tmp.nargs=end_of_cmd-start_of_cmd-1;
            strcpy(cmd_obj_tmp.cmd,cmd_obj.args[start_of_cmd]);
            for (int k = i-1; k >= start_of_cmd ; k--) {
                strcpy(cmd_obj_tmp.args[k-start_of_cmd],cmd_obj.args[k]);
            }

            //start_of end of
            for (int i=0 ; i<11 ; i++) {
                if (!strcmp(cmd_DB[i], cmd_obj_tmp.cmd)){
                    cmd_obj_tmp.internal = 1;
                }
            }
            start_of_cmd=end_of_cmd+1;
            cmd_list[num_cmd]=cmd_obj_tmp;
            num_cmd++;
        }
    }


    for (int i=0 ; i<11 ; i++) {
        if (!strcmp(cmd_DB[i], cmd_obj.cmd)){
            cmd_obj.internal = 1;
        }
    }

	for (int i = 19 ; i>0 ; i--) {
		if (cmd_obj.args[i] == "&"){
			cmd_obj.bg = 1;
			cmd_obj.nargs--;
		}
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
    return cmd_list;
}






