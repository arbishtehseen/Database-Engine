// ============================================================================
// test_persistence.cpp  -  tests for Persistence::Save / Persistence::Load
// ============================================================================
#include "persistence.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static int g_failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "FAIL line " << __LINE__ << ": " #cond << "\n";   \
            ++g_failed;                                                    \
        }                                                                  \
    } while (0)

static const char* kFile = "test_persistence_tmp.db";

static void Cleanup() {
    std::remove(kFile);
    std::remove((std::string(kFile) + ".tmp").c_str());
}

// 1) Save then Load gives back exactly the same records.
static void TestRoundTrip() {
    Cleanup();
    std::vector<KeyValue> in;
    in.push_back(KeyValue(1, "hello"));
    in.push_back(KeyValue(2, "world"));
    in.push_back(KeyValue(3, "third"));

    CHECK(Persistence::Save(kFile, in));

    std::vector<KeyValue> out;
    CHECK(Persistence::Load(kFile, &out));
    CHECK(out.size() == in.size());
    for (size_t i = 0; i < in.size() && i < out.size(); ++i) {
        CHECK(out[i].first == in[i].first);
        CHECK(out[i].second == in[i].second);
    }
    Cleanup();
}

// 2) Values with spaces, newlines and empty values survive.
static void TestTrickyValues() {
    Cleanup();
    std::vector<KeyValue> in;
    in.push_back(KeyValue(10, "has some spaces"));
    in.push_back(KeyValue(11, "line1\nline2\nline3"));
    in.push_back(KeyValue(12, ""));
    in.push_back(KeyValue(13, "  leading and trailing  "));

    CHECK(Persistence::Save(kFile, in));

    std::vector<KeyValue> out;
    CHECK(Persistence::Load(kFile, &out));
    CHECK(out.size() == in.size());
    for (size_t i = 0; i < in.size() && i < out.size(); ++i) {
        CHECK(out[i].first == in[i].first);
        CHECK(out[i].second == in[i].second);
    }
    Cleanup();
}

// 3) Saving over an existing file replaces it completely.
static void TestOverwrite() {
    Cleanup();
    std::vector<KeyValue> first;
    first.push_back(KeyValue(1, "old"));
    first.push_back(KeyValue(2, "old2"));
    CHECK(Persistence::Save(kFile, first));

    std::vector<KeyValue> second;
    second.push_back(KeyValue(5, "new"));
    CHECK(Persistence::Save(kFile, second));

    std::vector<KeyValue> out;
    CHECK(Persistence::Load(kFile, &out));
    CHECK(out.size() == 1);
    if (!out.empty()) {
        CHECK(out[0].first == 5);
        CHECK(out[0].second == "new");
    }
    Cleanup();
}

// 4) Missing file = first run: Load succeeds with empty result.
static void TestMissingFile() {
    Cleanup();
    std::vector<KeyValue> out;
    out.push_back(KeyValue(99, "junk"));  // must be cleared by Load
    CHECK(Persistence::Load(kFile, &out));
    CHECK(out.empty());
}

// 5) Empty list can be saved and loaded.
static void TestEmptyList() {
    Cleanup();
    std::vector<KeyValue> in;
    CHECK(Persistence::Save(kFile, in));

    std::vector<KeyValue> out;
    CHECK(Persistence::Load(kFile, &out));
    CHECK(out.empty());
    Cleanup();
}

// 6) Corrupt files are rejected and give an empty result.
static void TestCorruptFile() {
    Cleanup();

    {  // bad header
        std::ofstream f(kFile, std::ios::binary | std::ios::trunc);
        f << "NOTMINIDB 1\n0\n";
    }
    std::vector<KeyValue> out;
    CHECK(!Persistence::Load(kFile, &out));
    CHECK(out.empty());

    {  // truncated value: says 50 bytes but file ends early
        std::ofstream f(kFile, std::ios::binary | std::ios::trunc);
        f << "MINIDB 1\n1\n7 50 short\n";
    }
    CHECK(!Persistence::Load(kFile, &out));
    CHECK(out.empty());

    {  // absurd length
        std::ofstream f(kFile, std::ios::binary | std::ios::trunc);
        f << "MINIDB 1\n1\n7 99999999999 x\n";
    }
    CHECK(!Persistence::Load(kFile, &out));
    CHECK(out.empty());

    Cleanup();
}

int main() {
    TestRoundTrip();
    TestTrickyValues();
    TestOverwrite();
    TestMissingFile();
    TestEmptyList();
    TestCorruptFile();

    if (g_failed == 0) {
        std::cout << "All persistence tests passed.\n";
        return 0;
    }
    std::cerr << g_failed << " check(s) failed.\n";
    return 1;
}