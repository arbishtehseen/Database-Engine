// ============================================================================
// test_hash_table.cpp  -  tests for HashTable (plain main + check(), no framework)
// std::unordered_map is used ONLY here, as a "checker" (oracle).
// ============================================================================
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>

#include "hash_table.h"

static int failures = 0;

// If cond is false: print the message and count a failure.
static void check(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << "\n";
        ++failures;
    }
}

int main() {
    // ---- Test 1: basic put / get / update / remove -------------------------
    {
        HashTable<int> t;
        check(t.Put(1, 10), "new key returns true");
        check(!t.Put(1, 11), "updating existing key returns false");
        int v = 0;
        check(t.Get(1, &v) && v == 11, "update stored the new value");
        check(!t.Get(2, &v), "missing key is not found");
        check(t.Remove(1) && t.Size() == 0, "remove existing key");
        check(!t.Remove(1), "removing a missing key returns false");
    }

    // ---- Test 2: collisions + removing from the middle of a chain ----------
    {
        HashTable<int> t;
        // With 17 buckets, keys 0, 17 and 34 all land in bucket 0.
        t.Put(0, 100);
        t.Put(17, 117);
        t.Put(34, 134);
        int v = 0;
        check(t.BucketCount() == 17, "3 keys must not trigger a rehash yet");
        check(t.Get(0, &v) && v == 100, "collision: key 0 found");
        check(t.Get(17, &v) && v == 117, "collision: key 17 found");
        check(t.Get(34, &v) && v == 134, "collision: key 34 found");

        // New nodes go to the front, so the chain is 34 -> 17 -> 0.
        // Key 17 is therefore in the MIDDLE.
        check(t.Remove(17), "remove middle of chain");
        check(!t.Get(17, &v), "middle key is gone");
        check(t.Get(0, &v) && v == 100, "last node survives");
        check(t.Get(34, &v) && v == 134, "first node survives");

        check(t.Remove(34), "remove first node of chain");
        check(t.Remove(0), "remove last node of chain");
        check(t.Size() == 0, "table empty after removing everything");
    }

    // ---- Test 3: rehash keeps every key ------------------------------------
    {
        HashTable<int> t;
        for (int i = 0; i < 1000; ++i) t.Put(i, i * 2);
        check(t.Size() == 1000, "size is 1000");
        check(t.BucketCount() > 17, "rehash made the table bigger");
        check(t.LoadFactor() <= 0.75, "load factor stays <= 0.75");

        bool all_found = true;
        for (int i = 0; i < 1000; ++i) {
            int v = -1;
            if (!t.Get(i, &v) || v != i * 2) all_found = false;
        }
        check(all_found, "every key and value survives rehashing");
    }

    // ---- Test 4: negative and extreme keys ---------------------------------
    {
        HashTable<int> t;
        for (int i = -50; i < 0; ++i) t.Put(i, i);
        bool all_found = true;
        for (int i = -50; i < 0; ++i) {
            int v = 0;
            if (!t.Get(i, &v) || v != i) all_found = false;
        }
        check(all_found, "negative keys work");
        check(t.Put(INT64_MIN, 1) && t.Put(INT64_MAX, 2), "extreme keys work");
    }

    // ---- Test 5: random operations compared with std::unordered_map --------
    {
        HashTable<int> t;
        std::unordered_map<Key, int> oracle;  // the trusted reference
        std::mt19937 rng(12345);              // fixed seed -> same run every time
        bool same = true;

        for (int i = 0; i < 20000; ++i) {
            Key k = static_cast<Key>(rng() % 500) - 250;  // keys in [-250, 249]
            int op = static_cast<int>(rng() % 3);

            if (op == 0) {  // put
                int val = static_cast<int>(rng());
                bool is_new = oracle.find(k) == oracle.end();
                if (t.Put(k, val) != is_new) same = false;
                oracle[k] = val;
            } else if (op == 1) {  // remove
                bool existed = oracle.erase(k) > 0;
                if (t.Remove(k) != existed) same = false;
            } else {  // get
                int v = 0;
                auto it = oracle.find(k);
                bool got = t.Get(k, &v);
                if (got != (it != oracle.end())) same = false;
                else if (got && v != it->second) same = false;
            }
            if (t.Size() != oracle.size()) same = false;
        }
        check(same, "behaves exactly like std::unordered_map");
    }

    // ---- Test 6: pointer values (the LRU cache stores Node* in it) ---------
    {
        HashTable<int*> t;
        int x = 5;
        t.Put(1, &x);
        int* p = nullptr;
        check(t.Get(1, &p) && p == &x, "pointer values work");
    }

    if (failures == 0) std::cout << "test_hash_table: all passed\n";
    return failures == 0 ? 0 : 1;
}