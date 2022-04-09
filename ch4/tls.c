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
  unsigned long address;
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
#define INIT_REFERENCE 1

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
  // page_count = 3; //Just testing...
  tid_tls_pairs[thread_count].tid = tid;
  tid_tls_pairs[thread_count].tls = calloc(1, sizeof(TLS));
  tid_tls_pairs[thread_count].tls->tid = tid;
  tid_tls_pairs[thread_count].tls->size = size;
  tid_tls_pairs[thread_count].tls->page_num = page_count;
  tid_tls_pairs[thread_count].tls->pages = calloc(page_count, sizeof(struct page *));
  for (int j = 0; j < page_count; j++) {
    tid_tls_pairs[thread_count].tls->pages[j] = calloc(1, sizeof(struct page));
    tid_tls_pairs[thread_count].tls->pages[j]->address = (unsigned long) mmap(NULL, page_size, PROT_NONE, MAP_ANON | MAP_PRIVATE, 0, 0);
    tid_tls_pairs[thread_count].tls->pages[j]->ref_count = INIT_REFERENCE;
    printf("address = %lx ref_count = %d\n", tid_tls_pairs[thread_count].tls->pages[j]->address, tid_tls_pairs[thread_count].tls->pages[j]->ref_count);

// void * src = (void*) (tid_tls_pairs[thread_count].tls->pages[j]->address + 4096);
// printf("address = %p\n", src);
  }
	return 0;
}

int tls_destroy()
{
	return 0;
}

int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
  pthread_t tid = pthread_self();
  bool tid_exists = false;
  int tls_reference;
  int page_size = getpagesize();
  int curr_page = 0;
  char *curr_dest;
  unsigned long int length_in_page = 0;
  unsigned long int dest_calc = 0;
  // unsigned long int curr_dest;
  if (length <= 0) {
    perror("ERROR: Length must be a positive integer");
    return -1;
  }
  if (thread_count == 0) {
    perror("ERROR: TLS system not initialized");
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {
    if (tid == tid_tls_pairs[i].tid) {
      tls_reference = i;
      tid_exists = true;
    }
  }
  if (tid_exists != true) {
    perror("ERROR: LSA of target thread does not exist");
    return -1;
  }
  if (offset + length <= tid_tls_pairs[tls_reference].tls->size) {
    perror("ERROR: Memory referenced out of size of LSA");
    return -1;
  }

  while ((offset < page_size) != true) {
    offset = offset - page_size;
    curr_page++;
  }
  

  void * src = (void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address + offset);
  curr_dest = buffer;
  length_in_page = page_size - offset;
  while(length_in_page <= length) {
    memcpy(curr_dest, src, length_in_page);

    dest_calc = ((unsigned long int) curr_dest ) + length_in_page;
    curr_dest = (char *) dest_calc;
    length = length - length_in_page;
    curr_page++;
    length_in_page = page_size;
    src = (void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address);
  }
  memcpy(curr_dest, src, length);

	return 0;
}

int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
  // pthread_t my_tid = pthread_self();
	return 0;
}

int tls_clone(pthread_t tid)
{
  pthread_t my_tid = pthread_self();
  bool tid_exists = false;
  int tls_reference;
  
  if (thread_count == 0) {
    perror("ERROR: TLS system not initialized");
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {
    if (my_tid == tid_tls_pairs[i].tid) {
      perror("ERROR: LSA for this thread is already created");
      return -1;
    }
    if (tid == tid_tls_pairs[i].tid) {
      tls_reference = i;
      tid_exists = true;
    }
  }
  if (tid_exists != true) {
    perror("ERROR: LSA of target thread does not exist");
    return -1;
  }

  thread_count++;
  printf("thread count = %d\n", thread_count);
  tid_tls_pairs[thread_count].tid = my_tid;
  tid_tls_pairs[thread_count].tls = calloc(1, sizeof(TLS));
  tid_tls_pairs[thread_count].tls->tid = my_tid;
  tid_tls_pairs[thread_count].tls->size = tid_tls_pairs[tls_reference].tls->size;
  tid_tls_pairs[thread_count].tls->page_num = tid_tls_pairs[tls_reference].tls->page_num;
  tid_tls_pairs[thread_count].tls->pages = tid_tls_pairs[tls_reference].tls->pages;

  for (int j = 0; j < tid_tls_pairs[thread_count].tls->page_num; j++) {
    tid_tls_pairs[thread_count].tls->pages[j]->ref_count++;
    printf("address = %lx ref_count = %d\n", tid_tls_pairs[thread_count].tls->pages[j]->address, tid_tls_pairs[thread_count].tls->pages[j]->ref_count);
  }

	return 0;
}
