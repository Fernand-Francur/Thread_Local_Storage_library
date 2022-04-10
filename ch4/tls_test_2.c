#include <stdio.h>
#include <string.h>
#include "tls.h"

/* How many threads (aside from main) to create */
#define THREAD_CNT 1

/* Each counter goes up to a multiple of this value. If your test is too fast
 * use a bigger number. Too slow? Use a smaller number. See the comment about
 * sleeping in count() to avoid this size-tuning issue.
 */
#define COUNTER_FACTOR 1000000

pthread_t threads[THREAD_CNT];

/* Waste some time by counting to a big number.
 *
 * Alternatively, introduce your own sleep function to waste a specific amount
 * of time. But make sure it plays nice with your scheduler's interrupts (HINT:
 * see the man page for nanosleep, and its possible ERROR codes).
 */
void *count(void *arg)
{
    int create = tls_create(4096);

    if (create == 0)
    {
        char buffer[20];
        strcpy(buffer, "");
        int write = tls_write(0, 20, buffer);

        if (write == 0)
        {
            printf("Write success\n");

            char read_buf[20];
            int read = tls_read(0, 20, read_buf);

            if (read == 0)
            {
                printf("Read success: %s, %ld\n", read_buf, pthread_self());
            }
        }
    }

    int i = 0;

    while (i != 47 * COUNTER_FACTOR)
    {
        i++;

        if (i == 10 * COUNTER_FACTOR)
        {
            char read_buf[20];
            int read = tls_read(0, 20, read_buf);

            if (read == 0)
            {
                printf("Read success in thread after being cloned: %s, %ld\n", read_buf, pthread_self());

                char buffer[20];
                strcpy(buffer, "write after clone");
                int offset = 40;
                int write = tls_write(offset, 20, buffer);

                if (write == 0)
                {
                    printf("Write success in thread after being cloned\n");

                    char read_buf2[60];
                    int read = tls_read(44, 10, read_buf2);

                    if (read == 0)
                    {
                        printf("Should be length 3 Read success: %s, %ld\n", read_buf2, pthread_self());
                    }
                }
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

    int j = 0;

    while (j != 50 * COUNTER_FACTOR)
    {

        j++;

        if (j == 5 * COUNTER_FACTOR)
        {
            int clone = tls_clone(threads[0]);

            if (clone == 0)
            {
                printf("Clone success\n");
                char read_buf[20];
                int read = tls_read(0, 20, read_buf);

                if (read == 0)
                {
                    printf("Clone Read success: %s, %ld\n", read_buf, pthread_self());
                }
            }
        }

        if (j == 15 * COUNTER_FACTOR)
        {
            char read_buf[20];
            int read = tls_read(0, 20, read_buf);

            if (read == 0)
            {
                printf("Read success after clone and clone write %s, %ld\n", read_buf, pthread_self());
            }
        }

        if (j == 20 * COUNTER_FACTOR)
        {
            char buffer[20];
            strcpy(buffer, "Clone Write");
            int write = tls_write(20, 20, buffer);

            if (write == 0)
            {
                printf("Clone Write success\n");

                char read_buf2[40];
                int read = tls_read(20, 40, read_buf2);

                if (read == 0)
                {
                    printf("Clone Read success: %s, %ld\n", read_buf2, pthread_self());
                }
            }
        }

        if (j == 25 * COUNTER_FACTOR)
        {
            char read_buf2[20];
            int read = tls_read(0, 20, read_buf2);

            if (read == 0)
            {
                printf("Clone Read success: %s, %ld\n", read_buf2, pthread_self());
            }
        }

        if (j % (2 * COUNTER_FACTOR) == 0)
        {
            printf("Thread: %ld\n", pthread_self());
        }
    }

  return 0;
}