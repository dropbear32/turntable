/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_QUEUE_H
#define TURNTABLE_QUEUE_H

#include <stdlib.h>

struct turntable_queue {
    void **data;
    size_t max;
    size_t count;
};

void *queue_peek(struct turntable_queue *queue);
void *queue_pop(struct turntable_queue *queue);
void queue_push(struct turntable_queue *queue, void *new_item);
void queue_cleanse(struct turntable_queue *queue, int (*filter)(void *item, void *param), void *param);
void queue_destroy(struct turntable_queue *queue, void (*item_destroy)(void *item));

#endif /* TURNTABLE_QUEUE_H */
