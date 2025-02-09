#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <unordered_map>
#include <optional>
#include <mutex>
#include <iostream>
#include <shared_mutex>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <thread>

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

    const size_t capacity;
    const size_t cleanup_interval;
    std::unordered_map<K, Node*> cache;
    Node* head;
    Node* tail;
    mutable std::atomic<int> hits{0};
    mutable std::atomic<int> misses{0};
    mutable std::shared_mutex mutex;

    // related to timed reset of cache...
    std::thread cleanup_thread;
    volatile std::atomic<bool> runCleanup{true};
    std::condition_variable_any cv;

    void cleanupWorker() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        while (runCleanup) {
            // Wait with timeout, can be interrupted by cv.notify_one()
            cv.wait_for(lock, 
                       std::chrono::seconds(cleanup_interval),
                       [this] { return !runCleanup; });
            
            if (!runCleanup) {
                break;
            }
            
            std::cout << "cleanup worker evicting entries..." << std::endl;
            doReset();
        }
        std::cout << "cleanup worker exiting..." << std::endl;
    }

    void removeNode(Node* node) {
        std::cout << "removing Node..." << std::endl;
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        if (node == head) head = node->next;
        if (node == tail) tail = node->prev;
    }

    void addNodeToHead(Node* node) {
        std::cout << "addNodeToHead" << std::endl;
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
        std::cout << "doReset" << std::endl;
        Node* current = head;
        // swap with empty cache
        std::unordered_map<K, Node*> tempCache;
        std::swap(cache, tempCache);
        head = nullptr;
        tail = nullptr;
        while (current) {
            Node* temp = current;
            current = current->next;
            delete temp;
        }
        resetStats();
    }

public:
    explicit LRUCache(size_t cap, size_t cleanup_int_seconds) : 
        capacity(cap),
        cleanup_interval(cleanup_int_seconds),
        head(nullptr), 
        tail(nullptr) 
    {
        cleanup_thread = std::thread(&LRUCache<K,V>::cleanupWorker, this);
        std::cout << "cleanup will be every " << cleanup_interval << " secs" << std::endl;
    }

    LRUCache() : LRUCache(100, 60*60) {}

    std::optional<const V> get(const K& key) {
        std::shared_lock<std::shared_mutex> lock(mutex);
        if (cache.find(key) == cache.end()) {
            misses++;
            return {};
        }
        hits++;
        auto it = cache.find(key);
        Node* node = it->second;
        moveToHead(node);
        return node->value;
    }

    void put(const K& key, const V& value) {
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

    std::pair<size_t, size_t> getStats() const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return {hits, misses};
    }

    void resetStats() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        hits = 0;
        misses = 0;
    }

    void reset() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        doReset();
    }

    void stop_cleaner_thread() {
        std::cout << "trying to stop cleaner thread" << std::endl;
        runCleanup = false;
        cv.notify_one();
        if (cleanup_thread.joinable()) {
            std::cout << "cleaner thread is joinable, waiting to join" << std::endl;
            cleanup_thread.join();
        }
        std::cout << "cleaner thread stopped." << std::endl;
    }

    ~LRUCache() {
        std::cout << "in LRUCache destructor." << std::endl;
        stop_cleaner_thread();
        std::unique_lock<std::shared_mutex> lock(mutex);
        Node* current = head;
        head = nullptr;
        tail = nullptr;
        cache.clear();
        while (current) {
            Node* temp = current;
            current = current->next;
            delete temp;
        }
        std::cout << "LRUCache destroyed." << std::endl;
    }
};

template <typename K, typename V>
class CacheHolder {
private:
    std::unique_ptr<LRUCache<K, V>> cache;
    static const int DEFAULT_CACHE_SIZE{100};
    static const int DEFAULT_CACHE_CLEANUP_INTERVAL{60*60};

public:
    CacheHolder() : 
        cache(std::make_unique<LRUCache<K, V>>(DEFAULT_CACHE_SIZE, DEFAULT_CACHE_CLEANUP_INTERVAL)) {}

    CacheHolder(size_t capacity, size_t cleanup_interval = DEFAULT_CACHE_CLEANUP_INTERVAL) : 
        cache(std::make_unique<LRUCache<K, V>>(capacity, cleanup_interval)) {}

    LRUCache<K, V>& getCache() { return *cache; }
};

#endif