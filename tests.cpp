
#include "lru_cache.h"
#include <iostream>
#include <thread>
#include <vector>

void testLRUCache() {
    CacheHolder<int, int> holder(2);
    auto& cache = holder.getCache();
    
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

    CacheHolder<std::string, std::string> strHolder(2);
    auto& strCache = strHolder.getCache();
    strCache.put("pi", "3.14");
    strCache.put("e", "2.718");
    std::cout << "Test 6: " << (strCache.get("pi").value_or("") == "3.14" ? "PASS" : "FAIL") << std::endl;
    std::cout << "Test 7: " << (!strCache.get("phi").has_value() ? "PASS" : "FAIL") << std::endl;
}

void multithreadedTest() {
    std::cout << "\nRunning multithreaded test...\n";
    CacheHolder<int, int> mtHolder(5);
    auto& mtCache = mtHolder.getCache();
    std::vector<std::thread> threads;
    const int numThreads = 4;
    const int opsPerThread = 1000;
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
    }

    auto [hits, misses] = mtCache.getStats();
    std::cout << "Multithreaded test completed\n";
    std::cout << "Cache hits: " << hits << "\n";
    std::cout << "Cache misses: " << misses << "\n";
    std::cout << "Successful retrievals: " << successCount << "\n";
}
