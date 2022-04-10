#include "tls.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

/*It turns out assert.h may not work with pthreads, so we're left with printf failures*/


/*use these constants to modify the tests. LOCKS allow you to turn on or off certain
* tests. The program attempts code in a lock if it is set to 1, and does not if set to 0.
*/
#define NUM_THREADS 2
#define DURDLE 50 * 1000000
#define NUM_COW 5
#define LOCK_ONE 1 //make and destroy many tls's
#define LOCK_TWO 1 //ensure that tls can make tls's of many different sizes
#define LOCK_THREE 1 //test write and reads from a single tls
#define LOCK_FOUR 1 //test cloning and reading from a cloned tls
#define LOCK_FIVE 1//testing COW
#define TEST_ASSERT(x) do { \
	if (!(x)) { \
		fprintf(stderr, "%s:%d: Assertion (%s) failed!\n", \
				__FILE__, __LINE__, #x); \
	       	abort(); \
	} \
} while(0)


int i;
pthread_t pid1, pid2, pid3;
int ret;
int ret2[5];
int lock;
pthread_t cowpids[NUM_COW];
char * clone_data;
/*This test will not tell you if you have memory leaks, but it will help spot unexpected -1 returns and segfaults*/
void * create_many_tls()
{
    
    static int internal_index = 0;
    unsigned int data = getpagesize();    
    /*test 2: make tls's of many different sizes, and destroy them.*/
    if (LOCK_TWO >= 1)
        data = data * i;      

    TEST_ASSERT(tls_create(data) == 0);

    /*waste some time*/
    int k;
    for(k = 0; k < 1000; k++)
        k++;
    
    TEST_ASSERT(tls_destroy() == 0);
    /*test if a thread can remake a tls after destroying its own*/
    TEST_ASSERT(tls_create(data * 2) == 0);

    /*waste some time*/
    
    for(k = 0; k < 10; k++)
        usleep(1);
    
    TEST_ASSERT(tls_destroy() == 0);
    internal_index++;
    printf("thread %d done!\n", internal_index);
    return NULL;
}

void * basic_write_read()
{
    char * data = "Hello, World!";
    char * return_dat = malloc(sizeof(char) * strlen(data));
    
    /*attempt to write to nonexistant tls*/
    TEST_ASSERT(tls_write(0, strlen(data), data) != 0);
    tls_create(getpagesize() * 10);
    
    /*write/read a basic string with no offset*/
    TEST_ASSERT(tls_write(0, strlen(data), data) == 0);
    TEST_ASSERT(tls_read(0, strlen(data), return_dat) == 0);
    TEST_ASSERT(strcmp(data, return_dat) == 0);
    
    free(return_dat);
    /*write/read with an offset*/
    data = "Round 2: tls_write Boogaloo";
    
    return_dat = malloc(sizeof(char) * strlen(data));
    
    TEST_ASSERT(tls_write(500, strlen(data), data) == 0);
    TEST_ASSERT(tls_read(500, strlen(data), return_dat) == 0);
    TEST_ASSERT(strcmp(data, return_dat) == 0);
    
    free(return_dat);
    
    /*write/read that overlaps from one page to another*/
    data = "Round 3: Really, I have no idea why this randomly segfaults. It honestly makes no sense at all";
    
    TEST_ASSERT(tls_write((getpagesize()-5), strlen(data), data) == 0);
    TEST_ASSERT(tls_read(getpagesize()-5, strlen(data), return_dat) == 0);
    TEST_ASSERT(strcmp(data, return_dat) == 0);
    
    /*write/read a single byte somewhere in a random page*/
    data = "E";
    
    TEST_ASSERT(tls_write(getpagesize() * 9 + 50, strlen(data), data) == 0);
    TEST_ASSERT(tls_read(getpagesize() * 9 + 50, strlen(data), return_dat) == 0);
    TEST_ASSERT(strcmp(data, return_dat) == 0);    
    
    /*try and read/write out of bounds*/
    data = "THIS SHOULD NOT WOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOORK";
    
    TEST_ASSERT(tls_write(getpagesize() * 10 - 1, strlen(data), data) != 0);
    TEST_ASSERT(tls_read(getpagesize() * 10 - 1, strlen(data), return_dat) != 0);
    
    /*write/read a very large buffer that spans 3 pages*/
    char * long_data = malloc(getpagesize()*2);
    int k;
    for (k = 0; k < strlen(data); k++)
    {
        long_data[k] = 'e';
    }
    
    TEST_ASSERT(tls_write(getpagesize() / 2, strlen(data), long_data) == 0);
    TEST_ASSERT(tls_read(getpagesize() / 2, strlen(data), return_dat) == 0);
    TEST_ASSERT(strcmp(long_data, return_dat) == 0);
    
    TEST_ASSERT(tls_destroy() == 0);
    
    /*try to write to destroyed tls*/
    TEST_ASSERT(tls_write(0, strlen(data), data) != 0);
    printf("WHYYYY\n");
    free(long_data);
    printf("WHYYYY\n");
    return NULL;
}


void * clone_test()
{
    char * data = "hello, world!";
    if(pthread_self() == pid1)
    {
        /*make a large tls*/
        TEST_ASSERT(tls_create(getpagesize() * 100) == 0);
        tls_write(getpagesize() * 50 + 5, strlen(data), data);
        ret = 1;
    } else
    {
        char * return_data = malloc(sizeof(char) * 13);
        while(ret == 0)
        {
            usleep(100);
        }
        /*can clone a tls twice*/
        TEST_ASSERT(tls_clone(pid1) == 0);
        
        /*clonee's can read from a cloned tls*/
        TEST_ASSERT(tls_read(getpagesize() * 50 + 5, strlen(data), return_data) == 0);
        TEST_ASSERT(strcmp(data, return_data) == 0);
        /*and destroy it*/
        TEST_ASSERT(tls_destroy() == 0);
    }
    
    return NULL;
}

void * many_clone_test()
{
    for(i = 0; i < NUM_THREADS; i++)
    {
        /*many tls's can clone*/
        TEST_ASSERT(tls_clone(pid1) == 0);

        /*and read*/
        char * data = "hello, world!";
        char * return_data = malloc(sizeof(data));
        TEST_ASSERT(tls_read(getpagesize() * 50 + 5, strlen(data), return_data) == 0);
        TEST_ASSERT(strcmp(data, return_data) == 0);        
        
        free(return_data);
        /*and destroy*/
        TEST_ASSERT(tls_destroy() == 0);
    }
    
    return NULL;
}


void * cow_test()
{
    char * data[5];
    data[0] = "Hello, World!";
    data[1] = "E";
    data[2] = "According to all known laws of physics";
    data[3] = "The bumble bee should not be able to fly";
    data[4] = "Did anyone ever *actually* watch that horrid movie?";
    /*make 1 tls and write some stuff to it*/
    if(pthread_self() == cowpids[0])
    {
        TEST_ASSERT(tls_create(getpagesize() * 5) == 0); //what, you thought we'd stop testing the create function?
        TEST_ASSERT(tls_write(0, strlen(data[0]), data[0]) == 0);
        TEST_ASSERT(tls_write(getpagesize() * 3 + 55, strlen(data[1]), data[1]) == 0);
        ret2[0] = 1;
        
        while(ret2[1] != 1)
        {
            usleep(1);
        }
        
        /*waste some time so all of the clones can read data[1]*/
        int wait = 0;
        while(wait <= 1000)
            wait++;
        
        /*Force a COW*/
        TEST_ASSERT(tls_write(getpagesize() * 3 + 55, strlen(data[2]), data[2]) == 0);
        clone_data = calloc(1,sizeof(char) * strlen(data[2]));
        TEST_ASSERT(tls_read(getpagesize() * 3 + 55, strlen(data[2]), clone_data) == 0);
        
        ret2[2] = 1;
        
    }
    /*rest of the cowpids copy and start doing... schenanigans*/
    else
    {
        char * return_dat;
        char * return_dat2;
        while(ret2[0] != 1)
        {
            usleep(1);
        }
        
        /*test a basic no offset read from a clone*/
        return_dat = calloc(1, sizeof(char) * strlen(data[0]));
        TEST_ASSERT(tls_clone(cowpids[0]) == 0);
        TEST_ASSERT(tls_read(0, strlen(data[0]), return_dat) == 0);
        TEST_ASSERT(strcmp(return_dat, data[0]) == 0);
        free(return_dat);
        
        /*test an offset read from a clone*/
        return_dat2 = malloc(sizeof(char) * strlen(data[1]));
        TEST_ASSERT(tls_read(getpagesize() * 3 + 55, strlen(data[1]), return_dat2) == 0);
        TEST_ASSERT(strcmp(return_dat2, data[1]) == 0);
        free(return_dat2);
        ret2[1] = 1;

        
        while(ret2[2] != 1)
            usleep(1);
        
        int wait = 0;
        while(wait <= 1000)
            wait++;       
        
        /*try and read from a COW'd page*/
        return_dat2 = calloc(1,sizeof(char) * strlen(clone_data));
        TEST_ASSERT(tls_read(getpagesize() * 3 + 55, strlen(data[2]), return_dat2) == 0);
        TEST_ASSERT(strcmp(return_dat2, clone_data) != 0);                    
    }
    
    tls_destroy();
    return NULL;
}


int main()
{
    lock = 0;
    pthread_t pids[NUM_THREADS];
    /*test 1: I can create and destroy many tls's*/
    if(LOCK_ONE >= 1)
    {
        printf("---Starting Creation/Deletion Testing---\n");
        pthread_t threads[NUM_THREADS];

        for(i = 0; i < NUM_THREADS; i++)
            pthread_create(&threads[i], NULL, create_many_tls, NULL);
    }
    
    
    /*finish first set of tests before going onto next*/
    int j;
    int p = 0;
    for (j = 0; j < DURDLE; j++)
    {
        if(j % 10000000 == 0)
            printf("...\n");
        p++;
    }

    
    /*test 3: I can read and write from multiple points in a single tls*/
    if(LOCK_THREE >= 1)
    {
        printf("---Starting Write Testing---\n");
        pthread_t pid;
        pthread_create(&pid, NULL, basic_write_read, NULL);
    }
    
    for (j = 0; j < DURDLE; j++)
    {
        if(j % 10000000 == 0)
            printf("...\n");
        p++;
    }
    
    printf("done!\n");
    /*test 4: I can clone a tls, potentially many times*/
    if(LOCK_FOUR >= 1)
    {
        ret = 0;
        printf("---Starting Clone Testing---\n");
        pthread_create(&pid1, NULL, clone_test, NULL);
        
        /*waste some time*/
        int k;
        for(k = 0; k < 100000; k++)
            k++;

        pthread_create(&pid2, NULL, clone_test, NULL);
        //pthread_create(&pid3, NULL, clone_test, NULL);
        
        
        for (j = 0; j < DURDLE; j++)
        {
            if(j % 10000000 == 0)
                printf("...\n");
            p++;
        } 
        
        printf("---Starting Many Clone Testing---\n");
        for (k = 0; k< NUM_THREADS; k++)
        {
            pthread_create(&pids[k], NULL, many_clone_test, NULL);            
        }
        

    }

    for (j = 0; j < DURDLE; j++)
    {
        if(j % 10000000 == 0)
            printf("...\n");
        p++;
    }
    
    if(LOCK_FIVE >= 1)
    {
        printf("---Starting COW testing ---\n");
        int z;
        for(z = 0; z < NUM_COW; z++)
        {
            int reti;
            for(reti = 0; reti < 5; reti++)
                ret2[reti] = 0;
            pthread_create(&cowpids[z], NULL, cow_test, NULL);
        }
        
    }
    
    for (j = 0; j < DURDLE; j++)
    {
        if(j % 10000000 == 0)
            printf("...\n");
        p++;
    }
    
    return 0;
}