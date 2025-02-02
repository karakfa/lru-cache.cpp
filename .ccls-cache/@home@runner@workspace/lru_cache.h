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
    /**
     * Constructs an LRU cache with specified capacity
     * @param cap The maximum number of key-value pairs the cache can hold
     * @throws std::invalid_argument if capacity is 0
     */
    explicit LRUCache(size_t cap);

    /**
     * Constructs an LRU cache with default capacity of 100
     */
    LRUCache();

    /**
     * Retrieves a value associated with the given key
     * @param key The key to look up
     * @return Optional containing the value if found, empty if not found
     */
    std::optional<V> get(const K& key);

    /**
     * Returns cache hit/miss statistics
     * @return Pair of (hits, misses) counts
     */
    std::pair<size_t, size_t> getStats() const;

    /**
     * Resets the hit/miss statistics to zero
     */
    void resetStats();

    /**
     * Inserts or updates a key-value pair in the cache
     * @param key The key to insert/update
     * @param value The value to associate with the key
     */
    void put(K key, V value);

    /**
     * Clears all entries from the cache
     */
    void reset();

    /**
     * Destructor - cleans up all allocated nodes
     */
    ~LRUCache();
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

#include "lru_cache.tpp"  // Template implementation
#endif