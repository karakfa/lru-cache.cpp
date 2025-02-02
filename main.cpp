
#include "tests.cpp"

int main() {
    std::cout << "Running LRU Cache tests...\n";
    testLRUCache();
    multithreadedTest();
    std::cout << "All tests done!\n";
    return 0;
}
