#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "tls.h"

/* How many threads (aside from main) to create */
#define THREAD_CNT 40

/* Each counter goes up to a multiple of this value. If your test is too fast
 * use a bigger number. Too slow? Use a smaller number. See the comment about
 * sleeping in count() to avoid this size-tuning issue.
 */
#define COUNTER_FACTOR 1000000
#define BOUND_OFFSET 0
#define TLS_SIZE 10000

pthread_t threads[THREAD_CNT];
pthread_t main_thread;

/* Waste some time by counting to a big number.
 *
 * Alternatively, introduce your own sleep function to waste a specific amount
 * of time. But make sure it plays nice with your scheduler's interrupts (HINT:
 * see the man page for nanosleep, and its possible ERROR codes).
 */
void *count(void *arg)
{
 
    int i;
    while (i != 35 * COUNTER_FACTOR)
    {
        i++;
        if (i == COUNTER_FACTOR * 5) {
          tls_clone(main_thread);

        }

        if (i % (2 * COUNTER_FACTOR) == 0)
        {
            //printf("Thread: %ld\n", pthread_self());
        }
    }

    tls_destroy();
    return 0;
}

int main(int argc, char **argv) {
  int i;
  main_thread = pthread_self();
  for(i = 0; i < THREAD_CNT; i++) {
    pthread_create(&threads[i], NULL, count, NULL);
  }
  
  tls_create(TLS_SIZE);

    int j = 0;

    while (j != 50 * COUNTER_FACTOR)
    {
      j++;


      if (j % (2 * COUNTER_FACTOR) == 0)
      {
        //printf("Thread: %ld\n", pthread_self());
      }        
  }

  return 0;
}