
#ifndef LRU_CACHE_TPP
#define LRU_CACHE_TPP

template<typename K, typename V>
LRUCache<K,V>::LRUCache(size_t cap) : capacity(cap), head(nullptr), tail(nullptr) {}

template<typename K, typename V>
LRUCache<K,V>::LRUCache() : LRUCache(100) {}

template<typename K, typename V>
void LRUCache<K,V>::removeNode(Node* node) {
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == head) head = node->next;
    if (node == tail) tail = node->prev;
}

template<typename K, typename V>
void LRUCache<K,V>::addNodeToHead(Node* node) {
    node->next = head;
    node->prev = nullptr;
    if (head) head->prev = node;
    head = node;
    if (!tail) tail = head;
}

template<typename K, typename V>
void LRUCache<K,V>::moveToHead(Node* node) {
    removeNode(node);
    addNodeToHead(node);
}

template<typename K, typename V>
std::optional<const V> LRUCache<K,V>::get(const K& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (cache.find(key) == cache.end()) {
        misses++;
        return {};
    }
    hits++;
    Node* node = cache[key];
    moveToHead(node);
    return node->value;
}

template<typename K, typename V>
void LRUCache<K,V>::put(const K& key, const V& value) {
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

template<typename K, typename V>
std::pair<size_t, size_t> LRUCache<K,V>::getStats() const {
    std::shared_lock<std::shared_mutex> lock(mutex);
    return {hits, misses};
}

template<typename K, typename V>
void LRUCache<K,V>::resetStats() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    hits = 0;
    misses = 0;
}

template<typename K, typename V>
void LRUCache<K,V>::doReset() {
    while (head) {
        Node* temp = head;
        head = head->next;
        delete temp;
    }
    cache.clear();
    tail = nullptr;
    resetStats();
}

template<typename K, typename V>
void LRUCache<K,V>::reset() {
    std::unique_lock<std::shared_mutex> lock(mutex);
    doReset();
}

template<typename K, typename V>
LRUCache<K,V>::~LRUCache() {
    doReset();
}

// CacheHolder is fully defined in the header

#endif
