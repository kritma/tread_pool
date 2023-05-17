#include "../src/thread_pool.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define N_MAX 10000

long long time_ms(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

void calc_sum(void *val) {
  float sum = 0;
  for (int i = 0; i < N_MAX; i++) {
    sum += 1.0F / powf(*(float *)val, i);
  }
  *(float *)val = sum;
}

int main() {
  const int sums_length = 1000;
  float *sums = malloc(sizeof(int) * sums_length);

  long long start = time_ms();

  threadpool_t *pool = threadpool_create(10, sums_length);
  for (int i = 0; i < sums_length; i++) {
    sums[i] = i + 1;
    threadpool_run(pool, calc_sum, sums + i);
  }
  threadpool_clean(pool, 1);

  long long end = time_ms();
  printf("time: %lldms\n", end - start);

  start = time_ms();

  for (int i = 0; i < sums_length; i++) {
    sums[i] = i + 1;
    float sum = 0;
    for (int j = 0; j < N_MAX; j++) {
      sum += 1.0F / powf(i, j);
    }
    sums[i] = sum;
  }

  end = time_ms();
  printf("time: %lldms\n", end - start);
  // Intel(R) Core(TM) i7-7700HQ (8) @ 3.8 GHz
  //  time: 23ms
  //  time: 77ms
}