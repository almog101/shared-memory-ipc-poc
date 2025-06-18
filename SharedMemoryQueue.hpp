#pragma once

#include <fcntl.h>
#include <semaphore.h>
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/wait.h>

template<typename T, size_t MaxSize>
struct SharedMemoryQueue
{
private:
  struct SharedData
  {
    T buffer[MaxSize];
    size_t head;
    size_t tail;
    size_t count;
  };

public:
  SharedMemoryQueue(std::string name)
  {
    {
      std::ostringstream stringStream;
      stringStream << "/SMQ_";
      stringStream << name;
      stringStream << "_shm";
      m_shared_memory_name = stringStream.str();
    }

	{
      std::ostringstream stringStream;
      stringStream << "/SMQ_";
      stringStream << name;
      stringStream << "_sem";
      m_semaphore_name = stringStream.str();
	}

#if UNLINK_OLD_QUEUE
    sem_unlink(m_shared_memory_name);
    shm_unlink(m_semaphore_name);
#endif

    m_shared_memory_fd = shm_open(m_shared_memory_name.c_str(), O_CREAT | O_RDWR, 0666);
    if (m_shared_memory_fd == -1) {
      perror("shm_open");
      exit(1);
    }

    if (ftruncate(m_shared_memory_fd, sizeof(SharedData)) == -1) {
      perror("ftruncate");
      exit(1);
    }

    {
      void* ptr = mmap(0,
                       sizeof(SharedData),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       m_shared_memory_fd,
                       0);
      if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
      }

      m_data = static_cast<SharedData*>(ptr);
    }

    m_semaphore = sem_open(m_semaphore_name.c_str(), O_CREAT, 0666, 1);
    if (m_semaphore == SEM_FAILED) {
      perror("sem_open");
      exit(1);
    }
  }

  ~SharedMemoryQueue()
  {
    munmap(m_data, sizeof(SharedData));
    close(m_shared_memory_fd);
    sem_close(m_semaphore);
  }

  void reset()
  {
    lock();
    memset(m_data, 0, sizeof(SharedData));
    unlock();
  }

  bool empty() const
  {
    lock();
    bool is_empty = size_unsafe() <= 0;
    unlock();
    return is_empty;
  }

  bool full() const
  {
    lock();
    bool is_full = size_unsafe() >= MaxSize;
    unlock();
    return is_full;
  }

  size_t size() const
  {
    lock();
    int size = size_unsafe();
    unlock();
    return size;
  }

  constexpr size_t capacity() const { return MaxSize; }

  void enqueue(const T& value)
  {
    lock();

    if (full_unsafe()) {
      unlock();
      throw std::overflow_error("Queue is full");
    }

    m_data->buffer[m_data->tail] = value;
    m_data->tail = (m_data->tail + 1) % MaxSize;
    ++m_data->count;

    unlock();
  }

  T dequeue()
  {
    lock();

    if (empty_unsafe()) {
      unlock();
      throw std::underflow_error("Queue is empty");
    }

    T value = m_data->buffer[m_data->head];
    m_data->head = (m_data->head + 1) % MaxSize;
    --m_data->count;

    unlock();

    return value;
  }

private:
  void lock() const { sem_wait(m_semaphore); }

  void unlock() const { sem_post(m_semaphore); }

  // Check if the queue is empty
  bool empty_unsafe() const { return size_unsafe() <= 0; }

  // Check if the queue is full
  bool full_unsafe() const { return size_unsafe() >= MaxSize; }

  // Current number of elements
  size_t size_unsafe() const { return m_data->count; }

private:
  int m_shared_memory_fd{0};
  SharedData* m_data{nullptr};
  sem_t* m_semaphore{nullptr};
  std::string m_shared_memory_name{};
  std::string m_semaphore_name{};
};
