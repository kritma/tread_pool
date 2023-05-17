#include <pthread.h>

typedef struct threadpool_t threadpool_t;

/**
 * @brief Creates a threadpool_t object.
 * @param thread_count threads count in pool
 * @param queue_length queue of callbacks length
 * @return threadpool
 */
threadpool_t *threadpool_create(int thread_count, int queue_length);

/**
 * @param args argument which will be passed in callback
 */
int threadpool_run(threadpool_t *pool, void (*callback)(void *), void *args);

/**
 * @brief clean resources
 *
 * @param pool
 *
 * @param wait if not zero wait till queue processed
 */
void threadpool_clean(threadpool_t *pool, int wait);
