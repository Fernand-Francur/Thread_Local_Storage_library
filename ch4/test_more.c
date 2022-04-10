#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "tls.h"

/* How many threads (aside from main) to create */
#define THREAD_CNT 125

/* Each counter goes up to a multiple of this value. If your test is too fast
 * use a bigger number. Too slow? Use a smaller number. See the comment about
 * sleeping in count() to avoid this size-tuning issue.
 */
#define COUNTER_FACTOR 10000000

pthread_t threads[THREAD_CNT];

/* Waste some time by counting to a big number.
 *
 * Alternatively, introduce your own sleep function to waste a specific amount
 * of time. But make sure it plays nice with your scheduler's interrupts (HINT:
 * see the man page for nanosleep, and its possible ERROR codes).
 */
void *count(void *arg) 
{
    unsigned long int thread_num = (unsigned long int) arg;
    
    int clone = tls_clone(threads[0]);
    
    if (clone == 0)
    {
        //printf("Clone success in thread %ld\n", thread_num);
        char read_buf[20];
        int read = tls_read(4095, 20, read_buf);

        if (read == 0)
        {
            printf("Clone read success in thread %ld: %s, %ld\n", thread_num, read_buf, pthread_self());
        }
    }
    
    int i = 0;
    
    while (i != COUNTER_FACTOR)
    {
        i++;
    }
    tls_destroy();
}

/*
 * Expected behavior: THREAD_CNT number of threads print increasing numbers
 * in a round-robin fashion. The first thread finishes soonest, and the last
 * thread finishes latest. All threads are expected to reach their maximum
 * count.
 *
 * This isn't a great test for `make check` automation, since it doesn't
 * actually fail when things go wrong. Consider adding lock() and unlock()
 * functions to let your threads "pause" the scheduler occasionally to
 * compare the current state of each thread's counters (you may also need to
 * have an array of per-thread counters). See man pages for sigprocmask,
 * sigemptyset, and sigaddset.
 */
int main(int argc, char **argv) {
    //int create = tls_create(4096);
    threads[0] = pthread_self();
    
    int create = tls_create(1000 * 4096);
    
    if (create == 0)
    {
        printf("Main create success\n");
        char buffer[20];
        strcpy(buffer, "I'll get cloned.");
        int write = tls_write(4095, 20, buffer);

        if (write == 0)
        {
            printf("Main write success\n");
        }
    }
    
    int i;
    for(i = 0; i < THREAD_CNT; i++) 
    {
        pthread_create(&threads[i + 1], NULL, count, (void*)(intptr_t)(i + 1));
    }
    
    int j = 0;
    
    while (j != COUNTER_FACTOR * 2)
    {
        j++;
    }

  return 0;
}