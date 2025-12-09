//commands.c

#include "commands.h"


#define MAX_ERROR_LEN 30
#define CMD_LENGTH_MAX 80
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
const char* cmd_DB[11]= {"showpid","pwd","cd","jobs","kill","fg","bg","quit","diff","alias","unalias"};
char old_cd[CMD_LENGTH_MAX] = {0};
char current_cd[CMD_LENGTH_MAX] = {0};
cmd cmd_list[ARGS_NUM_MAX]= {0};
list* head_alias_list = NULL;

job jobs_list[100];

int smash_pid;
int curr_fg_pid;



// TODO update accordingly


void build_cmd_full(cmd* c) {
    memset(c->cmd_full, 0, CMD_LENGTH_MAX); // Clear it
    if (c->args[0] == NULL) return;

    // Copy command name
    strcpy(c->cmd_full, c->args[0]);

    // Concatenate arguments
    for (int i = 1; i <= c->nargs; i++) { // Note: nargs counts args after cmd
        if (c->args[i] != NULL) {
            strcat(c->cmd_full, " ");
            strcat(c->cmd_full, c->args[i]);
        }
    }

    // Append '&' if it's a background command
    if (c->bg == 1) {
        strcat(c->cmd_full, " &");
    }
}
int diff(cmd cmd_obj){
    if (cmd_obj.nargs!=2){
        perrorSmash("diff","expected 2 arguments");
        return ERROR ;
    }
    char* path1 = cmd_obj.args[1];
    char* path2 = cmd_obj.args[2];

    //opening both files;
    int fd1 = my_system_call(SYS_OPEN,path1,O_RDONLY);
    int fd2 = my_system_call(SYS_OPEN,path2,O_RDONLY);

    if ( ( (fd1 == ERROR) &&  (errno==ENOENT) ) || ( (fd2 == ERROR) &&  (errno==ENOENT) ) ){
        perrorSmash("diff","expected valid paths for files");
        return ERROR ;
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
            return ERROR;
        }
        if (bytes_read1 != bytes_read2) { //different sizes, so different ofc
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            printf("%d\n", DIFFERENT);
            return DIFFERENT;
        }
        if ( memcmp(buffer1, buffer2, bytes_read1) != IDENTICAL ) { //chunks are not identical
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            printf("%d\n", DIFFERENT);
            return DIFFERENT;
        }
        if ( ( ( bytes_read1 == EOF_READ ) && (bytes_read2 != EOF_READ ) ) || ( (bytes_read2 == EOF_READ) && (bytes_read1 != EOF_READ) ) ) { //ended reading one file but not the other.
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            printf("%d\n", DIFFERENT);
            return DIFFERENT;
        }
        if ( (bytes_read1 == EOF_READ ) && (bytes_read2 == EOF_READ) ) { //ended reading both files no errors so far
            my_system_call(SYS_CLOSE, fd1);
            my_system_call(SYS_CLOSE, fd2);
            printf("%d\n", IDENTICAL);
            return IDENTICAL;
        }

    }
}

int fg(cmd cmd_obj) {
    int job_id;
    if (cmd_obj.nargs > 1 ){
        perrorSmash("fg","invalid arguments");
        return ERROR;
    }
    else if (cmd_obj.nargs == 0){
        job_id = NOARGSVAL;
    }
    else if (cmd_obj.nargs == 1){
        job_id = atoi(cmd_obj.args[1]);
    }
    int job_idx_in_jobs = 0;
    if (job_id == NOARGSVAL ) { //if no job_id, two options, either take the maximal job_id,
        int job_id = jobs_list[0].JOB_ID;
        if (jobs_list[0].PID == ERROR) {
            // TODO is this the best way to verify empty joblist????
            perrorSmash("fg", " jobs list is empty");
            return ERROR;
        }
        //find maximal job_id
        for (int i = 1; i < JOBS_NUM_MAX; i++) {
            if (jobs_list[i].JOB_ID > job_id) {
                job_id = jobs_list[i].JOB_ID;
                job_idx_in_jobs = i;
            }
        }
    }
    else{
        for (int j = 0; j < JOBS_NUM_MAX; ++j) {
            if (jobs_list[j].JOB_ID == job_id ){
                job_idx_in_jobs = j;
                break;
            }
        }
        // if after the loop we didn't find the job_ID, print err
        if (job_idx_in_jobs == -1){
            char msg[CMD_LENGTH_MAX];
            sprintf(msg, "job id %d does not exist", job_id);
            perrorSmash("fg",msg);
            return ERROR;
        }
    }
    //now we know job_idx_in_jobs
    //TODO: verify print format
    printf("%s %d\n", jobs_list[job_idx_in_jobs].cmd_full, jobs_list[job_idx_in_jobs].PID);
    if (jobs_list[job_idx_in_jobs].state == JOB_STATE_STP){ // if stopped, send it SIGCONT
        jobs_list[job_idx_in_jobs].state = JOB_STATE_FG;
        my_system_call(SYS_KILL,jobs_list[job_idx_in_jobs].PID,SIGCONT);
        printf("%s\n", jobs_list[job_idx_in_jobs].cmd_full);
    }
    else{
        jobs_list[job_idx_in_jobs].state = JOB_STATE_FG;
    }
    curr_fg_pid = jobs_list[job_idx_in_jobs].PID;
    int wait_ret;
    int wait_status;
    int EINTR_VAL = 4; // Assuming EINTR is 4, consistent with smash.c
    do{
        wait_ret = my_system_call(SYS_WAITPID, curr_fg_pid, &wait_status, 2);
    }
    while (wait_ret == ERROR && errno == EINTR_VAL);

    // 5. Cleanup foreground tracking
    job_to_fg_pid = ERROR; // Smash regains control
    curr_fg_pid = smash_pid;

    if (wait_ret > 0) {
        // The process was stopped (equivalent to WIFSTOPPED(wait_status) check)
        // Check if the process was stopped (0x7f is for STOPPED status)
        if (((wait_status & 0xFF) == 0x7f) && ((wait_status >> 8) == SIGSTP))
        {
            // The signal handler (sigintHandler) is responsible for updating the jobs_list state
            // and adding the job if it wasn't there (though it should be if 'fg' was run).
            // Since the signal handler is unsafe, you must assume it's set the flag
            // and printed the output. The job should remain in the jobs_list as STOPPED.

            // Note: The sigintHandler should ideally just set a flag, and the main loop should
            // handle the job list update safely. For this fix, we assume the handler handles it.
            return 0;
        }
            // Process terminated or was killed (WIFEXITED or WIFSIGNALED)
        else{
            // The SIGCHLD handler (sigchldHandler) is responsible for cleaning up the job entry.
            return 0;
        }
    }

    return 0;
}

int bg(cmd cmd_obj){
    int job_id = atoi(cmd_obj.args[1]);
    if (cmd_obj.nargs > 1 ){
        perrorSmash("bg","invalid arguments");
        return ERROR;
    }
    int job_idx_in_jobs = 0;
    if (job_id == NOARGSVAL ){
        int maxId = jobs_list[0].JOB_ID;
        for (int i = 1; i < JOBS_NUM_MAX; i++) {
            // TODO: MAKE SURE REGARDING NO JOB_ID AND NO JOB IN STOPPED, BECAUSE WE SAID IT IS EQUAL TO MAX, ALSO MAKE SURE ARG FORMAT IS OKAY

            if ( ( ( jobs_list[i].JOB_ID < 100 )&& (jobs_list[i].JOB_ID > 100 )  ) &&  (jobs_list[i].JOB_ID > maxId )   ) {
                maxId = jobs_list[i].JOB_ID;
                job_idx_in_jobs = i;
            }
        }
        if ( (job_idx_in_jobs !=ERROR) && (jobs_list[job_idx_in_jobs].state != JOB_STATE_STP )){
            char msg[CMD_LENGTH_MAX];
            sprintf(msg, "job id %d is already in background", job_id);
            perrorSmash("bg",msg);
            return ERROR;
        }
    }
    else{
        for (int j = 0; j < JOBS_NUM_MAX ; ++j) {
            if (jobs_list[j].JOB_ID == job_id ){
                job_idx_in_jobs = j;
                break;
            }
        }

        if (job_idx_in_jobs == ERROR){  // if after the loop we didn't find the job_ID, print err
            char msg[CMD_LENGTH_MAX];
            sprintf(msg, "job id %d does not exist", job_id);
            perrorSmash("bg",msg);
            return ERROR;
        }
        else{ //in case we found the JOB_ID, verify it is stopped.

            if (jobs_list[job_idx_in_jobs].state != JOB_STATE_STP){ //if not stopped, err
                char msg[CMD_LENGTH_MAX];
                sprintf(msg, "job id %d is already in background", job_id);
                perrorSmash("bg",msg);
                return ERROR;
            }
        }
    }
    // if alles gut
    printf("%s %d", jobs_list[job_idx_in_jobs].cmd_full, jobs_list[job_idx_in_jobs].PID);
    my_system_call(SYS_KILL,jobs_list[job_idx_in_jobs].PID,SIGCONT); //sending SIGCONT to the stopped job
    jobs_list[job_idx_in_jobs].state = JOB_STATE_BG;
    return 0;
}

int quit(cmd cmd_obj){// kill the smash

    if (cmd_obj.nargs>1){
        perrorSmash("quit","expected 0 or 1 arguments");
        return ERROR;
    }
    if ( cmd_obj.args[1] != NULL && strcmp(cmd_obj.args[1],"kill") == 0){   //kill jobs in order.
        for (int i = 0; i < JOBS_NUM_MAX; ++i) {
            if (jobs_list[i].PID == ERROR) continue;
            printf("[%d] %s - ",jobs_list[i].JOB_ID,jobs_list[i].cmd_full);
            my_system_call(SYS_KILL, jobs_list[i].PID, SIGTERM);
            printf("sending SIGTERM... ");
            fflush(stdout);
            bool job_is_dead = false;
            for (int second = 0; second < 5 ; ++second) {
                int still_alive = my_system_call(SYS_KILL,jobs_list[i].PID,0);
                if (still_alive == ERROR){
                    //eliminated the process successfully :)
                    job_is_dead=true;
                    break;
                }
                sleep(1);
            }
            if (job_is_dead){
                printf("done");
                fflush(stdout);
            }
            else{ // 5 sec passed but job is still alive, send SIGKILL
                my_system_call(SYS_KILL, jobs_list[i].PID, SIGKILL);
                printf("sending SIGKILL... done");
                fflush(stdout);
            }
            int status;
            my_system_call(SYS_WAITPID, jobs_list[i].PID, &status, 0); //small wait for Process table to update - needed?
            printf("\n");
        }
    }
    else if (cmd_obj.args[1] != NULL && strcmp(cmd_obj.args[1],"kill") !=0 ) {
        perrorSmash("quit","unexpected arguments");
        return ERROR;
    }
    return QUITVAL;
}

int showpid(cmd cmd_obj) {
    if (cmd_obj.nargs != 0) {
        perrorSmash("showpid","expected 0 arguments");
        return ERROR;
    }
    else {
        printf ("smash pid is %d\n", smash_pid);
        return smash_pid;
    }
}

int pwd(cmd cmd_obj) {
    if (cmd_obj.nargs != 0) {
        perrorSmash("pwd","expected 0 arguments");
        return ERROR;
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

int my_kill(cmd cmd_obj) {
	int signum =(cmd_obj.args[1]!=NULL) ? atoi(cmd_obj.args[1]) : -1;
	int job_id = (cmd_obj.args[2]!=NULL) ? atoi(cmd_obj.args[2]) : -1 ;
    if ( ( (cmd_obj.nargs ) != 2)  || ( signum < 1) || (( signum ) > 64) ) { // make sure signum is a valid signum (1<=args[1]<=64)
        perrorSmash("kill","invalid arguments");

        return ERROR;
    }
    for (int i=0 ; i < 100 ; i++){
        if (job_id == i && jobs_list[i].PID != ERROR ){
            my_system_call(5, jobs_list[i].PID, signum); //send signal
            printf("signal %d was sent to pid %d", signum, jobs_list[i].PID); // maybe we should define all global variables in commands.h ??
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
        return ERROR;
    }
    else {
		getcwd(current_cd, CMD_LENGTH_MAX);
		char temp[CMD_LENGTH_MAX];
		char* path = cmd_obj.args[1];

        if (strcmp(cmd_obj.args[1],"-") == 0){
            if (strcmp(old_cd,"") == 0 )  {
                perrorSmash("cd","old pwd not set");
                return ERROR;
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
            if ( strcmp(current_cd, "") ) {
                char delimiters = '/';
                //char path_to_print [CMD_LENGTH_MAX] = {0};
                char* last_dir = strrchr(current_cd, delimiters);
                if ( strcmp(last_dir, current_cd) == 0 ){
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
                    return  ERROR;

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
    return 0;
}

int jobs(cmd cmd_obj){
    if (cmd_obj.nargs != 0){
        perrorSmash("jobs","expected 0 arguments");
        return ERROR;
		}
    else {
        for (int i = 0 ; i<JOBS_NUM_MAX ; i++){
			time_t curr_time = time(NULL);
            if (jobs_list[i].PID != ERROR){
				float diff = difftime(curr_time, jobs_list[i].time);
                if (jobs_list[i].state == JOB_STATE_STP){
                    printf("[%d] %s: %d %f secs (stopped)\n", i, jobs_list[i].cmd_full, jobs_list[i].PID, diff);
                }
                else{
                    printf("[%d] %s: %d %f secs\n", i, jobs_list[i].cmd_full, jobs_list[i].PID, diff);
                }
            }
        }
        return 0;
    }
}

int alias(cmd cmd_obj){
	
	// check if this word already exist - if yes - delete it from the list and create new one
	list* current_to_delete = head_alias_list;
            while (current_to_delete != NULL) {
                if ( strcmp(current_to_delete->alias, cmd_obj.args[1]) ) {
                    if (current_to_delete->prev == NULL) {
                        // if current is head - update head
                        head_alias_list = current_to_delete->next;
                    } else {
                        // update the "next" ptr of the prev node
                        current_to_delete->prev->next = current_to_delete->next;
                    }

                    if (current_to_delete->next != NULL) {
                        // in both cases update the next node's prev if it exists
                        current_to_delete->next->prev = current_to_delete->prev;
                    }

                    // release memory
                    free(current_to_delete);
                    break;
                }

                current_to_delete = current_to_delete->next;
            }
        
        strcpy(cmd_obj.cmd, cmd_obj.args[1]);
		int num_cmd = 0;
	 	int start_of_cmd=2;
        int end_of_cmd=-1;
        list* new_node = (list*)malloc(sizeof(list));
        if (new_node == NULL) {
            // if malloc fails
            return ERROR;
        }
        memset(new_node->og_cmd_list, 0, sizeof(new_node->og_cmd_list));
        memset(new_node->alias, 0, sizeof(new_node->alias));
        strcpy(new_node->alias, cmd_obj.args[1]);

        cmd cmd_obj_tmp;
        cmd_obj_tmp.bg=0;
        cmd_obj_tmp.internal = 0;

        for (int i = 2; i < ARGS_NUM_MAX; ++i) {
            if( strcmp(cmd_obj.args[i],"&&")  == 0 ){
                
                end_of_cmd=i;
                cmd_obj_tmp.nargs=end_of_cmd-start_of_cmd-1;
                strcpy(cmd_obj_tmp.cmd,cmd_obj.args[start_of_cmd]);
                for (int k = i-1; k >= start_of_cmd ; k--) {
                    strcpy(cmd_obj_tmp.args[k-start_of_cmd],cmd_obj.args[k]);
                }

                for (int i=0 ; i<11 ; i++) {
                    if (!strcmp(cmd_DB[i], cmd_obj_tmp.cmd)){
                        cmd_obj_tmp.internal = 1;
                    }
                }
                start_of_cmd=end_of_cmd+1;
                build_cmd_full(&cmd_obj_tmp);
                new_node->og_cmd_list[num_cmd]=cmd_obj_tmp;
                num_cmd++;
            }
        }

        // add the last command
        if (num_cmd!=0){
             cmd_obj_tmp.bg = 0;
             end_of_cmd = cmd_obj.nargs + 1 ;
             cmd_obj_tmp.nargs = end_of_cmd - start_of_cmd - 1;
             strcpy(cmd_obj_tmp.cmd, cmd_obj.args[start_of_cmd]);
             for (int k = cmd_obj.nargs ; k >= start_of_cmd; k--) {
                 cmd_obj_tmp.args[k - start_of_cmd]=cmd_obj.args[k];
             }

             for (int j = 0; j < 11; j++) {
                 if (!strcmp(cmd_DB[j], cmd_obj_tmp.cmd)) {
                     cmd_obj_tmp.internal = 1;
                 }
             }
            build_cmd_full(&cmd_obj_tmp);

             start_of_cmd = end_of_cmd + 1;
             // only if there wasnt alias
            // if (old_num_cmd == num_cmd) { TODO MAYA LOOK HERE WHERE SHOULD OLD_NUM_CMD BE DEFINEDD
            if(1){   new_node->og_cmd_list[num_cmd] = cmd_obj_tmp;
                 num_cmd++;
             }
         }

        //if only one cmd
        if (num_cmd == 0) {

            for (int i=0; i<(cmd_obj.nargs - 2); i++){
                cmd_obj.args[i] = cmd_obj.args[i+2];
            }
            
            for (int i=(cmd_obj.nargs - 1); i<ARGS_NUM_MAX; i++){
                cmd_obj.args[i] = NULL; // maybe should be 0?
            }
            cmd_obj.nargs = cmd_obj.nargs - 2;

            for (int i = 0; i < 11; i++) {
                if ((cmd_obj.cmd != NULL) && (strcmp(cmd_DB[i], cmd_obj.cmd) == 0)) {
                    cmd_obj.internal = 1;
                    break;
                }
            }
            
            // if (cmd_obj.args[cmd_obj.nargs] != NULL && strcmp(cmd_obj.args[cmd_obj.nargs], "&") == 0) {
            //     cmd_obj.bg = 1;
            //     cmd_obj.args[cmd_obj.nargs]=NULL;
            //     cmd_obj.nargs--;
            // }
            build_cmd_full(&cmd_obj);
            new_node->og_cmd_list[num_cmd]=cmd_obj; // 1 cmd only

        }
        // update ptrs
        new_node->next = head_alias_list;
        new_node->prev = NULL; // first node will point to NULL

        // update prev node only if it is not the first node
        if (head_alias_list != NULL) {
            (head_alias_list)->prev = new_node;
        }

        // update head of the list - to be the new node
        head_alias_list = new_node;

        //return new_node;

    return 0;

}

int unalias(cmd cmd_obj) {

	list* current = head_alias_list;

            while (current != NULL) {
                if ( strcmp(current->alias, cmd_obj.args[1]) ) {
                    if (current->prev == NULL) {
                        // if current is head - update head
                        head_alias_list = current->next;
                    } else {
                        // update the "next" ptr of the prev node
                        current->prev->next = current->next;
                    }

                    if (current->next != NULL) {
                        // in both cases update the next node's prev if it exists
                        current->next->prev = current->prev;
                    }

                    // release memory
                    free(current);
					return 0;
                }

                current = current->next;
            }
            perrorSmash("unalias", "command not found");
			return ERROR;

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
         return my_kill(cmd_after_parse);
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[5]  ) == 0  ) {
        //TODO: add check at parser for #args, and pass -1 if no args
          fg(cmd_after_parse);
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[6] ) == 0  ) {
         bg(cmd_after_parse);
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[7] ) == 0 ) {
         if( quit(cmd_after_parse) == QUITVAL){
             return QUITVAL ; //SIGNAL TO END the program
         }
         return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[8]  ) == 0 ) {
           diff(cmd_after_parse);
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[9] ) == 0  ) {
           alias(cmd_after_parse);
        return 1;
    }
    else if ( strcmp(cmd_after_parse.cmd,cmd_DB[10] ) == 0  ) {
         unalias(cmd_after_parse);
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


cmd* parseCommandExample(char* line){
    cmd cmd_obj;
    cmd_obj.bg = 0;
    cmd_obj.internal = 0;
    cmd_obj.nargs = 0;
    for (int i = 0; i < ARGS_NUM_MAX; ++i) {
        cmd_obj.args[i]=NULL;
    }
    char* delimiters = "\" \t\n="; //parsing should be done by spaces, tabs or newlines
    strcpy(cmd_obj.cmd,strtok(line, delimiters));//read strtok documentation - parses string by delimiters
    if(strcmp(cmd_obj.cmd, "") == 0) return INVALID_COMMAND; //this means no tokens were found, most like since command is invalid
    cmd_obj.args[0] = cmd_obj.cmd; //first token before spaces/tabs/newlines should be command name
   // printf("@be FOR \n");
    for(int i = 1; i < ARGS_NUM_MAX; i++){
       // printf("@interation %d",i);
        cmd_obj.args[i] = strtok(NULL, delimiters); //first arg NULL -> keep tokenizing from previous call
        if(!cmd_obj.args[i])
            break;
        cmd_obj.nargs++;
    }
        int num_cmd=0;
        int start_of_cmd=0;
        int end_of_cmd=-1;
        int old_num_cmd;
        cmd cmd_obj_tmp;
        strcpy(cmd_obj_tmp.cmd, "");
        for (int i = 0; i < ARGS_NUM_MAX; ++i) {
            cmd_obj_tmp.args[i]=NULL;
            }

        for (int i = 0; i < ARGS_NUM_MAX; ++i) {
            old_num_cmd = num_cmd;

            if ((cmd_obj.args[i] != NULL) && (strcmp(cmd_obj.args[i], "&&") == 0)) {
                cmd_obj_tmp.bg = 0;
                end_of_cmd = i;
                cmd_obj_tmp.nargs = end_of_cmd - start_of_cmd - 1;
                strcpy(cmd_obj_tmp.cmd, cmd_obj.args[start_of_cmd]);
                for (int k = i - 1; k >= start_of_cmd; k--) {
                   /*  strcpy(cmd_obj_tmp.args[k - start_of_cmd], cmd_obj.args[k]);*/
                   cmd_obj_tmp.args[k - start_of_cmd]=cmd_obj.args[k];
                }

                //start_of end of

                for (int j = 0; j < 11; j++) {
                    if ( strcmp(cmd_DB[j], cmd_obj_tmp.cmd) == 0 ) {
                        cmd_obj_tmp.internal = 1;
                        break;
                    }
                }
                build_cmd_full(&cmd_obj_tmp);
                // check if cmd is aliased - if there was alias, we will update the cmd_list here
                if (cmd_obj_tmp.internal == 0) {
                    printf("alias cehck \n");
                    list *current = head_alias_list;
                    while (current != NULL) {
                        if ((current->alias != NULL) && (strcmp(current->alias, cmd_obj_tmp.cmd) == 0)) {
                            int j = 0;
                            while (current->og_cmd_list[j].bg != ERROR) {
                                cmd_list[num_cmd] = current->og_cmd_list[j];
                                j++;
                                num_cmd++;

                            }
                            break;
                        }
                        current = current->next;
                    }
                }
                start_of_cmd = end_of_cmd + 1;
                // only if there wasnt alias
                if (old_num_cmd == num_cmd) {
                    cmd_list[num_cmd] = cmd_obj_tmp;
                    num_cmd++;
                }
            }
        }
         if (num_cmd!=0){
             cmd_obj_tmp.bg = 0;
             end_of_cmd = cmd_obj.nargs + 1 ;
             cmd_obj_tmp.nargs = end_of_cmd - start_of_cmd - 1;
             strcpy(cmd_obj_tmp.cmd, cmd_obj.args[start_of_cmd]);
             for (int k = cmd_obj.nargs ; k >= start_of_cmd; k--) {
                 cmd_obj_tmp.args[k - start_of_cmd]=cmd_obj.args[k];
             }

             //start_of end of

             for (int j = 0; j < 11; j++) {
                 if (strcmp(cmd_DB[j], cmd_obj_tmp.cmd) == 0) {
                     printf("@ me thinkj %s is internal\n",cmd_obj_tmp.cmd);
                     cmd_obj_tmp.internal = 1;
                     break;
                 }
             }
             build_cmd_full(&cmd_obj_tmp);

             // check if cmd is aliased - if there was alias, we will update the cmd_list here
             if (cmd_obj_tmp.internal == 0) {
                 list *current = head_alias_list;
                 while (current != NULL) {
                     printf("insde\n");
                     if ((current->alias != NULL) && (strcmp(current->alias, cmd_obj_tmp.cmd) == 0)) {
                         int j = 0;
                         while (current->og_cmd_list[j].bg != ERROR) {
                             cmd_list[num_cmd] = current->og_cmd_list[j];
                             j++;
                             num_cmd++;
                         }
                         break;
                     }
                     current = current->next;
                 }
             }
             start_of_cmd = end_of_cmd + 1;
             // only if there wasnt alias
             if (old_num_cmd == num_cmd) {
                 cmd_list[num_cmd] = cmd_obj_tmp;
                 num_cmd++;
             }
         }
        if (num_cmd==0){ //single cmd
            for (int i = 0; i < 11; i++) {
                if ((cmd_obj.cmd != NULL) && (strcmp(cmd_DB[i], cmd_obj.cmd) == 0)) {
                    cmd_obj.internal = 1;
                    break;
                }
            }
            // check if cmd is aliased - if there was alias, we will update the cmd_list here
            if (cmd_obj.internal == 0) {
                printf("alias cehck \n");
                list *current = head_alias_list;
                while (current != NULL) {
                    if ((current->alias != NULL) && (strcmp(current->alias, cmd_obj.cmd) == 0)) {
                        int j = 0;
                        while (current->og_cmd_list[j].bg != ERROR) {
                            cmd_list[num_cmd] = current->og_cmd_list[j];
                            j++;
                            num_cmd++;
                        }
                        break;
                    }
                    current = current->next;
                }
            }

            // TODO end PROBLEM  */

            // printf("$$$$$$$$$$$$$$");

            if (cmd_obj.args[cmd_obj.nargs] != NULL && strcmp(cmd_obj.args[cmd_obj.nargs], "&") == 0) {
                cmd_obj.bg = 1;
                cmd_obj.args[cmd_obj.nargs]=NULL;
                cmd_obj.nargs--;
            }

            build_cmd_full(&cmd_obj);
            cmd_list[num_cmd] = cmd_obj;

        }
/*
    printf("@@@ cmd_obj @@@@\n");
    for (int i = 0; i < cmd_obj.nargs+1; i++) {
        printf("Arg %d: %s\n", i, cmd_obj.args[i]);
    }
    printf("cmd is: %s \n",cmd_obj.cmd);
    printf("nargs: %d \n",cmd_obj.nargs);
    printf("internal: %d \n",cmd_obj.internal);
    printf("bg: %d \n",cmd_obj.bg);
*/
// PRINTS
  /*  for (int i = 0; i < ARGS_NUM_MAX; i++) {
        printf("cmd list member #%d\n",i);
        printf("cmd is: %s \n",cmd_list[i].cmd);
        printf("nargs: %d \n",cmd_list[i].nargs);
        printf("internal: %d \n",cmd_list[i].internal);
        printf("bg: %d \n",cmd_list[i].bg);
        for (int J = 0; J < cmd_list[i].nargs+1; J++) {
            printf("Arg %d: %s\n", J, cmd_list[i].args[J]);
        }
    }*/
       return cmd_list;
}












