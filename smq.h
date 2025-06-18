#ifndef SMQ_H
#define SMQ_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct SharedMemoryQueue;
typedef struct SharedMemoryQueue SharedMemoryQueue;

void
smq_create(SharedMemoryQueue** const o_smq, const char* const name);

void
smq_destory(SharedMemoryQueue* const smq);

void
smq_reset(SharedMemoryQueue* const smq);

bool
smq_empty(SharedMemoryQueue* const smq);

bool
smq_full(SharedMemoryQueue* const smq);

size_t
smq_size(SharedMemoryQueue* const smq);

size_t
smq_enqueue(SharedMemoryQueue* const smq,
            const uint8_t* const data,
            size_t size);

size_t
smq_dequeue(SharedMemoryQueue* const smq, uint8_t* const data, size_t size);

size_t
smq_dequeue_block(SharedMemoryQueue* const smq,
                  uint8_t* const data,
                  size_t size);

#endif
