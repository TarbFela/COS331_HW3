#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>


#ifndef Num_Points
#define Num_Points 1000000
#endif
#ifndef Num_Threads
#define Num_Threads 2
#endif

int Total_Hits = 0 ;
int lock = 0 ;
pthread_mutex_t My_Mutex ;
void *Compute (void *Count) ;
pthread_t ThreadID[Num_Threads] ;


int main()
{
    printf("NUMPOINTS: %d\t\tNUMTHREADS: %d\n",Num_Points,Num_Threads);

    int ret ;
    int i ;
    double pi ;
    pthread_mutex_init(&My_Mutex, NULL) ;

    clock_t strt = clock();
    for (i = 0; i < Num_Threads ; i++)
    {
        ret = pthread_create(&ThreadID[i], NULL, Compute, (void *) NULL) ;
    }
    for(i = 0 ; i < Num_Threads ; i++)
        pthread_join(ThreadID[i], NULL) ;
//compute and print out pi.
    clock_t end = clock();
    printf("TIME: %.10f\n",(float)(end-strt)/CLOCKS_PER_SEC);
    printf("PI: %.10f\n",4.0 * (float)Total_Hits / (float)Num_Points);
    FILE *fd;
    fd = fopen("results.csv","a");
    if(fd==NULL) return -1;
    fprintf(fd, "%s, %d, %d, %.10f, %.10f,\n",__FILE_NAME__, Num_Threads, Num_Points, 4.0 * (float)Total_Hits / (float)Num_Points,(float)(end-strt)/CLOCKS_PER_SEC );
    fclose(fd);
}


void *Compute(void *arg) {
    int count = Num_Points / Num_Threads;
    int Local_Hits = 0;
    int i;
//creating a different seed for each thread
    uintptr_t tid = (uintptr_t) pthread_self();
    time_t t = time(NULL);
    unsigned int seedp = (unsigned int) (t ^ (tid >> 16) ^ (tid & 0xFFFF)); // per-thread seed
    double x, y, z;
    for (i = 0; i < count; i++) {
        y = (double) rand_r(&seedp) / RAND_MAX * 2 - 1;
        x = (double) rand_r(&seedp) / RAND_MAX * 2 - 1;
        z = ((x * x) + (y * y));
        if (z <= 1)
            ++Local_Hits;
    }
//entry section
    pthread_mutex_lock(&My_Mutex) ; //entry section
    Total_Hits+=Local_Hits; // critical section
    pthread_mutex_unlock(&My_Mutex) ; //exit section
//exit section
}