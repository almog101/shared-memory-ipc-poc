#include "smq.h"
#include "smq_internal.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void
smq_create(SharedMemoryQueue** const o_smq, const char* const name)
{
  assert(o_smq != NULL);

  SharedMemoryQueue* const smq = malloc(sizeof(SharedMemoryQueue));
  if (smq == NULL) {
    perror("malloc");
    exit(1);
  }

  snprintf(smq->m_shared_memory_name,
           sizeof(smq->m_shared_memory_name),
           "/SMQ_%s_shm",
           name);
  snprintf(
    smq->m_semaphore_name, sizeof(smq->m_semaphore_name), "/SMQ_%s_sem", name);

#if UNLINK_OLD_QUEUE
  sem_unlink(m_shared_memory_name);
  shm_unlink(m_semaphore_name);
#endif

  smq->m_shared_memory_fd =
    shm_open(smq->m_shared_memory_name, O_CREAT | O_RDWR, 0666);
  if (smq->m_shared_memory_fd == -1) {
    perror("shm_open");
    exit(1);
  }

  if (ftruncate(smq->m_shared_memory_fd, sizeof(SharedData)) == -1) {
    perror("ftruncate");
    exit(1);
  }

  {
    void* ptr = mmap(0,
                     sizeof(SharedData),
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     smq->m_shared_memory_fd,
                     0);
    if (ptr == MAP_FAILED) {
      perror("mmap");
      exit(1);
    }

    smq->m_data = (SharedData*)(ptr);
  }

  smq->m_semaphore = sem_open(smq->m_semaphore_name, O_CREAT, 0666, 1);
  if (smq->m_semaphore == SEM_FAILED) {
    perror("sem_open");
    exit(1);
  }

  *o_smq = smq;
}

void
smq_destory(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  munmap(smq->m_data, sizeof(SharedData));
  smq->m_data = NULL;
  close(smq->m_shared_memory_fd);
  smq->m_shared_memory_fd = -1;
  sem_close(smq->m_semaphore);
  smq->m_semaphore = NULL;
}

void
smq_reset(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  smq_lock(smq);
  memset(smq->m_data, 0, sizeof(SharedData));
  smq_unlock(smq);
}

bool
smq_empty(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  smq_lock(smq);
  bool is_empty = smq_size_unsafe(smq) <= 0;
  smq_unlock(smq);
  return is_empty;
}

bool
smq_full(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  smq_lock(smq);
  bool is_full = smq_size_unsafe(smq) >= MaxSize;
  smq_unlock(smq);
  return is_full;
}

size_t
smq_size(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  smq_lock(smq);
  int size = smq_size_unsafe(smq);
  smq_unlock(smq);
  return size;
}

size_t
smq_enqueue(SharedMemoryQueue* const smq,
            const uint8_t* const data,
            size_t size)
{
  assert(smq != NULL);
  assert(size <= MaxBufferSize);
  smq_lock(smq);

  if (smq_full_unsafe(smq)) {
    smq_unlock(smq);
    fprintf(stderr, "Queue is full\n");
    abort();
  }

  Buffer* const buffer = &smq->m_data->buffers[smq->m_data->tail];
  memcpy(buffer->data, data, size);
  buffer->size = size;

  smq->m_data->tail = (smq->m_data->tail + 1) % MaxSize;
  ++smq->m_data->count;

  smq_unlock(smq);

  return size;
}

size_t
smq_dequeue(SharedMemoryQueue* const smq, uint8_t* const data, size_t size)
{
  assert(smq != NULL);
  smq_lock(smq);

  if (smq_empty_unsafe(smq)) {
    smq_unlock(smq);
    fprintf(stderr, "Queue is empty\n");
    abort();
  }

  const Buffer* const buffer = &smq->m_data->buffers[smq->m_data->head];
  const size_t amount_to_copy = size > buffer->size ? buffer->size : size;
  memcpy(data, buffer->data, amount_to_copy);

  smq->m_data->head = (smq->m_data->head + 1) % MaxSize;
  --smq->m_data->count;

  smq_unlock(smq);

  return amount_to_copy;
}

size_t
smq_dequeue_block(SharedMemoryQueue* const smq,
                  uint8_t* const data,
                  size_t size)
{
  assert(smq != NULL);
  for (;;) {
    smq_lock(smq);

    if (smq_empty_unsafe(smq)) {
      smq_unlock(smq);
      usleep(1);
    } else {

      Buffer* const buffer = &smq->m_data->buffers[smq->m_data->head];
      const size_t amount_to_copy = size > buffer->size ? buffer->size : size;
      memcpy(data, buffer->data, amount_to_copy);

      smq->m_data->head = (smq->m_data->head + 1) % MaxSize;
      --smq->m_data->count;

      smq_unlock(smq);

      return amount_to_copy;
    }
  }
}

static void
smq_lock(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  sem_wait(smq->m_semaphore);
}

static void
smq_unlock(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  sem_post(smq->m_semaphore);
}

static bool
smq_empty_unsafe(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  return smq_size_unsafe(smq) <= 0;
}

static bool
smq_full_unsafe(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  return smq_size_unsafe(smq) >= MaxSize;
}

static size_t
smq_size_unsafe(SharedMemoryQueue* const smq)
{
  assert(smq != NULL);
  return smq->m_data->count;
}
