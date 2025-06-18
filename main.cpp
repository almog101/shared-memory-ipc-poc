#include "SharedMemoryVector.hpp"

int
main()
{
  SharedVector<int, 20> vec{};
  vec.erase();
  
  printf("current length %d\n", vec.length());

  for (int i = 0; i < 10; i++) {
	printf("here %d\n", i);
    vec.push_back(i);
  }
  sleep(3);
  return 0;
}
