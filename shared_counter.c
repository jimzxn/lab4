#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>


struct shared_val {
    int value;
    pthread_mutex_t mutex;
};

int main(){
    int shm_id;
    struct shared_val *shm_ptr;
    pid_t pid;
    pthread_mutexattr_t attr;
    
    shm_id = shmget(IPC_PRIVATE, sizeof(struct shared_val), IPC_CREAT | 0666); // created shared mem region
    assert(shm_id >= 0); // error check memory creation
    shm_ptr = (struct shared_val *) shmat(shm_id, NULL, 0); // attach memory
    assert(shm_ptr != (struct shared_val *) -1); // error check memory attachment
    shm_ptr->value = 0; // set value in shared memory

    // init mutex
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shm_ptr->mutex), &attr);
    
    printf("value of shared value before fork is %d\n", shm_ptr->value);
    pid = fork();
    assert(pid >= 0);
    if (pid == 0){ // child
	// note child has access to shared memory right away
	for (int i = 0; i < 10000; i++){
	    pthread_mutex_lock(&(shm_ptr->mutex));
	    shm_ptr->value++;
	    pthread_mutex_unlock(&(shm_ptr->mutex));
	}
	//printf("child's shared value: %d\n", shm_ptr->value);
	shmdt(shm_ptr); // IMPORTANT: detach before exiting
	exit(0);
    } else { // parent
	for (int i = 0; i < 10000; i++){
	    pthread_mutex_lock(&(shm_ptr->mutex));
	    shm_ptr->value++;
	    pthread_mutex_unlock(&(shm_ptr->mutex));
	}
	waitpid(pid, NULL, 0); // wait for child to finish
	printf("parent's shared value after child: %d\n", shm_ptr->value); // 20000
	shmdt(shm_ptr); // detach memory before exiting
	exit(0);
    }
}
