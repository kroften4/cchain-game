#include "src/ts_queue.h"
#include <assert.h>

int main() {
    TS_QUEUE(q, 10);
    assert(q.back == -1 && "Must be empty on creation");

    ts_queue_push(&q, 3);
    ts_queue_push(&q, 2);
    ts_queue_push(&q, 1);

    assert(q.back == 2 && "Incorrect size (q.back) after pushing");
    assert(q.items[0] == 1 && q.items[1] == 2 && q.items[2] == 3 &&
     "Incorrect contents after pushing");

    ts_queue_remove(&q, 1);

    assert(q.back == 1 && q.items[0] == 1 && q.items[1] == 3 &&
     "Incorrect size or contents after removing");

    ts_queue_pop(&q);
    ts_queue_pop(&q);

    assert(q.back == -1 && "Incorrect size after popping");

    ts_queue_destroy(&q);

    return 0;
}

