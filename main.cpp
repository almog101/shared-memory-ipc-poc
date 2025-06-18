#include "SharedMemoryQueue.hpp"
#include <cstdio>

int
main()
{
  SharedMemoryQueue<int, 20> queue{"my_queue"};

  while (!queue.empty())
  {
	  printf("poped %d\n", queue.dequeue());
  }

  for (int i = 0; i < 10; i++) {
	printf("here %d\n", i);
    queue.enqueue(i);
  }

  sleep(1);
  return 0;
}
