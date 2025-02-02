
#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <unordered_map>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <memory>
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

    size_t capacity;
    std::unordered_map<K, Node*> cache;
    Node* head;
    Node* tail;
    std::atomic<int> hits{0};
    std::atomic<int> misses{0};
    mutable std::shared_mutex mutex;

    void removeNode(Node* node);
    void addNodeToHead(Node* node);
    void moveToHead(Node* node);
    void doReset();

public:
    explicit LRUCache(size_t cap);
    LRUCache();
    std::optional<V> get(K key);
    std::pair<size_t, size_t> getStats() const;
    void resetStats();
    void put(K key, V value);
    void reset();
    ~LRUCache();
};

template <typename K, typename V>
class CacheFactory {
private:
    static std::unordered_map<std::string, std::unique_ptr<LRUCache<K, V>>> caches;
    static std::recursive_mutex mutex;
    static const int DEFAULT_CACHE_SIZE{100};

public:
    static void createCache(const std::string& name, int capacity);
    static LRUCache<K, V>& getCache(const std::string& name);
};

#include "lru_cache.tpp"  // Template implementation
#endif
