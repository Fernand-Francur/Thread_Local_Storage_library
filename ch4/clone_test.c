#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "tls.h"

/* How many threads (aside from main) to create */
#define THREAD_CNT 1

/* Each counter goes up to a multiple of this value. If your test is too fast
 * use a bigger number. Too slow? Use a smaller number. See the comment about
 * sleeping in count() to avoid this size-tuning issue.
 */
#define COUNTER_FACTOR 1000000
#define BOUND_OFFSET 0

pthread_t threads[THREAD_CNT];

/* Waste some time by counting to a big number.
 *
 * Alternatively, introduce your own sleep function to waste a specific amount
 * of time. But make sure it plays nice with your scheduler's interrupts (HINT:
 * see the man page for nanosleep, and its possible ERROR codes).
 */
void *count(void *arg)
{
    int create = tls_create(8000);
    int str_length = 20;
    char read_buf[str_length];
    char read_buf2[str_length];
    char read_one[str_length];
    if (create == 0)
    {
        char buffer[20];
        strcpy(buffer, "Written by thread");
        int write = tls_write(BOUND_OFFSET, 20, buffer);

        if (write == 0)
        {
            printf("Write success\n");

            
            int read = tls_read(BOUND_OFFSET, str_length, read_buf);

            if (read == 0)
            {
                printf("Read success: %s, %ld\n", read_buf, pthread_self());
            }
        }
    }
    int i = 0;

    while (i != 35 * COUNTER_FACTOR)
    {
        i++;

        if (i == 10 * COUNTER_FACTOR) {
          
            int read = tls_read(BOUND_OFFSET, str_length, read_buf2);

            if (read == 0)
            {
                printf("Read after clone success: %s, %ld\n", read_buf2, pthread_self());
                if (strncmp(read_buf,read_buf2,str_length) != 0) {
                  perror("ERROR: Main has written to original TLS without COW");
                  exit(1);
                }

            }
           
        }

        if (i == 12 * COUNTER_FACTOR) {
            char one[2];
            strcpy(one, "E");
            int write2 = tls_write(4096, 1, one);

        if (write2 == 0)
        {
            printf("Write of one success\n");

            
            int read2 = tls_read(4096, str_length, read_one);

            if (read2 == 0)
            {
                printf("Read of one success: %s, %ld\n", read_one, pthread_self());
            }
        }
        }
        if (i == 30 *COUNTER_FACTOR) {
          int read2 = tls_read(4096, str_length, read_one);

            if (read2 == 0)
            {
                printf("Read of one success twice: %s, %ld\n", read_one, pthread_self());
            }
        }

        if (i % (2 * COUNTER_FACTOR) == 0)
        {
            printf("Thread: %ld\n", pthread_self());
        }
    }

    tls_destroy();
    return 0;
}

int main(int argc, char **argv) {
  int i;
  for(i = 0; i < THREAD_CNT; i++) {
    pthread_create(&threads[i], NULL, count, NULL);
  }
  
  char read_buf4[20];
  char read_one[20];
    int j = 0;

    while (j != 50 * COUNTER_FACTOR)
    {
      j++;

      if (j == 5 * COUNTER_FACTOR) {
        int clone = tls_clone(threads[0]);

        if (clone == 0)
        {              
          printf("Clone success\n");
          char read_buf[20];
          int read = tls_read(BOUND_OFFSET, 20, read_buf);

          if (read == 0)
          {
            printf("Clone Read success: %s, %ld\n", read_buf, pthread_self());  
          }

          char buffer[20];
          strcpy(buffer, "Written by main");
          int write = tls_write(BOUND_OFFSET, 20, buffer);

          if (write == 0)
          {
            printf("Write to clone success\n");

            char read_buf3[20];
            int read = tls_read(BOUND_OFFSET, 20, read_buf3);

            if (read == 0)
            {
                printf("Read from clone success: %s, %ld\n", read_buf3, pthread_self());
            }
        }
      }


      }

        if (j == 20 * COUNTER_FACTOR) {
          
            int read3 = tls_read(4096, 20, read_buf4);
            printf("read 3 = %d\n", read3);

            if (read3 == 0)
            {
                printf("Read after one written by thread in main: %s, %ld\n", read_buf4, pthread_self());
                // if (strncmp(read_buf,read_buf2,str_length) != 0) {
                //   perror("ERROR: Main has written to original TLS without COW");
                //   exit(1);
                // }

            }
                 char one[2];
            strcpy(one, "B");
            int write2 = tls_write(4096, 1, one);

        if (write2 == 0)
        {
            printf("Write of one success in main\n");

            
            int read2 = tls_read(4096, 20, read_one);

            if (read2 == 0)
            {
                printf("Read of one success in main: %s, %ld\n", read_one, pthread_self());
            }
        }
        }
           
        

      if (j % (2 * COUNTER_FACTOR) == 0)
      {
        printf("Thread: %ld\n", pthread_self());
      }        
  }

  return 0;
}