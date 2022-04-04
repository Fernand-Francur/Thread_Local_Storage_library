#include "tls.h"
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

/*
 * This is a good place to define any data structures you will use in this file.
 * For example:
 *  - struct TLS: may indicate information about a thread's local storage
 *    (which thread, how much storage, where is the storage in memory)
 *  - struct page: May indicate a shareable unit of memory (we specified in
 *    homework prompt that you don't need to offer fine-grain cloning and CoW,
 *    and that page granularity is sufficient). Relevant information for sharing
 *    could be: where is the shared page's data, and how many threads are sharing it
 *  - Some kind of data structure to help find a TLS, searching by thread ID.
 *    E.g., a list of thread IDs and their related TLS structs, or a hash table.
 */
typedef struct thread_local_storage {
  pthread_t tid;
  unsigned int size;
  unsigned int page_num;
  struct page **pages;
} TLS;

struct page {
  unsigned int address;
  int ref_count;
};

struct tid_tls_pair {
  pthread_t tid;
  TLS *tls;
};
/*
 * Now that data structures are defined, here's a good place to declare any
 * global variables.
 */
#define MAX_THREAD_COUNT 128
#define PAGE_SIZE 4096

static struct tid_tls_pair tid_tls_pairs[MAX_THREAD_COUNT];
static int thread_count = 0;
/*
 * With global data declared, this is a good point to start defining your
 * static helper functions.
 */

/*
 * Lastly, here is a good place to add your externally-callable functions.
 */ 
void tls_handle_page_fault(int sig, siginfo_t *si, void *context) {
  int page_size = getpagesize();
  printf("Page size = %d\n", page_size);

  //int p_fault = ((unsigned int) si->si_addr) & ~(page_size -1);
}

void tls_init() {
  struct sigaction sa;
  int page_size = getpagesize();
  printf("Page size = %d\n", page_size);

  sa.sa_flags = SA_SIGINFO;
  //sa.sa_sigaction = tls_handle_page_fault;
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL); 
}

int tls_create(unsigned int size) {
  pthread_t tid = pthread_self();
  int page_size = getpagesize();
  unsigned int page_count = (size + (page_size-1))/page_size;
  printf("page count = %d\n", page_count);
  printf("page size = %d\n", page_size);
  if (thread_count == 0) {
    tls_init();
  }
  if (size < 0) {
    perror("ERROR: Size is less than zero");
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {
    if (tid== tid_tls_pairs[i].tid) {
      perror("ERROR: LSA for this thread is already created");
      return -1;
    }
  }
  thread_count++;
  printf("thread count = %d\n", thread_count);
  tid_tls_pairs[thread_count].tid = tid;

  tid_tls_pairs[thread_count].tls = calloc(1, sizeof(TLS));
  tid_tls_pairs[thread_count].tls->tid = tid;
  tid_tls_pairs[thread_count].tls->size = size;
  tid_tls_pairs[thread_count].tls->page_num = page_count;
  tid_tls_pairs[thread_count].tls->pages = calloc(page_count, sizeof(struct page *));
  for (int j = 0; j < page_count; j++) {
    tid_tls_pairs[thread_count].tls->pages[j]->address = mmap(NULL, page_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
    
  }






	return 0;
}

int tls_destroy()
{
	return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
	return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
	return 0;
}

int tls_clone(pthread_t tid)
{
	return 0;
}
