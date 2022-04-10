#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "tls.h"

#define THREAD_CNT 2

#define HAVE_PTHREAD_JOIN 0

#define COUNTER_FACTOR 1000000

bool create = 1, create2 = 1; //only allow each create call once

pthread_t tid, tid2; //store tls created thread id's

int counter = 0, loop = 0; //counter keeps track of tid to thread number, loop keeps main while loop running

pthread_mutex_t m1; 

pthread_barrier_t b1;

void *count(void *arg) {

  char *write = "I Test abcdefg",*readA1 = malloc(sizeof(char)*strlen(write)), *readB1 = malloc(sizeof(char)*strlen(write));
  char *write2 = "My angel is the centerfold", *readA2 = malloc(sizeof(char)*strlen(write2)),*readB2 = malloc(sizeof(char)*strlen(write2));
  char *write3 = "Everything is Awesome!", *readA3 = malloc(sizeof(char)*strlen(write3)), *readB3 = malloc(sizeof(char)*strlen(write3));
  
  pthread_mutex_lock(&m1); //enforces thread order, TID 0 runs then TID 1
  
  printf("thread %d = %ld\n", counter, pthread_self()); //print each thread with a small index to keep track

  counter++;
    
    if(create){ //first tls create

      create = 0;

      tid = pthread_self(); //store first tid
      
      printf("Thread %ld creating tls\n", pthread_self());

      tls_create(8192); //create a 2 page tls

      printf("normal write\n");

      tls_write(2000, strlen(write), write); //write in middle of page

      printf("writing on boundary\n");

      tls_write(4093, strlen(write3), write3); //write on boundary
    }
    else if(pthread_self() != tid && create2){ //tls clone, must not be first thread

      printf("Thread %ld copying thread %ld?\n", pthread_self(), tid);
      
      create2 = 0;
      
      tid2 = pthread_self(); //store second tid
      
      tls_clone(tid);
      
      tls_write(100, strlen(write2), write2); //write in the middle of the page
    }
  pthread_mutex_unlock(&m1);

  printf("thread %ld barrier waiting\n",pthread_self());
  
  pthread_barrier_wait(&b1); //syncs both threads and makes sure both are done creating, cloning and writing

  pthread_mutex_lock(&m1); //enforces thread order again, usually TID 1 then TID 0, but it depends on who exits the barrier first
  
  if(pthread_self() == tid){ //if TID 0, the tls_create call

    printf("reading from thread 0?\n");
    
    tls_read(2000, strlen(write), readA1); //should be "I Test abcdefg"

    tls_read(100, strlen(write2), readA2); //should be blank, only written in clone so should only appear in clone due to COW

    tls_read(4093, strlen(write3), readA3); //should be "Everything is Awesome!"
    
    printf("readA1 = %s, readA2 = %s, readA3 = %s\n", readA1, readA2, readA3);
    
    printf("Thread %ld destroying tls?\n", pthread_self());

    tls_destroy();

    loop++; //mark one thread as done
  }

  else if(pthread_self() == tid2){

    printf("reading from thread 1?\n");
    
    tls_read(2000, strlen(write), readB1); //should be "I Test abcdefg"

    tls_read(100, strlen(write2), readB2); //should be "My angel is the centerfold"

    tls_read(4093, strlen(write3), readB3); //should be "everything is awesome", clone can read from boundary
    
    printf("readB1 = %s, readB2 = %s, readB3 = %s\n", readB1, readB2, readB3);

    printf("Thread %ld destroying tls?\n", pthread_self());

    tls_destroy();
   
    loop++; //mark one thread as done
  }

  pthread_mutex_unlock(&m1);
  
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t threads[THREAD_CNT];
  int i;
  pthread_mutex_init(&m1, NULL);
  pthread_barrier_init(&b1, NULL, THREAD_CNT);
  for(i = 0; i < THREAD_CNT; i++) {
    pthread_create(&threads[i], NULL, count,
       (void *)(intptr_t)((i + 1) * COUNTER_FACTOR));
  }

  while(loop < 2){}; //wait until both threads are finished

  pthread_mutex_destroy(&m1);
  pthread_barrier_destroy(&b1);

  printf("end of main\n"); //says when main is about to exit, when program exits overall
  
  return 0;
}