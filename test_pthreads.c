#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "testconditions.h"

//500000000


int Total_Hits = 0;
int lock = 0;
int *local_hits;

void *compute_polling (void *arg);
void *compute_nosync(void *arg);
void *compute_mutex(void *arg);

void *compute_separate(void *arg);

const int NPoints_arr[] = {1000000,50000000, 500000000};
const int NThreads_arr[] = {2, 8, 16};
void* functions[] = {compute_polling, compute_nosync, compute_mutex};
char* names[] = {"POLLING","NO SYNC","MUTEX", "ARRAY"};

pthread_mutex_t My_Mutex;



pthread_t *ThreadID;

int run_test(int Num_Points, int Num_Threads, float *pi_result, void *(*fn)(void*))
{
    Total_Hits = 0;
    ThreadID = (pthread_t *)malloc(sizeof(pthread_t) * Num_Threads);
    int ret ;
    int i ;
    double pi ;
    void *ptharg;
    int count_per_thread = Num_Points/Num_Threads;
    ptharg = (void *)(&count_per_thread);
    pthread_mutex_init(&My_Mutex, ptharg) ;
    for (i = 0; i < Num_Threads ; i++)
    {
        ret = pthread_create(&ThreadID[i], NULL, fn, ptharg) ;
    }
    for(i = 0 ; i < Num_Threads ; i++) pthread_join(ThreadID[i], NULL);
    *pi_result = 4.0f * (float)Total_Hits / (float)Num_Points;
    free(ThreadID);
    return 0;
}

int run_test_with_malloc(int Num_Points, int Num_Threads, float *pi_result, void *(*fn)(void*))
{
    Total_Hits = 0;
    ThreadID = (pthread_t *)malloc(sizeof(pthread_t) * Num_Threads);
    int ret ;
    int i ;
    double pi ;
    // define an array of local hit destinations.
    local_hits = (int *)malloc(sizeof(int) * Num_Threads);
    //for(int i=0;i<Num_Threads;i++) local_hits[i] = i;

    // define an array of arrays of arguments
    void **ptharg_arr;
    ptharg_arr = (void **)malloc(sizeof(void *) * Num_Threads);
    for(i=0 ;i<Num_Threads; i++) {
        // argument array (per thread): a count and an index to write to in local hits
        ptharg_arr[i] = (int *)malloc(sizeof(int) * 2);
        int *tmpint; tmpint = (int *)ptharg_arr[i];
        *tmpint = Num_Points/Num_Threads;
        tmpint++;
        *tmpint = i;
    }

    pthread_mutex_init(&My_Mutex, NULL);
    for (i = 0; i < Num_Threads ; i++)
    {
        ret = pthread_create(&ThreadID[i], NULL, fn, ptharg_arr[i]) ;
    }
    for(i = 0 ; i < Num_Threads ; i++) pthread_join(ThreadID[i], NULL);
    for(i = 0; i<Num_Threads; i++) Total_Hits += local_hits[i];

    *pi_result = 4.0f * (float)Total_Hits / (float)Num_Points;

    free(ThreadID);
    for(i=0; i<Num_Threads; i++) free(ptharg_arr[i]);
    free(ptharg_arr);
    return 0;
}



int main() {
    int nnpoints = sizeof(NPoints_arr)/sizeof(NPoints_arr[0]);
    int nnthreads = sizeof(NThreads_arr)/sizeof(NThreads_arr[0]);
    int nntests = nnthreads * nnpoints;
    clock_t timer_start, timer_end;
    float result;

    testconditions_t *tests;

    tests = (testconditions_t *)malloc(sizeof(testconditions_t)*nntests);
    for(int i = 0; i<nnpoints;i++) {
        for(int j = 0; j<nnthreads; j++) {
            tests[i*nnthreads + j].np = NPoints_arr[i];
            tests[i*nnthreads + j].nt = NThreads_arr[j];
        }
    }
    for (int i = 0; i < nntests; i++) {
        for(int j = 0; j<4; j++) {
            result = 0; Total_Hits = 0;
            printf("\n");
            timer_start = clock();
            int npoints = tests[i].np;
            int nthreads = tests[i].nt;
            if(j==3) run_test_with_malloc(npoints,nthreads,&result,compute_separate);
            else run_test(npoints, nthreads, &result, functions[j]);
            timer_end = clock();
            printf("\n");
            //printf("TEST %d\t\tPoints: %d\t\tThreads: %d\n\t\tTIME TAKEN = %.4f\n\t\tCLOCKS TAKEN = %lu\n\t\tPI = %.8f\n\t\tERR = %E\n",
            //       i, npoints, nthreads, (float) (timer_end - timer_start) / CLOCKS_PER_SEC,
            //       timer_end - timer_start, result,
            //       M_PI - result
            //);
            printf("%.10f, %.5f, %d, %d\n",result,(float)(timer_end-timer_start)/CLOCKS_PER_SEC,npoints,nthreads);
        }
    }



}

void *compute_polling(void *arg) {
    printf("POLLING | ");
    int count = *(int *)arg;
    //printf("\nPOLL\n");
    int Local_Hits = 0 ;
    int i;
//creating a different seed for each thread
    uintptr_t tid = (uintptr_t)pthread_self();
    time_t t = time(NULL);
    unsigned int seedp = (unsigned int)(t ^ (tid >> 16) ^ (tid & 0xFFFF)); // per-thread seed
    double x,y,z ;
    for(i=0; i < count ; i++)
    {
        y = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        x = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        z = ((x*x) + (y*y)) ;
        if (z <= 1)
            ++Local_Hits ;
    }
//entry section
    while(__sync_lock_test_and_set(&lock, 1) != 0);
    Total_Hits += Local_Hits; //total hits is shared must protect
    __sync_lock_release(&lock);
//exit section
}

void *compute_nosync(void *arg) {
    printf("NOSYNC | ");
    int count = *(int *)arg;
    //printf("\nNOSYNC\n");
    //printf("\tMY COUNT IS: %d\n", count);
    int Local_Hits = 0 ;
    int i;
//creating a different seed for each thread
    uintptr_t tid = (uintptr_t)pthread_self();
    time_t t = time(NULL);
    float fck = (float)clock();
    unsigned int seedp = (unsigned int)(t ^ (tid >> 16) ^ (tid & 0xFFFF) + (int)fck); // per-thread seed
    double x,y,z ;
    for(i=0; i < count ; i++)
    {
        y = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        x = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        z = ((x*x) + (y*y)) ;
        if (z <= 1)
            ++Local_Hits ;
    }
//entry section
    //while(__sync_lock_test_and_set(&lock, 1) != 0);
    Total_Hits += Local_Hits; //total hits is shared must protect
    //__sync_lock_release(&lock);
//exit section
}

void *compute_mutex(void *arg) {
    printf("MUTEX | ");
    int count = *(int *)arg;
    //printf("MUTEX\n");
    int Local_Hits = 0 ;
    int i;
//creating a different seed for each thread
    uintptr_t tid = (uintptr_t)pthread_self();
    time_t t = time(NULL);
    unsigned int seedp = (unsigned int)(t ^ (tid >> 16) ^ (tid & 0xFFFF)); // per-thread seed
    double x,y,z ;
    for(i=0; i < count ; i++)
    {
        y = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        x = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        z = ((x*x) + (y*y)) ;
        if (z <= 1)
            ++Local_Hits ;
    }
//entry section straight from the slides...
    pthread_mutex_lock(&My_Mutex) ; //entry section
    Total_Hits+=Local_Hits; // critical section
    pthread_mutex_unlock(&My_Mutex) ; //exit section
//exit section
}

void *compute_separate(void *arg) {
    printf("SEPARATE | ");
    int count = *((int *)arg );
    int hits_dest = *((int *)arg + 1);

    int i;
//creating a different seed for each thread
    uintptr_t tid = (uintptr_t)pthread_self();
    time_t t = time(NULL);
    unsigned int seedp = (unsigned int)(t ^ (tid >> 16) ^ (tid & 0xFFFF)); // per-thread seed
    double x,y,z ;
    for(i=0; i < count ; i++)
    {
        y = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        x = (double) rand_r(&seedp)/RAND_MAX *2 - 1 ;
        z = ((x*x) + (y*y)) ;
        if (z <= 1)
            ++local_hits[hits_dest] ;
    }

    //printf("Thread Point Count: %d. Local Points Hit: %d\n",count,local_hits[hits_dest]);
}


