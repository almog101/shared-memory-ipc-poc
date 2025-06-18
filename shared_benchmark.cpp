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
		SharedMemoryQueue<NUM_MESSAGES, BUFFER_SIZE> queue{"bench"};
		queue.reset();
	}

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 2;
    }

    if (pid == 0) {
        // Child process - read from pipe
		SharedMemoryQueue<NUM_MESSAGES, BUFFER_SIZE> queue{"bench"};

        uint8_t buffer[BUFFER_SIZE];
        size_t total_bytes = 0;

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            size_t bytes = queue.dequeue_block(buffer, BUFFER_SIZE);
            /*if (bytes <= 0) {
                break;
            }*/
            total_bytes += bytes;
        }

        std::cerr << "Child received: " << total_bytes << " bytes\n";

        return 0;
    } else {
		SharedMemoryQueue<NUM_MESSAGES, BUFFER_SIZE> queue{"bench"};

        uint8_t buffer[BUFFER_SIZE];
        std::memset(buffer, 'A', BUFFER_SIZE);

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            ssize_t sent = queue.enqueue(buffer, BUFFER_SIZE);
            if (sent != BUFFER_SIZE) {
                std::cerr << "Send error\n";
                break;
            }
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
