#ifndef _TS_QUEUE_H
#define _TS_QUEUE_H

/*
 * Thread safe FIFO queue implementation (using fixed size array)
 */

#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>

#define TS_QUEUE(name, size) struct ts_queue name = {.capacity = size};\
                             ts_queue_init(&name)

struct ts_queue {
    int *items;
    int front;
    int back;
    int capacity;
    pthread_mutex_t lock;
};

/*
 * Only for internal use
 */
void __ts_queue_increment_size(struct ts_queue *q);

/*
 * Only for internal use
 */
void __ts_queue_decrement_size(struct ts_queue *q);

/*
 * Initialize ts_queue. Requires `capacity` field to be filled
 */
void ts_queue_init(struct ts_queue *q);

/*
 * Cleanup after done using ts_queue
 */
void ts_queue_destroy(struct ts_queue *q);

/*
 * Push `item` to the front
 */
void ts_queue_push(struct ts_queue *q, int item);

/*
 * Pop an item from the back
 */
void ts_queue_pop(struct ts_queue *q);

/*
 * Remove `index`-th item
 */
void ts_queue_remove(struct ts_queue *q, int index);

#endif
