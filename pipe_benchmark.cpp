#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <sys/wait.h>
#include "benchmark.h"

int main() {
    int sv[2]; // Socket pair

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1) {
        perror("socketpair");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return 2;
    }

    if (pid == 0) {
        // Child process - receive data
        close(sv[0]); // Close unused end

        char buffer[BUFFER_SIZE];
        size_t total_bytes = 0;

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            ssize_t bytes = recv(sv[1], buffer, BUFFER_SIZE, 0);
            if (bytes <= 0) {
                break;
            }
            total_bytes += bytes;
        }

        close(sv[1]);
        std::cerr << "Child received: " << total_bytes << " bytes\n";
        return 0;
    } else {
        // Parent process - send data
        close(sv[1]); // Close unused end

        char buffer[BUFFER_SIZE];
        std::memset(buffer, 'B', BUFFER_SIZE);

        auto start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            ssize_t sent = send(sv[0], buffer, BUFFER_SIZE, 0);
            if (sent != BUFFER_SIZE) {
                std::cerr << "Send error\n";
                break;
            }
        }

        close(sv[0]); // Signal EOF
        wait(nullptr); // Wait for child

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
