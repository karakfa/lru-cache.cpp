#include <iostream>
#include <unordered_map>

using namespace std;

class Node {
public:
    int key;
    int value;
    Node* prev;
    Node* next;

    Node(int k, int v) : key(k), value(v), prev(nullptr), next(nullptr) {}
};


class LRUCache {
private:
    int capacity;
    unordered_map<int, Node*> cache;
    Node* head;
    Node* tail;

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

    Node* removeTail() {
        Node* node = tail;
        removeNode(tail);
        return node;
    }

public:
    LRUCache(int cap) : capacity(cap), head(nullptr), tail(nullptr) {}

    int get(int key) {
        if (cache.find(key) == cache.end()) return -1;
        Node* node = cache[key];
        moveToHead(node);
        return node->value;
    }

    void put(int key, int value) {
        if (cache.find(key) != cache.end()) {
            Node* node = cache[key];
            node->value = value;
            moveToHead(node);
        } else {
            Node* newNode = new Node(key, value);
            if (cache.size() >= capacity) {
                Node* tailNode = removeTail();
                cache.erase(tailNode->key);
                delete tailNode;
            }
            cache[key] = newNode;
            addNodeToHead(newNode);
        }
    }
};





void testLRUCache() {
    // Test case 1: Basic operations
    LRUCache cache(2);
    
    // Test put operation
    cache.put(1, 1);
    cache.put(2, 2);
    std::cout << "Test 1: " << (cache.get(1) == 1 ? "PASS" : "FAIL") << std::endl;
    
    // Test eviction
    cache.put(3, 3);    // evicts key 2
    std::cout << "Test 2: " << (cache.get(2) == -1 ? "PASS" : "FAIL") << std::endl;
    
    // Test updating existing key
    cache.put(1, 4);    // updates value of key 1
    std::cout << "Test 3: " << (cache.get(1) == 4 ? "PASS" : "FAIL") << std::endl;
    
    // Test capacity constraint
    cache.put(4, 4);    // evicts key 3
    std::cout << "Test 4: " << (cache.get(3) == -1 ? "PASS" : "FAIL") << std::endl;
    
    // Test accessing existing key
    std::cout << "Test 5: " << (cache.get(4) == 4 ? "PASS" : "FAIL") << std::endl;
}

int main() {
    std::cout << "Running LRU Cache tests...\n";
    testLRUCache();
    return 0;
}