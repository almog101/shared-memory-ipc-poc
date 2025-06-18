#ifndef SMQ_INTERNAL_H
#define SMQ_INTERNAL_H

#include <linux/limits.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>

#define MaxSize 100
#define MaxBufferSize 4096

typedef struct Buffer
{
  uint8_t data[MaxBufferSize];
  size_t size;
} Buffer;

typedef struct SharedData
{
  Buffer buffers[MaxSize];
  size_t head;
  size_t tail;
  size_t count;
} SharedData;

typedef struct SharedMemoryQueue
{
  int m_shared_memory_fd;
  SharedData* m_data;
  sem_t* m_semaphore;
  char m_shared_memory_name[PATH_MAX];
  char m_semaphore_name[PATH_MAX];
} SharedMemoryQueue;


static void
smq_lock(SharedMemoryQueue* const smq);
static void
smq_unlock(SharedMemoryQueue* const smq);
static bool
smq_empty_unsafe(SharedMemoryQueue* const smq);
static bool
smq_full_unsafe(SharedMemoryQueue* const smq);
static size_t
smq_size_unsafe(SharedMemoryQueue* const smq);

#endif
