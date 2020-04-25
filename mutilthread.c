#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

int excutetime = 30;
int excutecount = 10000;
pthread_t one[2];
pthread_t two[2];
pthread_t reprot;
struct sighandler
{
    int send_sigcount_1;
    int send_sigcount_2;
    int recive_sigcount_1;
    int recive_sigcount_2;
    int total_signal;
    pthread_mutex_t lock;
    int mode;
    time_t starttime;
} sighandler;

struct sighandler *shm_ptr;

void *signalreciveing_1()
{

    sigset_t sigset;
    sigemptyset(&sigset); // initalize set to empty
    sigaddset(&sigset, SIGUSR2);
    sigprocmask(SIG_BLOCK, &sigset, NULL); // modify mask
    sigemptyset(&sigset);                  // initalize set to empty
    sigaddset(&sigset, SIGUSR1);           // add SIGINT to set
    int return_val = 0;
    int signal;
    while (true)
    {
        return_val = sigwait(&sigset, &signal);
        if (signal == SIGUSR1)
        {
            pthread_mutex_lock(&shm_ptr->lock);
            shm_ptr->recive_sigcount_1++;
            pthread_mutex_unlock(&shm_ptr->lock);
        }
    }
}
void *signalreciveing_2()
{

    sigset_t sigset;
    sigemptyset(&sigset); // initalize set to empty
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL); // modify mask
    sigemptyset(&sigset);                  // initalize set to empty
    sigaddset(&sigset, SIGUSR2);           // add SIGINT to set
    int return_val = 0;
    int signal;
    while (true)
    {
        return_val = sigwait(&sigset, &signal);
        if (signal == SIGUSR2)
        {
            pthread_mutex_lock(&shm_ptr->lock);
            shm_ptr->recive_sigcount_2++;
            pthread_mutex_unlock(&shm_ptr->lock);
        }
    }
}
void block()
{
    sigset_t sigset;
    sigemptyset(&sigset);        // initalize set to empty
    sigaddset(&sigset, SIGUSR1); // add SIGINT to set
    sigaddset(&sigset, SIGUSR2);
    sigprocmask(SIG_BLOCK, &sigset, NULL); // modify mask
}
void *thread_gene()
{
    block();
    if (shm_ptr->mode == 1)
    {
        while (1)
        {
            struct timespec finish;
            clock_gettime(CLOCK_REALTIME, &finish);
            int wait = ((rand() % 10) + 1) * 1000;
            usleep(wait);
            int sigrand = rand() % 2;

            if (sigrand == 0)
            {

                pthread_kill(one[0], SIGUSR1);
                pthread_kill(one[1], SIGUSR1);
                pthread_kill(reprot, SIGUSR1);
                pthread_mutex_lock(&shm_ptr->lock);
                shm_ptr->send_sigcount_1++;
                pthread_mutex_unlock(&shm_ptr->lock);
            }
            else
            {
                pthread_kill(two[0], SIGUSR2);
                pthread_kill(two[1], SIGUSR2);
                pthread_kill(reprot, SIGUSR2);
                pthread_mutex_lock(&shm_ptr->lock);
                shm_ptr->send_sigcount_2++;
                pthread_mutex_unlock(&shm_ptr->lock);
            }

            //update

            if ((finish.tv_sec - shm_ptr->starttime) > 30)
            {
                pthread_exit(0);
            }
        }
    }
    else
    {
        for (size_t i = 0; i < 300; i++)
        {
            int sigrand = rand() % 2;

            if (sigrand == 0)
            {
                pthread_kill(one[0], SIGUSR1);
                pthread_kill(one[1], SIGUSR1);
                pthread_kill(reprot, SIGUSR1);
                pthread_mutex_lock(&shm_ptr->lock);
                shm_ptr->send_sigcount_1++;
                pthread_mutex_unlock(&shm_ptr->lock);
            }
            else
            {
                pthread_kill(two[0], SIGUSR2);
                pthread_kill(two[1], SIGUSR2);
                pthread_kill(reprot, SIGUSR2);
                pthread_mutex_lock(&shm_ptr->lock);
                shm_ptr->send_sigcount_2++;
                pthread_mutex_unlock(&shm_ptr->lock);
            }
        }
        pthread_exit(0);
    }
}
void *reprot_thread()
{
    sigset_t sigset;
    sigemptyset(&sigset);        // initalize set to empty
    sigaddset(&sigset, SIGUSR2); // add SIGINT to set
    sigaddset(&sigset, SIGUSR1);
    int return_val = 0;
    int signal;

    while (true)
    {
        return_val = sigwait(&sigset, &signal);
        pthread_mutex_lock(&shm_ptr->lock);
        shm_ptr->total_signal++;
        pthread_mutex_unlock(&shm_ptr->lock);
        if (shm_ptr->total_signal >= 10)
        {
            pthread_mutex_lock(&shm_ptr->lock);
            shm_ptr->total_signal = 0;
            pthread_mutex_unlock(&shm_ptr->lock);
            printf("for this 10 signal signal one have :%d   signal two have :%d\n", shm_ptr->recive_sigcount_1, shm_ptr->send_sigcount_2);
        }
    }
}
int main(int argc, char *argv[])
{
    //share memory set form example (modified)
    int shm_id;
    pthread_mutexattr_t attr;

    shm_id = shmget(IPC_PRIVATE, sizeof(struct sighandler), IPC_CREAT | 0666); // created shared mem region
    assert(shm_id >= 0);                                                       // error check memory creation
    shm_ptr = (struct sighandler *)shmat(shm_id, NULL, 0);                     // attach memory
    assert(shm_ptr != (struct sighandler *)-1);                                // error check memory attachment
    // set value in shared memory
    shm_ptr->recive_sigcount_1 = 0;
    shm_ptr->recive_sigcount_2 = 0;
    shm_ptr->send_sigcount_1 = 0;
    shm_ptr->send_sigcount_2 = 0;
    shm_ptr->total_signal = 0;
    puts("enter mode 1 for time limited, 2 for count limted");
    int select;
    scanf("%d", &select);
    switch (select)
    {
    case 1:
        shm_ptr->mode = 1; //time
        break;
    case 2:
        shm_ptr->mode = 2; //count
    default:
        break;
    }
    struct timespec start;
    clock_gettime(CLOCK_REALTIME, &start);
    shm_ptr->starttime = start.tv_sec;

    // init mutex
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shm_ptr->lock), &attr);
    //standerd share memory code end

    //thread
    block();
    for (size_t i = 0; i < 2; i++)
    {
        pthread_create(&one[i], NULL, signalreciveing_1, NULL);
    }
    pthread_create(&reprot,NULL,reprot_thread,NULL);
    for (size_t i = 0; i < 2; i++)
    {
        pthread_create(&two[i], NULL, signalreciveing_2, NULL);
    }
    pthread_t gene[3];
    for (size_t i = 0; i < 3; i++)
    {
        pthread_create(&gene[i], NULL, thread_gene, NULL);
    }
    for (size_t i = 0; i < 3; i++)
    {
        pthread_join(gene[i], NULL);
    }
    printf("send 1: %d recive 1; %d send 2: %d recive 2 :%d\n", shm_ptr->send_sigcount_1, shm_ptr->recive_sigcount_1, shm_ptr->send_sigcount_2, shm_ptr->recive_sigcount_2);
}