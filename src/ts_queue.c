#include <pthread.h>
#include <stdlib.h>
#include "ts_queue.h"

void __ts_queue_increment_size(struct ts_queue *q) {
    if (q->back == -1) {
        q->front = 0;
        q->back = 0;
    } else {
        q->back++;
    }
}

void __ts_queue_decrement_size(struct ts_queue *q) {
    if (q->back == 0) {
        q->front = -1;
        q->back = -1;
    } else {
        q->back--;
    }
}

void ts_queue_init(struct ts_queue *q) {
    q->items = malloc(sizeof(int) * q->capacity);
    q->front = -1;
    q->back = -1;
    pthread_mutex_init(&q->lock, NULL);
}

void ts_queue_destroy(struct ts_queue *q) {
    free(q->items);
    pthread_mutex_destroy(&q->lock);
}

void ts_queue_push(struct ts_queue *q, int item) {
    pthread_mutex_lock(&q->lock);
    for (int i = q->back; i >= 0; i--) {
        q->items[i + 1] = q->items[i];
    }
    q->items[0] = item;
    __ts_queue_increment_size(q);
    pthread_mutex_unlock(&q->lock);
}

void ts_queue_pop(struct ts_queue *q) {
    pthread_mutex_lock(&q->lock);
    __ts_queue_decrement_size(q);
    pthread_mutex_unlock(&q->lock);
}

void ts_queue_remove(struct ts_queue *q, int index) {
    pthread_mutex_lock(&q->lock);
    for (int i = index; i < q->back; i++) {
        q->items[i] = q->items[i + 1];
    }
    __ts_queue_decrement_size(q);
    pthread_mutex_unlock(&q->lock);
}

