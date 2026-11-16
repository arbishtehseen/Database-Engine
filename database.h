#ifndef MINIDB_DATABASE_H
#define MINIDB_DATABASE_H

#include "index.h"
#include "lru_cache.h"
#include "stack.h"
#include <memory>
#include <string>
#include <vector>

class Database {
private:
    enum class UndoAction { REMOVE, RESTORE };

    struct UndoEntry {
        UndoAction action = UndoAction::REMOVE;
        Key key = 0;
        std::string value;
    };

    std::string file;
    std::unique_ptr<Index> index;
    Stack<UndoEntry> undoStack;
    LRUCache cache;

public:
    // Default cache capacity 64 rakhi hai (aap isay change bhi kar sakti hain)
    Database(const std::string& fileName, std::unique_ptr<Index> indexToUse,
             std::size_t cacheCapacity = 64);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool Put(Key key, const std::string& value);
    bool Get(Key key, std::string* out);
    bool Delete(Key key);

    std::vector<KeyValue> Scan(Key lo, Key hi);
    std::vector<KeyValue> SortedByValue();

    bool Undo();

    std::size_t Size() const;
    std::size_t UndoDepth() const;
    std::string CacheStats() const;
    const std::string& FileName() const;
};

#endif