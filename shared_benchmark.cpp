#include <array>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include <cstring>
#include "SharedMemoryQueue.hpp"
#include "benchmark.h"

int main() {
	{
		SharedMemoryQueue<std::array<uint8_t, BUFFER_SIZE>, NUM_MESSAGES> queue{"bench"};
		queue.reset();
	}

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 2;
    }

    if (pid == 0) {
        // Child process - read from pipe
		SharedMemoryQueue<std::array<uint8_t, BUFFER_SIZE>, NUM_MESSAGES> queue{"bench"};

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
			auto buffer{queue.dequeue_block()};
        }

        return 0;
    } else {
		SharedMemoryQueue<std::array<uint8_t, BUFFER_SIZE>, NUM_MESSAGES> queue{"bench"};

		std::array<uint8_t, BUFFER_SIZE> buffer{};
		buffer.fill('A');

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
			queue.enqueue(buffer);
        }

        wait(nullptr);    // Wait for child to finish

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;

        size_t total_bytes = BUFFER_SIZE * NUM_MESSAGES;
        double throughput_MBps = (total_bytes / 1e6) / duration.count();

        std::cout << "Sent " << total_bytes << " bytes in " 
                  << duration.count() << " seconds\n";
        std::cout << "Throughput: " << throughput_MBps << " MB/s\n";
    }

    return 0;
}
