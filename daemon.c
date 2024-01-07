#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<signal.h>
#include<unistd.h>
#include<pthread.h>
#include "utils.h"
#include "constants.h"
#include "instructions.h"

//this program creates a new thread of execution for every job analysis required from the input
//the data structure used for arranging the threads in order of their priorities is a linked list
//each job has a mutex associated for preventing race conditions when one or more threads of execution 
//try to access the same shared files
//to get a better understanding of what these threads actually do
//lets consider 2 jobs of analysis:
//the first one has priority 3 and the second one has priority 1, these jobs handle different tasks according to the requirements of the problem
//before the threads get to finish their tasks the user add a new job with priority 2
//this new job is different from the others and therefore creates a new thread
//since it has a higher priority than the last one and a lower priority than the first one it needs to be the second one in the line
//we want to insert the thread in the linked list and in order to prevent any unexpected behaviour from threads 1 & 2 we use their mutex to lock their execution for a while

void write_solution_to_da(char * solution){
    int fd = open (output_from_daemon, O_CREAT, O_TRUNC | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
    write(fd, solution, strlen(solution));
    close(fd);
}

void process_task_from_da(){
    debug_message("Processing the task from da.c utility...\n");
    int fd = open(da_pid_path, O_RDONLY);
    char *pid_da_string = (char*)malloc(10);
    read(fd, pid_da_string,10);
    close(fd);

    char *result = (char*)RESULT_SIZE;
    char *instruction = (char*)malloc(500);
    fd = open(input_from_user, O_RDONLY);
    char *s = RESULT_SIZE;
    return_solution(instruction, result); //function from process_instructions.h that processes the socution
    sprintf(s, "Final solution from daemon solution: %s", result);
    debug_message(s);
    write_solution_to_da(result);

    //send back the signal to da to announce that the daemon finished
    int pid = atoi(pid_da_string);
    kill(pid, SIGUSR2);    
}


void initialization(){
    if(pthread_mutex_init(&mutex_lock, NULL)!=0){
        debug_message("Error at assigning the thread mutex\n");
    }
    dir = malloc(sizeof(struct directory));
    dir_hash_init(dir,10); //allocated space for the initial analysis of 10 directories
    thread_list_init();
    signal(SIGUSR1, process_task_from_da);
}

write_daemon_pid(){
    create_dir_if_needed(daemon_pid_path);
    int fd = open(da_pid_path, O_CREAT|O_TRUNC|O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
    if(fd < 0){
        perror("Couldn't open daemon input file\n");
        return errno;
    }
    char *pid_daemon_string = malloc(10);
    int pid = getpid();
    sprintf(pid_daemon_string, "%d", pid);
    close(fd);
}

int main(){

    initialization();
    write_daemon_pid();

    struct thread *current_thread = (struct thread*)malloc(sizeof(struct thread));
    void *result;
    while(1){

        current_thread = *threads_list_head; //list of threads
        while(current_thread!=NULL){
            //while the thread is still runnning
            pthread_join(*current_thread->thr, &result); //each thread puts its result in the res variable, the daemons waits for all the threads to finish via pthread_join
            current_thread = current_thread->next;
        }
    }

    return 0;

}
