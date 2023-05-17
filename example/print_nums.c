#include "../src/thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_val(void *val) {
  const int ms100 = 1000 * 1;
  usleep(ms100);
  printf("%d\n", *(int *)val);
  free(val);
}

int main() {
  threadpool_t *pool = threadpool_create(10, 50);
  for (int i = 0; i < 40; i++) {
    int *val = malloc(sizeof(int));
    *val = i;
    threadpool_run(pool, print_val, val);
  }
  threadpool_clean(pool, 1);
}