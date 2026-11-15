#pragma once
// ============================================================================
// lru_cache.h  -  LRU (Least Recently Used) cache
//
// Two structures work together:
//   1) HashTable : key -> pointer to its node.   Finds a node in O(1).
//   2) Doubly linked list : keeps the ORDER of use.
//        head_ = most recently used,  tail_ = least recently used.
//      Because each node has prev AND next, a node can be unlinked and moved
//      to the front in O(1), without searching.
//
// When the cache is full and a new key arrives, the tail (oldest) is evicted.
// ============================================================================
#include <cstddef>
#include <string>

#include "common.h"
#include "hash_table.h"

class LRUCache {
public:
    explicit LRUCache(size_t capacity);
    ~LRUCache();

    // Copying would make two caches own the same nodes -> disabled.
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // Hit: copies value to *out, marks key as most recently used, returns true.
    // Miss: returns false.
    bool Get(Key key, std::string* out);

    // Insert or update. Becomes the most recently used. Evicts the oldest if full.
    void Put(Key key, const std::string& value);

    // Remove a key (used on DELETE so the cache never returns a deleted key).
    void Remove(Key key);

    size_t Hits() const { return hits_; }
    size_t Misses() const { return misses_; }
    size_t Size() const { return size_; }

private:
    // One entry of the doubly linked list.
    struct Node {
        Key key;            // needed so eviction can erase the key from the map
        std::string value;
        Node* prev;         // towards head (newer)
        Node* next;         // towards tail (older)
    };

    void Unlink(Node* n);     // take node out of the list (does NOT delete it)
    void PushFront(Node* n);  // put node at the head (most recently used)
    void EvictTail();         // remove the least recently used entry

    size_t capacity_;  // maximum number of entries
    size_t size_;      // current number of entries
    size_t hits_;
    size_t misses_;
    Node* head_;       // most recently used (nullptr if empty)
    Node* tail_;       // least recently used (nullptr if empty)
    HashTable<Node*> map_;  // key -> node
};