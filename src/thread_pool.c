#include "thread_pool.h"
#include <stdlib.h>

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

  pthread_t *threads;
  pthread_mutex_t mutex;
  pthread_cond_t element_added_condition;

  int is_shutdown;
  int thread_count;
};

int threadpool_run(threadpool_t *pool, void (*callback)(void *), void *args) {
  pthread_mutex_lock(&pool->mutex);

  if (pool->queue_count == pool->queue_length) {
    pthread_mutex_unlock(&pool->mutex);
    return 0;
  }

  callback_task_t task;
  task.callback = callback;
  task.args = args;
  pool->callbacks[pool->queue_tail] = task;

  pool->queue_tail =
      pool->queue_tail == pool->queue_length - 1 ? 0 : pool->queue_tail + 1;
  pool->queue_count++;

  pthread_cond_signal(&pool->element_added_condition);
  pthread_mutex_unlock(&pool->mutex);
  return 1;
}

static void *worker(void *arg) {
  threadpool_t *pool = (threadpool_t *)arg;
  while (1) {
    pthread_mutex_lock(&pool->mutex);

    while (pool->queue_count == 0 && pool->is_shutdown == 0) {
      pthread_cond_wait(&pool->element_added_condition, &pool->mutex);
    }

    if (pool->is_shutdown == 1 ||
        (pool->is_shutdown == 2 && pool->queue_count == 0)) {
      pthread_mutex_unlock(&pool->mutex);
      return NULL;
    }

    const callback_task_t task = pool->callbacks[pool->queue_head];
    pool->queue_head =
        pool->queue_head == pool->queue_length - 1 ? 0 : pool->queue_head + 1;
    pool->queue_count--;

    int count = pool->queue_count;
    pthread_mutex_unlock(&pool->mutex);
    task.callback(task.args);
  }
  return NULL;
}

void threadpool_clean(threadpool_t *pool, int wait) {
  pool->is_shutdown = wait ? 2 : 1;
  pthread_cond_broadcast(&pool->element_added_condition);
  for (int i = 0; i < pool->thread_count; i++) {
    pthread_join(pool->threads[i], NULL);
  }

  free(pool->threads);
  free(pool->callbacks);

  pthread_mutex_destroy(&pool->mutex);
  pthread_cond_destroy(&pool->element_added_condition);
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
  pool->threads = malloc(thread_count * sizeof(pthread_t));

  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->element_added_condition, NULL);

  for (int i = 0; i < thread_count; i++) {
    pthread_create(&pool->threads[i], NULL, worker, pool);
  }
  return pool;
}