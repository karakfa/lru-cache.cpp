#include "lru_cache.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>





void testLRUCache() {
    static LRUCache<int,int> cache = LRUCache<int,int>(2,3600);

    cache.put(1, 1);
    cache.put(2, 2);
    std::cout << "Test 1: " << (cache.get(1).value_or(0) == 1 ? "PASS" : "FAIL") << std::endl;

    cache.put(3, 3);    // evicts key 2
    std::cout << "Test 2: " << (!cache.get(2).has_value() ? "PASS" : "FAIL") << std::endl;

    cache.put(1, 4);    // updates value of key 1
    std::cout << "Test 3: " << (cache.get(1).value_or(0) == 4 ? "PASS" : "FAIL") << std::endl;

    cache.put(4, 4);    // evicts key 3
    std::cout << "Test 4: " << (!cache.get(3).has_value() ? "PASS" : "FAIL") << std::endl;

    std::cout << "Test 5: " << (cache.get(4).value_or(0) == 4 ? "PASS" : "FAIL") << std::endl;
    auto [hits, misses] = cache.getStats();
    std::cout << "First test completed\n";
    std::cout << "Cache hits: " << hits << "\n";
    std::cout << "Cache misses: " << misses << "\n";
   // cache.stop_cleaner_thread();
}
void testLRUCacheString()
{
    static LRUCache<std::string,std::string> strCache = LRUCache<std::string,std::string>(2,3600);

    strCache.put("pi", "3.14");
    strCache.put("e", "2.718");
    std::cout << "Test 6: " << (strCache.get("pi").value_or("") == "3.14" ? "PASS" : "FAIL") << std::endl;
    std::cout << "Test 7: " << (!strCache.get("phi").has_value() ? "PASS" : "FAIL") << std::endl;
    auto [hits, misses] = strCache.getStats();
    std::cout << "test completed\n";
    std::cout << "Cache hits: " << hits << "\n";
    std::cout << "Cache misses: " << misses << "\n";

  //  strCache.stop_cleaner_thread();
}

void multithreadedTest() {
    std::cout << "\nRunning multithreaded test...\n";
    static LRUCache<int,int> mtCache = LRUCache<int,int>(2,3600);
    std::vector<std::thread> threads;
    const int numThreads = 4;
    const int opsPerThread = 100;
    std::atomic<int> successCount{0};

    auto threadFunc = [&](int threadId) {
        for (int i = 0; i < opsPerThread; i++) {
            int key = (threadId * opsPerThread + i) % 10;
            if (i % 2 == 0) {
                mtCache.put(key, threadId);
            } 
            if (i % 3 == 0 ) {
                auto val = mtCache.get(key);
                if (val.has_value()) {
                    successCount++;
                }
            }
        }
    };

    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(threadFunc, i);
    }

    for (auto& thread : threads) {
        thread.join();
        std::cout << "thread " << thread.get_id() << "joined\n";
    }

    auto [hits, misses] = mtCache.getStats();
    std::cout << "Multithreaded test completed\n";
    std::cout << "Cache hits: " << hits << "\n";
    std::cout << "Cache misses: " << misses << "\n";
    std::cout << "Successful retrievals: " << successCount << "\n";
  //  mtCache.stop_cleaner_thread();
}

void testCleanupWorker() {
    static LRUCache<int,int> cleanupCache = LRUCache<int,int>(2,3);
    // Reduced cleanup interval (3s) for testing
    
    cleanupCache.put(1,1);
    cleanupCache.put(2,2);
    std::cout << "caller is waiting for cleanup thread to do the work\n";
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait longer than the cleanup interval
    std::cout << "Cleanup test: " << (!cleanupCache.get(1).has_value() ? "PASS" : "FAIL") << std::endl;


    std::cout << "Continue using cache after cleanup thread finishes\n";
    cleanupCache.put(1,1);
    cleanupCache.put(2,2);
    cleanupCache.put(3,3);
    cleanupCache.put(4,4);
    cleanupCache.put(5,5);
    cleanupCache.put(6,6);
    std::cout << "Cleanup test after first cleanup: " << (cleanupCache.get(5).has_value() ? "PASS" : "FAIL") << std::endl;
    
    std::cout << "caller is waiting for cleanup thread to do the work\n";
    std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait longer than the cleanup interval
    std::cout << "Cleanup test: " << (!cleanupCache.get(1).has_value() ? "PASS" : "FAIL") << std::endl;
    
}

int main() {
    auto start = std::chrono::high_resolution_clock::now();

    std::cout << "\n=== Running Basic LRU Cache Tests ===\n";
    testLRUCache();
    std::cout << "\n=== Running Basic LRU Cache Tests with String Keys ===\n";
    testLRUCacheString();
    std::cout << "\n=== Running Multithreaded Tests ===\n";
    multithreadedTest();
    std::cout << "\n=== Running Cleanup Worker Test ===\n";
    testCleanupWorker();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "All tests done in " << duration.count() << "ms!\n";
    return 0;
}