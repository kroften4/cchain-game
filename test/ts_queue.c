#include "src/ts_queue.h"
#include <assert.h>
#include <stdlib.h>

int main() {
    struct ts_queue *q = ts_queue_new();
    int a = 1, b = 2, c = 3;
    ts_queue_enqueue(q, &a);
    ts_queue_enqueue(q, &b);
    ts_queue_enqueue(q, &c);

    assert(*(int *)(q->head->data) == 1 && "Incorrect head data after enqueue");
    assert(*(int *)(q->head->next->data) == 2 && "Incorrect contents after enqueue");
    assert(*(int *)(q->tail->data) == 3 && "Incorrect tail data after enqueue");

    ts_queue_dequeue(q);

    assert(*(int *)(q->head->data) == 2 && "Incorrect head data after dequeue");
    assert(*(int *)(q->tail->data) == 3 && "Incorrect tail data after dequeue");

    ts_queue_dequeue(q);

    assert(*(int *)(q->head->data) == 3 && 
           "Incorrect head data after dequeue (1 item)");
    assert(*(int *)(q->tail->data) == 3 && 
           "Incorrect tail data after dequeue (1 item)");

    ts_queue_dequeue(q);

    assert(__ts_queue_is_empty(q) && "Not empty after dequeue");

    ts_queue_destroy(q);
    free(q);

    return 0;
}

