#include <pthread.h>
#include <stdlib.h>
#include "ts_queue.h"

struct ts_queue_node *ts_queue_node_new() {
    struct ts_queue_node *node = malloc(sizeof(struct ts_queue_node));
    node->data = NULL;
    node->prev = NULL;
    node->next = NULL;
    return node;
};

void ts_queue_destroy(struct ts_queue *q) {
    // TODO: free all nodes
    pthread_mutex_destroy(&q->lock);
}

bool __ts_queue_is_empty(struct ts_queue *q) {
    return q->head == NULL;
}

void __ts_queue_add(struct ts_queue *q, struct ts_queue_node *prev,
                    struct ts_queue_node *next, struct ts_queue_node *node) {
    pthread_mutex_lock(&q->lock);
    if (__ts_queue_is_empty(q)) {
        q->head = node;
        q->tail = node;
    } else if (prev == NULL) {
        q->head = node;
        next->prev = node;
        node->next = next;
    } else if (next == NULL) {
        q->tail = node;
        prev->next = node;
        node->prev = prev;
    } else {
        prev->next = node;
        next->prev = node;
        node->prev = prev;
        node->next = next;
    }
    pthread_mutex_unlock(&q->lock);
}

void __ts_queue_remove(struct ts_queue *q, struct ts_queue_node *prev,
                    struct ts_queue_node *next) {
    pthread_mutex_lock(&q->lock);
    if (__ts_queue_is_empty(q))
        return;

    if (prev == NULL)
        q->head = next;
    if (next == NULL)
        q->tail = prev;

    struct ts_queue_node *node;
    if (prev == NULL && next == NULL) {
        node = q->tail;
    }

    if (prev != NULL) {
        node = prev->next;
        prev->next = next;
    }
    if (next != NULL) {
        node = next->prev;
        next->prev = prev;
    }

    free(node);
    pthread_mutex_unlock(&q->lock);
}

void ts_queue_enqueue(struct ts_queue *q, void *item) {
    pthread_mutex_lock(&q->lock);
    struct ts_queue_node *new_node = ts_queue_node_new();
    new_node->data = item;
    __ts_queue_add(q, q->tail, NULL, new_node);
    pthread_mutex_unlock(&q->lock);
}

void ts_queue_dequeue(struct ts_queue *q) {
    pthread_mutex_lock(&q->lock);
    if (__ts_queue_is_empty(q))
        return;
    __ts_queue_remove(q, NULL, q->head->next);
    pthread_mutex_unlock(&q->lock);
}

