
#include <iostream>
#include <unordered_map>
#include <optional>

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

public:
    LRUCache(int cap) : capacity(cap), head(nullptr), tail(nullptr) {}

    std::optional<V> get(K key) {
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
        return {hits, misses};
    }

    void resetStats() {
        hits = 0;
        misses = 0;
    }

    void put(K key, V value) {
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
        while (head) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
        cache.clear();
        tail = nullptr;
    }

    ~LRUCache() {
        reset();
    }
};

void testLRUCache() {
    // Test with int keys and values
    LRUCache<int, int> cache(2);

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

    // Test with string keys and double values
    LRUCache<std::string, std::string> strCache(2);
    strCache.put(std::string{"pi"}, std::string{"3.14"});
    strCache.put(std::string{"e"}, std::string{"2.718"});
    std::cout << "Test 6: " << (strCache.get("pi").value_or("") == "3.14" ? "PASS" : "FAIL") << std::endl;

    std::cout << "Test 7: " << (!strCache.get("phi").has_value() ? "PASS" : "FAIL") << std::endl;
}

int main() {
    std::cout << "Running LRU Cache tests...\n";
    testLRUCache();
    return 0;
}
