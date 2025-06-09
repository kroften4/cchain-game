#ifndef _TS_QUEUE_H
#define _TS_QUEUE_H

/*
 * Thread safe FIFO queue implementation (using fixed size array)
 */

#include <pthread.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#define TS_QUEUE(name, size) struct ts_queue name = {.capacity = size};\
                             ts_queue_init(&name)

struct ts_queue_node {
    void *data;
    struct ts_queue_node *prev;
    struct ts_queue_node *next;
};

struct ts_queue_node *ts_queue_node_new();

struct ts_queue {
    struct ts_queue_node *head;
    struct ts_queue_node *tail;
    pthread_mutex_t lock;
};

/*
 * Cleanup after done using ts_queue
 */
void ts_queue_destroy(struct ts_queue *q);

void __ts_queue_add(struct ts_queue *q, struct ts_queue_node *prev,
                    struct ts_queue_node *next, struct ts_queue_node *node);

void __ts_queue_remove(struct ts_queue *q, struct ts_queue_node *prev,
                    struct ts_queue_node *next);

bool __ts_queue_is_empty(struct ts_queue *q);

void ts_queue_enqueue(struct ts_queue *q, void *item);

void ts_queue_dequeue(struct ts_queue *q);

#endif
