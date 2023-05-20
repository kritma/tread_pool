#include "thread_pool.h"
#include <stdlib.h>
#include <threads.h>

typedef struct {
  void (*callback)(void *);
  void *args;
} callback_task_t;

struct threadpool_t {
  callback_task_t *callbacks;
  int queue_head;
  int queue_tail;
  int queue_length;
  int queue_count;

  thrd_t *threads;
  mtx_t mutex;
  cnd_t element_added_condition;

  int is_shutdown;
  int thread_count;
};

int threadpool_run(threadpool_t *pool, void (*callback)(void *), void *args) {
  mtx_lock(&pool->mutex);

  if (pool->queue_count == pool->queue_length) {
    mtx_unlock(&pool->mutex);
    return 0;
  }

  callback_task_t task;
  task.callback = callback;
  task.args = args;
  pool->callbacks[pool->queue_tail] = task;

  pool->queue_tail =
      pool->queue_tail == pool->queue_length - 1 ? 0 : pool->queue_tail + 1;
  pool->queue_count++;

  cnd_signal(&pool->element_added_condition);
  mtx_unlock(&pool->mutex);
  return 1;
}

static int worker(void *arg) {
  threadpool_t *pool = (threadpool_t *)arg;
  while (1) {
    mtx_lock(&pool->mutex);

    while (pool->queue_count == 0 && pool->is_shutdown == 0) {
      cnd_wait(&pool->element_added_condition, &pool->mutex);
    }

    if (pool->is_shutdown == 1 ||
        (pool->is_shutdown == 2 && pool->queue_count == 0)) {
      mtx_unlock(&pool->mutex);
      return 0;
    }

    const callback_task_t task = pool->callbacks[pool->queue_head];
    pool->queue_head =
        pool->queue_head == pool->queue_length - 1 ? 0 : pool->queue_head + 1;
    pool->queue_count--;

    int count = pool->queue_count;
    mtx_unlock(&pool->mutex);
    task.callback(task.args);
  }
  return 0;
}

void threadpool_destroy(threadpool_t *pool, int wait) {
  pool->is_shutdown = wait ? 2 : 1;
  cnd_broadcast(&pool->element_added_condition);
  for (int i = 0; i < pool->thread_count; i++) {
    thrd_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool->callbacks);

  mtx_destroy(&pool->mutex);
  cnd_destroy(&pool->element_added_condition);
}

threadpool_t *threadpool_create(int thread_count, int queue_length) {
  threadpool_t *pool = malloc(sizeof(threadpool_t));
  pool->queue_head = 0;
  pool->queue_tail = 0;
  pool->is_shutdown = 0;
  pool->queue_count = 0;
  pool->queue_length = queue_length;
  pool->thread_count = thread_count;

  pool->callbacks = malloc(queue_length * sizeof(callback_task_t));
  pool->threads = malloc(thread_count * sizeof(thrd_t));

  mtx_init(&pool->mutex, mtx_plain);
  cnd_init(&pool->element_added_condition);

  for (int i = 0; i < thread_count; i++) {
    thrd_create(&pool->threads[i], worker, pool);
  }
  return pool;
}