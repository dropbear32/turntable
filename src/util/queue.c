/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

// Linear queue implementation

#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

void *queue_peek(struct turntable_queue *queue) {
    return queue->data[0];
}

void *queue_pop(struct turntable_queue *queue) {
    void *result = queue_peek(queue);
    for (size_t i = 1; i < queue->count; i++) {
        queue->data[i - 1] = queue->data[i];
    }
    queue->count--;
    return result;
}

void queue_push(struct turntable_queue *queue, void *new_item) {
    if (queue->count + 1 > queue->max) {
        void *new_ptr = realloc(queue->data, sizeof(void *) * (queue->max * 2));
        if (new_ptr == NULL) {
            printf("%zu\n", sizeof(void *) * (queue->max * 2));
            fprintf(stderr, "could not realloc queue\n");
            return;
        }
        queue->data = new_ptr;
        queue->max *= 2;
    }

    queue->data[queue->count] = new_item;
    queue->count++;
}

void queue_cleanse(struct turntable_queue *queue, int (*filter)(void *item, void *param), void *param) {
    for (size_t i = 0; i < queue->count; i++) {
        if (filter(queue->data[i], param)) {
            for (size_t j = i + 1; j < queue->count; j++) {
                queue->data[j - 1] = queue->data[j];
            }
            i--;
            queue->count--;
        }
    }
}

void queue_destroy(struct turntable_queue *queue, void (*item_destroy)(void *item)) {
    for (size_t i = 0; i < queue->count; i++) {
        item_destroy(queue->data[i]);
    }
    queue->count = 0;
    queue->max = 0;
}
