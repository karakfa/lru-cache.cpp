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

using uint = unsigned int;

template<typename K, typename V>
class LRUCache {
private:
    struct Node {
        K key;
        V value;
        std::weak_ptr<Node> prev; // to eliminate circular reference...
        std::shared_ptr<Node> next;
        Node(const K& k, const V& v) : key(k), value(v) {}
    };

    const uint capacity;
    const uint cleanup_interval;
    std::unordered_map<K, std::shared_ptr<Node>> cache;
    std::shared_ptr<Node> head;
    std::shared_ptr<Node> tail;
    mutable std::atomic<uint> hits{0};
    mutable std::atomic<uint> misses{0};
    mutable std::shared_mutex mutex;

    // related to timed reset of cache...
    std::thread cleanup_thread;
    volatile std::atomic<bool> should_stop{false};
    std::condition_variable_any cv;

    void cleanupWorker() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        while (!should_stop) {
            auto status = cv.wait_for(lock, 
                       std::chrono::seconds(cleanup_interval),
                       [this] { return should_stop; });

            if (status) {
                break;
            }

            std::cout << "cleanup worker evicting entries..." << std::endl;
            head.reset();
            tail.reset();
            cache.clear();
            hits = 0;
            misses = 0;
            std::cout << "cleared entries" << std::endl;
        }
        std::cout << "cleanup worker exiting..." << std::endl;
    }

    void removeNode(std::shared_ptr<Node> node) {
        std::cout << "removing Node..." << std::endl;
        // if prevNode is around...
        if (auto prevNode = node->prev.lock()) {
            prevNode->next = node->next;
        }
        if (node->next) {
            node->next->prev = node->prev;
        }
        if (node == head) head = node->next;
        if (node == tail) tail = (node->prev.lock());
    }

    void addNodeToHead(std::shared_ptr<Node> node) {
        std::cout << "addNodeToHead" << std::endl;
        node->next = head;
        node->prev.reset();
        if (head) {
            head->prev = node;
        }
        head = node;
        if (!tail) tail = head;
    }

    void moveToHead(std::shared_ptr<Node> node) {
        removeNode(node);
        addNodeToHead(node);
    }

    void doReset() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        std::cout << "doReset" << std::endl;
        head.reset();
        tail.reset();
        cache.clear();
        std::cout << "cleared entries" << std::endl;
        resetStats();
        std::cout << "doReset done" << std::endl;
    }

public:
    explicit LRUCache(uint cap, uint cleanup_int_seconds) : 
        capacity(cap),
        cleanup_interval(cleanup_int_seconds) {
        cleanup_thread = std::thread(&LRUCache<K,V>::cleanupWorker, this);
        std::cout << "cleanup will be every " << cleanup_interval << " secs" << std::endl;
    }

    LRUCache() : LRUCache(100, 60*60) {}

    std::optional<const V> get(const K& key) {
        std::cout << "getting " << key <<  std::endl;
        std::shared_lock<std::shared_mutex> lock(mutex);
        if (cache.find(key) == cache.end()) {
            misses++;
            return {};
        }
        hits++;
        auto node = cache[key];
        moveToHead(node);
        return node->value;
    }

    void put(const K& key, const V& value) {
        std::cout << "putting " << key << " with value " << value << std::endl;
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (cache.find(key) != cache.end()) {
            // overwriting existing entry...
            auto node = cache[key];
            node->value = value;
            moveToHead(node);
        } else {
            // make new entry...
            auto newNode = std::make_shared<Node>(key, value);
            if (cache.size() >= capacity) {
                // drop the least recently used entry...
                if (tail) {
                    cache.erase(tail->key);
                    removeNode(tail);
                }
            }
            cache[key] = newNode;
            addNodeToHead(newNode);
        }
    }

    std::pair<uint, uint> getStats() const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return {hits, misses};
    }

    void resetStats() {
        hits = 0;
        misses = 0;
        std::cout << "resetting stats done." << std::endl;
    }

    void reset() {
        std::unique_lock<std::shared_mutex> lock(mutex);
        doReset();
    }

    void stop_cleaner_thread() {
        std::cout << "stopping cleaner thread" << std::endl;
        should_stop = true;
        cv.notify_one();
        if (cleanup_thread.joinable()) {
            cleanup_thread.join();
        }
    }

    ~LRUCache() {
        std::cout << "in LRUCache destructor." << std::endl;
        stop_cleaner_thread();
        std::unique_lock<std::shared_mutex> lock(mutex);
        doReset();
        std::cout << "LRUCache destroyed." << std::endl;
    }
};

#endif