
#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <unordered_map>
#include <optional>
#include <mutex>
#include <iostream>
#include <shared_mutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <stdio.h>

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

    size_t capacity;
    std::unordered_map<K, Node*> cache;
    Node* head;
    Node* tail;
    mutable std::atomic<int> hits{0};
    mutable std::atomic<int> misses{0};
    mutable std::shared_mutex mutex;
    std::thread cleanup_thread;
    volatile std::atomic<bool> should_stop{false};

    void cleanupWorker() {
        while (!should_stop) {
            for (int i = 0; i < 60 && !should_stop; i++) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!should_stop) {
                std::unique_lock<std::shared_mutex> lock(mutex);
                doReset();
            }
        }
        std::cout << "cleanup worker exiting..." << std::endl;
    }

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
    explicit LRUCache(size_t cap) : capacity(cap), head(nullptr), tail(nullptr) {
        cleanup_thread = std::thread(&LRUCache<K,V>::cleanupWorker, this);
    }

    LRUCache() : LRUCache(100) {}

    std::optional<const V> get(const K& key) const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        if (cache.find(key) == cache.end()) {
            misses++;
            return {};
        }
        hits++;
        auto it = cache.find(key);
        Node* node = it->second;
        const_cast<LRUCache*>(this)->moveToHead(node);
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
        should_stop = true;
        if (cleanup_thread.joinable()) {
            std::cout << "cleaner thread is joinable, waiting to join" << std::endl;
            cleanup_thread.join();
        }
    }

    ~LRUCache() {
        stop_cleaner_thread();
        doReset();
    }
};

template <typename K, typename V>
class CacheHolder {
private:
    std::unique_ptr<LRUCache<K, V>> cache;
    static const int DEFAULT_CACHE_SIZE{100};

public:
    CacheHolder(int capacity = DEFAULT_CACHE_SIZE) : cache(std::make_unique<LRUCache<K, V>>(capacity)) {}
    LRUCache<K, V>& getCache() { return *cache; }
};

#endif
