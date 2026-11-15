// ============================================================================
// test_lru_cache.cpp  -  tests for LRUCache (plain main + check())
// ============================================================================
#include <iostream>
#include <string>

#include "lru_cache.h"

static int failures = 0;

static void check(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    }
}

int main() {
    // ---- Test 1: the classic eviction case ---------------------------------
    // capacity 3: put 1, 2, 3, get 1, put 4  ->  key 2 must be evicted.
    {
        LRUCache c(3);
        std::string v;
        c.Put(1, "a");
        c.Put(2, "b");
        c.Put(3, "c");
        check(c.Get(1, &v) && v == "a", "get 1 is a hit");  // 1 is now newest
        c.Put(4, "d");                                      // cache full: evict oldest (2)
        check(!c.Get(2, &v), "key 2 was evicted");
        check(c.Get(1, &v), "key 1 still cached");
        check(c.Get(3, &v), "key 3 still cached");
        check(c.Get(4, &v), "key 4 cached");
        check(c.Size() == 3, "size stays at capacity");
    }

    // ---- Test 2: updating a key moves it to the front ----------------------
    {
        LRUCache c(2);
        std::string v;
        c.Put(1, "a");
        c.Put(2, "b");
        c.Put(1, "a2");  // update: 1 becomes the newest, 2 the oldest
        c.Put(3, "c");   // evicts 2
        check(!c.Get(2, &v), "2 evicted after 1 was updated");
        check(c.Get(1, &v) && v == "a2", "1 keeps its updated value");
    }

    // ---- Test 3: remove ----------------------------------------------------
    {
        LRUCache c(3);
        std::string v;
        c.Put(1, "a");
        c.Put(2, "b");
        c.Remove(1);
        check(!c.Get(1, &v), "removed key is gone");
        check(c.Size() == 1, "size shrinks after remove");
        c.Remove(99);  // removing a missing key must not crash
        c.Remove(2);
        c.Put(5, "e");
        check(c.Get(5, &v), "cache still works after becoming empty");
    }

    // ---- Test 4: hit / miss counters ---------------------------------------
    {
        LRUCache c(2);
        std::string v;
        c.Put(1, "a");
        c.Get(1, &v);  // hit
        c.Get(1, &v);  // hit
        c.Get(7, &v);  // miss
        check(c.Hits() == 2, "2 hits counted");
        check(c.Misses() == 1, "1 miss counted");
    }

    // ---- Test 5: tiny capacities -------------------------------------------
    {
        std::string v;
        LRUCache zero(0);
        zero.Put(1, "a");
        check(!zero.Get(1, &v) && zero.Size() == 0, "capacity 0 stores nothing");

        LRUCache one(1);
        one.Put(1, "a");
        one.Put(2, "b");  // evicts 1
        check(!one.Get(1, &v), "capacity 1: old key evicted");
        check(one.Get(2, &v) && v == "b", "capacity 1: new key kept");
    }

    // ---- Test 6: many keys, heavy eviction, negative keys ------------------
    {
        LRUCache c(16);
        std::string v;
        for (Key k = -500; k < 500; ++k) c.Put(k, std::to_string(k));
        check(c.Size() == 16, "size never exceeds capacity");
        check(c.Get(499, &v) && v == "499", "newest key is cached");
        check(!c.Get(-500, &v), "oldest key was evicted");
    }

    if (failures == 0) std::cout << "test_lru_cache: all passed\n";
    return failures == 0 ? 0 : 1;
}