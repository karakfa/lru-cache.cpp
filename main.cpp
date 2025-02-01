#include <iostream>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <atomic>



template<typename K, typename V>
class LRUCache {
private:
    struct Node {
        K key;
        V value;
        Node* prev;
        Node* next;
        Node(const K& k, const V& v) : key(k), value(v), prev(nullptr), next(nullptr) {}
    };

    int capacity;
    std::unordered_map<K, Node*> cache{};
    Node* head;
    Node* tail;
    size_t hits{0};
    size_t misses{0};
    mutable std::shared_mutex mutex;

    void removeNode(Node* node) {
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        if (node == head) head = node->next;
        if (node == tail) tail = node->prev;
    }

    void addNodeToHead(Node* node) {
        node->next = head;
        node->prev = nullptr;
        if (head) head->prev = node;
        head = node;
        if (!tail) tail = head;
    }

    void moveToHead(Node* node) {
        removeNode(node);
        addNodeToHead(node);
    }

    void doReset() {
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
        cache.clear();
        tail = nullptr;
        resetStats();
    }

public:

    LRUCache(int cap) : capacity(cap), head(nullptr), tail(nullptr) {}

    LRUCache() {
        LRUCache(10);   // default size
    }

    std::optional<V> get(K key) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (cache.find(key) == cache.end()) {
            misses++;
            return {};
        }
        hits++;
        Node* node = cache[key];
        moveToHead(node);
        return node->value;
    }

    std::pair<size_t, size_t> getStats() const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return {hits, misses};
    }

    void resetStats() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        hits = 0;
        misses = 0;
    }

    void put(K key, V value) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (cache.find(key) != cache.end()) {
            Node* node = cache[key];
            node->value = value;
            moveToHead(node);
        } else {
            Node* newNode = new Node(key, value);
            if (cache.size() >= capacity) {
                Node* tailNode = tail;
                removeNode(tail);
                cache.erase(tailNode->key);
                delete tailNode;
            }
            cache[key] = newNode;
            addNodeToHead(newNode);
        }
    }

    void reset() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        doReset();
    }

    ~LRUCache() {
        doReset();
    }
};

template <typename K, typename V>
class CacheFactory {
private:
    static std::unordered_map<std::string, std::unique_ptr<LRUCache<K, V>>> caches;
    static std::recursive_mutex mutex;
    static const int DEFAULT_CACHE_SIZE{100};

public:
    static void createCache(const std::string& name, int capacity) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (caches.find(name) == caches.end()) {
            caches[name] = std::make_unique<LRUCache<K, V>>(capacity);
        }
    }

    static LRUCache<K, V>& getCache(const std::string& name) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (caches.find(name) == caches.end()) {
            createCache(name, DEFAULT_CACHE_SIZE);
        }
        return *caches[name];
    }
};

template <typename K, typename V>
std::unordered_map<std::string, std::unique_ptr<LRUCache<K, V>>> CacheFactory<K,V>::caches;

template <typename K, typename V>
std::recursive_mutex CacheFactory<K,V>::mutex;

void testLRUCache() {
    // Basic functionality tests
    CacheFactory<int, int>::createCache("test", 2);
    auto& cache = CacheFactory<int, int>::getCache("test");

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


    // Test with string keys and values
    CacheFactory<std::string, std::string>::createCache("str_test", 2);
    auto& strCache = CacheFactory<std::string, std::string>::getCache("str_test4");
    strCache.put(std::string{"pi"}, std::string{"3.14"});
    strCache.put(std::string{"e"}, std::string{"2.718"});
    std::cout << "Test 6: " << (strCache.get("pi").value_or("") == "3.14" ? "PASS" : "FAIL") << std::endl;

    std::cout << "Test 7: " << (!strCache.get("phi").has_value() ? "PASS" : "FAIL") << std::endl;
}
    // Multithreaded test
    void multithreadedTest() {
    std::cout << "\nRunning multithreaded test...\n";
    CacheFactory<int, int>::createCache("mt_test", 5);
    auto& mtCache = CacheFactory<int, int>::getCache("mt_test");
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

    // Start threads
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(threadFunc, i);
    }

    // Wait for completion
    for (auto& thread : threads) {
        thread.join();
    }

    auto [hits, misses] = mtCache.getStats();
    std::cout << "Multithreaded test completed\n";
    std::cout << "Cache hits: " << hits << "\n";
    std::cout << "Cache misses: " << misses << "\n";
    std::cout << "Successful retrievals: " << successCount << "\n";
}

int main() {
    std::cout << "Running LRU Cache tests...\n";
    testLRUCache();
    multithreadedTest();
    std::cout << "All tests done!\n";
    return 0;
}