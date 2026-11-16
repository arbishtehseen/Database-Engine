#include "database.h"
#include "persistence.h"
#include "sorting.h"
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

Database::Database(const std::string& fileName,
                   std::unique_ptr<Index> indexToUse,
                   std::size_t cacheCapacity)
    : file(fileName), index(std::move(indexToUse)), cache(cacheCapacity) {
    if (!index) {
        throw std::invalid_argument("Database needs an index");
    }
    if (!file.empty()) {
        std::vector<KeyValue> loaded;
        if (Persistence::Load(file, &loaded)) {
            for (const KeyValue& kv : loaded) {
                index->Put(kv.first, kv.second);
            }
        }
    }
}

Database::~Database() {
    if (!file.empty()) {
        std::vector<KeyValue> all = index->Range(
            std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
        Persistence::Save(file, all);
    }
}

bool Database::Put(Key key, const std::string& value) {
    std::string oldValue;
    bool existed = index->Get(key, &oldValue);
    index->Put(key, value);

    UndoEntry entry;
    entry.key = key;
    if (existed) {
        entry.action = UndoAction::RESTORE;
        entry.value = oldValue;
    } else {
        entry.action = UndoAction::REMOVE;
    }
    undoStack.Push(entry);

    cache.Put(key, value);

    if (!file.empty()) {
        std::vector<KeyValue> all = index->Range(
            std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
        Persistence::Save(file, all);
    }
    return !existed;
}

bool Database::Get(Key key, std::string* out) {
    // 1. Pehle cache mein check karein
    if (cache.Get(key, out)) {
        return true;
    }

    // 2. Cache miss par index se nikalein aur cache mein save karein
    bool found = index->Get(key, out);
    if (found && out != nullptr) {
        cache.Put(key, *out);
    }
    return found;
}

bool Database::Delete(Key key) {
    std::string oldValue;
    if (!index->Get(key, &oldValue)) {
        return false;
    }
    index->Remove(key);

    UndoEntry entry;
    entry.action = UndoAction::RESTORE;
    entry.key = key;
    entry.value = oldValue;
    undoStack.Push(entry);

    cache.Remove(key);

    if (!file.empty()) {
        std::vector<KeyValue> all = index->Range(
            std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
        Persistence::Save(file, all);
    }
    return true;
}

std::vector<KeyValue> Database::Scan(Key lo, Key hi) {
    return index->Range(lo, hi);
}

std::vector<KeyValue> Database::SortedByValue() {
    std::vector<KeyValue> records = index->Range(
        std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
    MergeSort(records, [](const KeyValue& a, const KeyValue& b) {
        if (a.second != b.second) {
            return a.second < b.second;
        }
        return a.first < b.first;
    });
    return records;
}

bool Database::Undo() {
    UndoEntry entry;
    if (!undoStack.Pop(&entry)) {
        return false;
    }
    if (entry.action == UndoAction::RESTORE) {
        index->Put(entry.key, entry.value);
        cache.Put(entry.key, entry.value);
    } else {
        index->Remove(entry.key);
        cache.Remove(entry.key);
    }

    if (!file.empty()) {
        std::vector<KeyValue> all = index->Range(
            std::numeric_limits<Key>::min(), std::numeric_limits<Key>::max());
        Persistence::Save(file, all);
    }
    return true;
}

std::size_t Database::Size() const {
    return index->Size();
}

std::size_t Database::UndoDepth() const {
    return undoStack.Size();
}

std::string Database::CacheStats() const {
    std::ostringstream oss;
    oss << cache.Size() << "/" << cache.Capacity()
        << " (hits: " << cache.Hits()
        << ", misses: " << cache.Misses() << ")";
    return oss.str();
}

const std::string& Database::FileName() const {
    return file;
}