#pragma once

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

template<typename T, size_t AMOUNT>
struct SharedVector
{
  struct SharedData
  {
    T array[AMOUNT];
    int write_index;
  };

  static constexpr const char* shm_name = "/my_shm";
  static constexpr const char* sem_name = "/my_sem";
  static constexpr int SIZE = sizeof(SharedData);

  SharedVector()
  {
    shared_memory_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shared_memory_fd == -1) {
      perror("shm_open");
      exit(1);
    }

    if (ftruncate(shared_memory_fd, SIZE) == -1) {
      perror("ftruncate");
      exit(1);
    }

    {
      void* ptr =
        mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd, 0);
      if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
      }

      data = static_cast<SharedData*>(ptr);
    }

    semaphore = sem_open(sem_name, O_CREAT, 0666, 1);
    if (semaphore == SEM_FAILED) {
      perror("sem_open");
      exit(1);
    }
  }

  ~SharedVector()
  {
    munmap(data, SIZE);
    close(shared_memory_fd);
    sem_close(semaphore);
  }

  int length()
  {
    lock();
    int write_index = data->write_index;
    unlock();

    return write_index;
  }

  void erase() 
  {
	  lock();
	  memset(data, 0, sizeof(*data));
	  data->write_index = 0;
	  unlock();
  }

  void push_back(T value) const
  {
    lock();

	if (data->write_index == AMOUNT - 1)
	{
		unlock();
		fprintf(stderr, "vec out of space\n");
		exit(1);
	}

    data->array[data->write_index] = value;
    data->write_index++;

    unlock();
  }

  void lock() const { sem_wait(semaphore); }

  void unlock() const { sem_post(semaphore); }

  int shared_memory_fd{0};
  SharedData* data{nullptr};
  sem_t* semaphore{nullptr};
};
