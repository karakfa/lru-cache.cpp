
#include <iostream>
#include <unordered_map>
#include <memory>

using namespace std;

template<typename K, typename V>
class LRUCache {
private:
    struct Node {
        K key;
        V value;
        unique_ptr<Node> next;
        weak_ptr<Node> prev;  // Use weak_ptr to avoid circular reference
        Node(K k, V v) : key(k), value(v) {}
    };

    int capacity;
    unordered_map<K, shared_ptr<Node>> cache;
    shared_ptr<Node> head;
    shared_ptr<Node> tail;

    void removeNode(shared_ptr<Node> node) {
        if (auto prevNode = node->prev.lock()) {
            prevNode->next = std::move(node->next);
            if (node->next) {
                node->next->prev = prevNode;
            }
        } else {
            head = std::move(node->next);
            if (head) {
                head->prev.reset();
            }
        }
        
        if (node == tail) {
            tail = node->prev.lock();
        }
    }

    void addNodeToHead(shared_ptr<Node> node) {
        node->next = nullptr;
        node->prev.reset();
        if (head) {
            node->next = std::move(head);
            node->next->prev = node;
        }
        head = node;
        if (!tail) {
            tail = head;
        }
    }

    void moveToHead(shared_ptr<Node> node) {
        if (node != head) {
            removeNode(node);
            addNodeToHead(node);
        }
    }

public:
    LRUCache(int cap) : capacity(cap) {}

    V get(K key) {
        if (cache.find(key) == cache.end()) {
            throw runtime_error("Key not found");
        }
        auto node = cache[key];
        moveToHead(node);
        return node->value;
    }

    void put(K key, V value) {
        if (cache.find(key) != cache.end()) {
            cache[key]->value = value;
            moveToHead(cache[key]);
        } else {
            auto newNode = make_shared<Node>(key, value);
            if (cache.size() >= capacity) {
                if (tail) {
                    cache.erase(tail->key);
                    auto prevTail = tail->prev.lock();
                    if (prevTail) {
                        prevTail->next = nullptr;
                        tail = prevTail;
                    } else {
                        head = nullptr;
                        tail = nullptr;
                    }
                }
            }
            cache[key] = newNode;
            addNodeToHead(newNode);
        }
    }
};

void testLRUCache() {
    LRUCache<int, string> cache(2);
    
    cache.put(1, "one");
    cache.put(2, "two");
    cout << "Test 1: " << (cache.get(1) == "one" ? "PASS" : "FAIL") << endl;
    
    cache.put(3, "three");  // evicts 2
    try {
        cache.get(2);
        cout << "Test 2: FAIL" << endl;
    } catch (runtime_error&) {
        cout << "Test 2: PASS" << endl;
    }
    
    cache.put(1, "ONE");
    cout << "Test 3: " << (cache.get(1) == "ONE" ? "PASS" : "FAIL") << endl;
}

int main() {
    cout << "Running LRU Cache tests...\n";
    testLRUCache();
    return 0;
}
