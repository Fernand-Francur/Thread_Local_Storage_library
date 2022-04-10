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

#define MAX_THREAD_COUNT 128
#define PAGE_SIZE 4096
#define INIT_REFERENCE 1

static struct tid_tls_pair tid_tls_pairs[MAX_THREAD_COUNT];
static unsigned int thread_count = 0;
static pthread_mutex_t lock;

void *mem_cpy(void *dest, const void *src, size_t n) {
    size_t i;
    char * new_start = (char *) src;
    char * new_dest = (char *) dest;
    for (i = n; i-- > 0;) {
        new_dest[i] = new_start[i];
    }
    return new_dest;
}

void tls_handle_page_fault(int sig, siginfo_t *si, void *context) {
   pthread_mutex_lock(&lock);
  // pthread_exit(NULL);
  int page_size = getpagesize();
  //printf("Page size = %d\n", page_size);


  unsigned long int p_fault = ((unsigned long int) si->si_addr) & ~(page_size -1);
  //int tls_reference;
  pthread_t tid = pthread_self();
  TLS * curr;
  //bool tls_exists = false;
  bool addr_in_thread = false;
  void * retval = (void *)1;
  for (int i = 0; i < MAX_THREAD_COUNT; i++) {

    // printf("tid = %ld\n", tid_tls_pairs[i].tid);
    if ( tid_tls_pairs[i].tid == tid) {
      curr = tid_tls_pairs[i].tls;
      //tls_reference = i;
      //tls_exists = true;
      break;
    }
  }
  //if (tls_exists) {
    for (int j = 0; j < curr->page_num; j++) {
      if (tid_tls_pairs[thread_count].tls->pages[j]->address == p_fault) {
        addr_in_thread = true;
        break;
      }
    }
  //} 

  if(addr_in_thread == false) {
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
     pthread_mutex_unlock(&lock);
    raise(sig);
  }
   pthread_mutex_unlock(&lock);
  pthread_exit(retval);

}

void tls_init() {
  struct sigaction sa;
  //int page_size = getpagesize();
  //printf("Page size = %d\n", page_size);

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = tls_handle_page_fault;
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL); 
}



// This function creates a LSA for a thread

int tls_create(unsigned int size) {
  pthread_mutex_lock(&lock);
  //printf("\nCreate has started\n");
  pthread_t tid = pthread_self();
  int page_size = getpagesize();
  unsigned int page_count = (size + (page_size-1))/page_size;
  //printf("page count = %d\n", page_count);
  //printf("page size = %d\n", page_size);
  if (thread_count == 0) {
    tls_init();
  }
  if (size < 0) {
    perror("ERROR: Size is less than zero");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {
    if (tid== tid_tls_pairs[i].tid) {
      perror("ERROR: LSA for this thread is already created");
      pthread_mutex_unlock(&lock);
      return -1;
    }
  }
  //printf("thread count = %d\n", thread_count);

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
    //printf("address = %lx ref_count = %d\n", tid_tls_pairs[thread_count].tls->pages[j]->address, tid_tls_pairs[thread_count].tls->pages[j]->ref_count);

  }
  //printf("LSA for thread %ld created\n\n", tid_tls_pairs[thread_count].tid);
  thread_count++;
  pthread_mutex_unlock(&lock);
  //printf("Thread count = %d\n\n", thread_count);
	return 0;
}


// This function destroys a thread LSA

int tls_destroy()
{
  pthread_mutex_lock(&lock);
  //printf("Thread count = %d\n\n", thread_count);
  int page_size = getpagesize();
  pthread_t tid = pthread_self();
  int tls_reference;
  bool tid_exists = false;
  //printf("\nDestroy has started for thread %ld\n", tid);
  if (thread_count == 0) {
    perror("ERROR: TLS system not initialized");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {

    //printf("tid = %ld\n", tid_tls_pairs[i].tid);
    if (tid == tid_tls_pairs[i].tid) {
      tls_reference = i;
      tid_exists = true;
      break;
    }
  }
    if (tid_exists != true) {
    perror("ERROR: LSA of target thread does not exist");
    pthread_mutex_unlock(&lock);
    return -1;
  }

  int page_destroy = tid_tls_pairs[tls_reference].tls->page_num;
  for (int j = 0; j < page_destroy; j++) {
    if (tid_tls_pairs[tls_reference].tls->pages[j]->ref_count == 1) {
      munmap((void *) tid_tls_pairs[tls_reference].tls->pages[j]->address, page_size);
      free(tid_tls_pairs[tls_reference].tls->pages[j]);
    } else {
      tid_tls_pairs[tls_reference].tls->pages[j]->ref_count--;
    }
  }

  free(tid_tls_pairs[tls_reference].tls->pages);
  free(tid_tls_pairs[tls_reference].tls);
  tid_tls_pairs[tls_reference].tid = 0;

  for (int i = tls_reference; i < (MAX_THREAD_COUNT - 1); i++) {
    tid_tls_pairs[i] = tid_tls_pairs[i+1];

  }

  tid_tls_pairs[MAX_THREAD_COUNT].tls = NULL;
 
  thread_count = thread_count - 1;
  pthread_mutex_unlock(&lock);
  //printf("Thread %ld destroyed\n", pthread_self());
  //printf("Thread count = %d\n\n", thread_count);

	return 0;
}








int tls_read(unsigned int offset, unsigned int length, char *buffer)
{
  pthread_mutex_lock(&lock);
  pthread_t tid = pthread_self();
  bool tid_exists = false;
  int tls_reference;
  int page_size = getpagesize();
  int curr_page = 0;
  char *curr_dest;
  unsigned long int length_in_page = 0;
  unsigned long int dest_calc = 0;

    //printf("\nRead has started of thread %ld\n", tid);

  if (length <= 0) {
    perror("ERROR: Length must be a positive integer");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  if (thread_count == 0) {
    perror("ERROR: TLS system not initialized");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {
    //printf("tid = %ld\n", tid_tls_pairs[i].tid);
    if (tid == tid_tls_pairs[i].tid) {
      
      tls_reference = i;
      tid_exists = true;
    }
  }
  if (tid_exists != true) {
    perror("ERROR: LSA of target thread does not exist");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  if (offset + length > tid_tls_pairs[tls_reference].tls->size) {
    perror("ERROR: Memory referenced out of size of LSA");
    pthread_mutex_unlock(&lock);
    return -1;
  }

  while ((offset < page_size) != true) {
    offset = offset - page_size;
    curr_page++;
  }

  void * src = (void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address + offset);


  curr_dest = buffer;
  length_in_page = page_size - offset;

  if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_READ) != 0) {
    perror("ERROR: Could not change memory protections of page");
    pthread_mutex_unlock(&lock);
    return -1;
  }

  while(length_in_page < length) {
    memcpy(curr_dest, src, length_in_page);

    if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
      perror("ERROR: Could not change memory protections of page");
      pthread_mutex_unlock(&lock);
      return -1;
    }

    dest_calc = ((unsigned long int) curr_dest ) + length_in_page;
    curr_dest = (char *) dest_calc;
    length = length - length_in_page;
    curr_page++;
    length_in_page = page_size;
    
    src = (void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address);
    if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_READ) != 0) {
      perror("ERROR: Could not change memory protections of page");
      pthread_mutex_unlock(&lock);
      return -1;
    }
  }
  memcpy(curr_dest, src, length);
  if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
    perror("ERROR: Could not change memory protections of page");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  pthread_mutex_unlock(&lock);
   //printf("Thread count = %d\n\n", thread_count);
  //printf("Read complete\n\n");
	return 0;
}







int tls_write(unsigned int offset, unsigned int length, const char *buffer)
{
  pthread_mutex_lock(&lock);
  pthread_t tid = pthread_self();
  bool tid_exists = false;
  int tls_reference;
  int page_size = getpagesize();
  int curr_page = 0;
  char * src;
  char * tmp_buff;
  unsigned long int length_in_page = 0;
  unsigned long int src_calc = 0;

  //printf("\nWrite has started for thread %ld\n", tid);
  // unsigned long int curr_dest;
  if (length <= 0) {
    perror("ERROR: Length must be a positive integer");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  if (thread_count == 0) {
    perror("ERROR: TLS system not initialized");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {
    //printf("tid = %ld\n", tid_tls_pairs[i].tid);
    if (tid == tid_tls_pairs[i].tid) {
      tls_reference = i;
      tid_exists = true;
      break;
    }
  }
  if (tid_exists != true) {
    perror("ERROR: LSA of target thread does not exist");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  if (offset + length > tid_tls_pairs[tls_reference].tls->size) {
    perror("ERROR: Memory referenced out of size of LSA");
    pthread_mutex_unlock(&lock);
    return -1;
  }

 while ((offset < page_size) != true) {
    offset = offset - page_size;
    curr_page++;
  }
  
  tmp_buff = calloc(length, 1);
  memcpy(tmp_buff, buffer, length);


  // hell breaks loose
  if (tid_tls_pairs[tls_reference].tls->pages[curr_page]->ref_count != 1) {
    struct page * new_page = calloc(1, sizeof(struct page));
    new_page->address = (unsigned long) mmap(NULL, page_size, PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    
    // new page is alloced

    if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_READ) != 0) {
      perror("ERROR: Could not change memory protections of page");
      pthread_mutex_unlock(&lock);
      return -1;
    }

    // old page is open to read

    memcpy((void *) new_page->address, (void *) tid_tls_pairs[tls_reference].tls->pages[curr_page]->address, page_size);
    
    if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
      perror("ERROR: Could not change memory protections of page");
      pthread_mutex_unlock(&lock);
      return -1;
    }

    // old page copied to new and old is locked

    new_page->ref_count = 1;
    tid_tls_pairs[tls_reference].tls->pages[curr_page]->ref_count--;
    tid_tls_pairs[tls_reference].tls->pages[curr_page] = new_page;
    //printf("new address = %lx ref_count = %d\n", tid_tls_pairs[tls_reference].tls->pages[curr_page]->address, tid_tls_pairs[tls_reference].tls->pages[curr_page]->ref_count);
    
    if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
      perror("ERROR: Could not change memory protections of page");
      pthread_mutex_unlock(&lock);
      return -1;
    }
    // new ref set to 1, old decremented, current page set to new
  }

  void * curr_dest = (void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address + offset);

  
  src = tmp_buff;
  length_in_page = page_size - offset;
  if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_WRITE) != 0) {
    perror("ERROR: Could not change memory protections of page");
    pthread_mutex_unlock(&lock);
    return -1;
  }

  while(length_in_page < length) {
    memcpy(curr_dest, src, length_in_page);
    if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
      perror("ERROR: Could not change memory protections of page");
      pthread_mutex_unlock(&lock);
      return -1;
    }

    src_calc = ((unsigned long int) src ) + length_in_page;
    src = (char *) src_calc;
    length = length - length_in_page;
    curr_page++;
    length_in_page = page_size;

    if (tid_tls_pairs[tls_reference].tls->pages[curr_page]->ref_count != 1) {
      struct page * new_page = calloc(1, sizeof(struct page));
      new_page->address = (unsigned long) mmap(NULL, page_size, PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    
      if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_READ) != 0) {
        perror("ERROR: Could not change memory protections of page");
        pthread_mutex_unlock(&lock);
        return -1;
      }

      memcpy((void *) new_page->address, (void *) tid_tls_pairs[tls_reference].tls->pages[curr_page]->address, page_size);
    
      if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
        perror("ERROR: Could not change memory protections of page");
        pthread_mutex_unlock(&lock);
        return -1;
      }

      new_page->ref_count = 1;
      tid_tls_pairs[tls_reference].tls->pages[curr_page]->ref_count--;
      tid_tls_pairs[tls_reference].tls->pages[curr_page] = new_page;
      if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
        perror("ERROR: Could not change memory protections of page");
        pthread_mutex_unlock(&lock);
        return -1;
      }
    }

    curr_dest = (void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address);
    if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_WRITE) != 0) {
      perror("ERROR: Could not change memory protections of page");
      pthread_mutex_unlock(&lock);
      return -1;
    }
  }
  memcpy(curr_dest, src, length);
  if (mprotect((void *) (tid_tls_pairs[tls_reference].tls->pages[curr_page]->address), page_size, PROT_NONE) != 0) {
    perror("ERROR: Could not change memory protections of page");
    pthread_mutex_unlock(&lock);
    return -1;
  }
   //printf("Thread count = %d\n\n", thread_count);
  //printf("Written to thread %ld\n\n", tid_tls_pairs[tls_reference].tid);
  pthread_mutex_unlock(&lock);
	return 0;
}






int tls_clone(pthread_t tid)
{
  pthread_mutex_lock(&lock);
  pthread_t my_tid = pthread_self();
  bool tid_exists = false;
  int tls_reference;
  
  //printf("\nClone has started for thread %ld\n", tid);

  if (thread_count == 0) {
    perror("ERROR: TLS system not initialized");
    pthread_mutex_unlock(&lock);
    return -1;
  }
  for (int i = 0; i < thread_count; i++) {
    //printf("tid = %ld\n", tid_tls_pairs[i].tid);
    if (my_tid == tid_tls_pairs[i].tid) {
      perror("ERROR: LSA for this thread is already created");
      pthread_mutex_unlock(&lock);
      return -1;
    }
    if (tid == tid_tls_pairs[i].tid) {
      tls_reference = i;
      tid_exists = true;
    }
  }
  if (tid_exists != true) {
    perror("ERROR: LSA of target thread does not exist");
    pthread_mutex_unlock(&lock);
    return -1;
  }


  //printf("thread count = %d\n", thread_count);
  tid_tls_pairs[thread_count].tid = my_tid;
  tid_tls_pairs[thread_count].tls = calloc(1, sizeof(TLS));
  tid_tls_pairs[thread_count].tls->tid = my_tid;
  tid_tls_pairs[thread_count].tls->size = tid_tls_pairs[tls_reference].tls->size;
  tid_tls_pairs[thread_count].tls->page_num = tid_tls_pairs[tls_reference].tls->page_num;
  tid_tls_pairs[thread_count].tls->pages = calloc(tid_tls_pairs[thread_count].tls->page_num, sizeof(struct page *));

  for (int j = 0; j < tid_tls_pairs[thread_count].tls->page_num; j++) {
    tid_tls_pairs[thread_count].tls->pages[j] = tid_tls_pairs[tls_reference].tls->pages[j];
    tid_tls_pairs[thread_count].tls->pages[j]->ref_count++;
    // printf("address1 = %lx address2 = %lx ref_count = %d\n", tid_tls_pairs[thread_count].tls->pages[j]->address,tid_tls_pairs[tls_reference].tls->pages[j]->address, tid_tls_pairs[thread_count].tls->pages[j]->ref_count);
  }
  //printf("LSA for thread %ld cloned\n\n", tid_tls_pairs[thread_count].tid);
  thread_count++;
  pthread_mutex_unlock(&lock);
    //printf("Thread count = %d\n\n", thread_count);
	return 0;
}
