
#include "lru_cache.h"
#include <iostream>
#include <chrono>

// Function declarations
void testLRUCache();
void multithreadedTest();

int main() {
    std::cout << "Running LRU Cache tests...\n";
    testLRUCache();
    multithreadedTest();
    std::cout << "All tests done!\n";
    return 0;
}
