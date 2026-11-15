#pragma once
// ============================================================================
// persistence.h  -  save / load all records to a file
//
// Why: RAM is erased when the program closes. A database must keep its data,
// so on exit we SAVE every record to a file and on start we LOAD them back.
//
// File format (plain text + exact value bytes):
//     MINIDB 1              <- header: name + format version
//     3                     <- number of records
//     5 3 Ali               <- <key> <value length> <value bytes>
//     -2 11 hello world     <- length first, so spaces/newlines in values are safe
//     7 0                   <- empty value (length 0)
// ============================================================================
#include <string>
#include <vector>

#include "common.h"  // Key, KeyValue

class Persistence {
public:
    // Writes all records to "<file>.tmp", then renames it over <file>.
    // (So a crash in the middle never destroys the old file.)
    // Returns false if anything failed.
    static bool Save(const std::string& file, const std::vector<KeyValue>& records);

    // Reads records from <file> into *out.
    //   file missing  -> returns true, *out empty (first run, not an error)
    //   file corrupt  -> prints an error, returns false, *out empty
    static bool Load(const std::string& file, std::vector<KeyValue>* out);
};