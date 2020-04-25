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

void block()
{
    sigset_t sigset;
    sigemptyset(&sigset);        // initalize set to empty
    sigaddset(&sigset, SIGUSR1); // add SIGINT to set
    sigaddset(&sigset, SIGUSR2);
    sigprocmask(SIG_BLOCK, &sigset, NULL); // modify mask
}
void signal_handler(int sig)
{
    if (sig = SIGUSR1)
    {
        pthread_mutex_lock(&shm_ptr->lock);
        shm_ptr->recive_sigcount_1++;
        printf("%d", 1);
        pthread_mutex_unlock(&shm_ptr->lock);
    }
    else
    {
        pthread_mutex_lock(&shm_ptr->lock);
        shm_ptr->recive_sigcount_2++;
        printf("%d", 2);
        pthread_mutex_unlock(&shm_ptr->lock);
    }
}

void signal_generating()
{
    block();
    srand(shm_ptr->starttime);
    for (size_t i = 0; i < 3; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
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
                        kill(0, SIGUSR1);
                        pthread_mutex_lock(&shm_ptr->lock);
                        shm_ptr->send_sigcount_1++;
                        pthread_mutex_unlock(&shm_ptr->lock);
                    }
                    else
                    {
                        kill(0, SIGUSR2);
                        pthread_mutex_lock(&shm_ptr->lock);
                        shm_ptr->send_sigcount_2++;
                        pthread_mutex_unlock(&shm_ptr->lock);
                    }

                    //update

                    if ((finish.tv_sec - shm_ptr->starttime) > 30)
                    {
                        exit(0);
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < 100; i++)
                {
                    int sigrand = rand() % 2;
                    int signal;
                    if (sigrand == 0)
                    {
                        signal = SIGUSR1;
                        pthread_mutex_lock(&shm_ptr->lock);
                        shm_ptr->send_sigcount_1++;
                        pthread_mutex_unlock(&shm_ptr->lock);
                    }
                    else
                    {
                        signal = SIGUSR2;
                        pthread_mutex_lock(&shm_ptr->lock);
                        shm_ptr->send_sigcount_2++;
                        pthread_mutex_unlock(&shm_ptr->lock);
                    }
                    kill(0, signal);
                }
                exit(0);
            }
        }
    }
    wait(0);
    wait(0);
    wait(0);
}

void signalreciveing_1()
{
    for (size_t i = 0; i < 2; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            sigset_t sigset;
            sigemptyset(&sigset);        // initalize set to empty
            sigaddset(&sigset, SIGUSR1); // add SIGINT to set
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
    }
}
void signalreciveing_2()
{
    for (size_t i = 0; i < 2; i++)
    {
        int pid = fork();
        if (pid == 0)
        {
            sigset_t sigset;
            sigemptyset(&sigset);        // initalize set to empty
            sigaddset(&sigset, SIGUSR2); // add SIGINT to set
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
    }
}
void signalreport()
{
    int pid = fork();
    if (pid == 0)
    {

        sigset_t sigset;
        sigemptyset(&sigset);        // initalize set to empty
        sigaddset(&sigset, SIGUSR2); // add SIGINT to set
        sigaddset(&sigset, SIGUSR1);
        int return_val = 0;
        int signal;

        while (true)
        {
            printf("1\n");
            return_val = sigwait(&sigset, &signal);
            pthread_mutex_lock(&shm_ptr->lock);
            shm_ptr->total_signal++;
            pthread_mutex_unlock(&shm_ptr->lock);
            if (shm_ptr->total_signal >= 10)
            {
                pthread_mutex_lock(&shm_ptr->lock);
                shm_ptr->total_signal = 0;
                pthread_mutex_unlock(&shm_ptr->lock);
                printf("for this 10 signal signal one have :%d   signal two have ;%d", shm_ptr->recive_sigcount_1,shm_ptr->send_sigcount_2);
            }
        }
    }
    wait(0);
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
    //signal
    block();
    //fork create
    srand(start.tv_sec);
    signalreciveing_1();
    signalreciveing_2();
    //signalreport();

    sleep(1);
    signal_generating();

    printf("generated\n");

    printf("send 1: %d recive 1; %d send 2: %d recive 2 :%d\n", shm_ptr->send_sigcount_1, shm_ptr->recive_sigcount_1, shm_ptr->send_sigcount_2, shm_ptr->recive_sigcount_2);

    //end
}