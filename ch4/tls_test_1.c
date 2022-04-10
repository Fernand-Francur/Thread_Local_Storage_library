#include "tls.h"
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

int main() {


  int p = tls_create(10);
  printf("p value = %d\n", p);
  int t = tls_destroy();
  printf("t value = %d\n", t);

  return 0;
}