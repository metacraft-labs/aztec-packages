#pragma once

#include "barretenberg/lmdblib/db_stats.hpp"
#include "barretenberg/serialize/msgpack.hpp"
#include "lmdb.h"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>
namespace bb::lmdblib {
using Key = std::vector<uint8_t>;
using Value = std::vector<uint8_t>;
using KeysVector = std::vector<Key>;
using ValuesVector = std::vector<Value>;
using KeyValuesPair = std::pair<Key, ValuesVector>;
using OptionalValues = std::optional<ValuesVector>;
using OptionalValuesVector = std::vector<OptionalValues>;
using KeyDupValuesVector = std::vector<KeyValuesPair>;
using KeyOptionalValuesPair = std::pair<Key, OptionalValues>;
using KeyOptionalValuesVector = std::vector<KeyOptionalValuesPair>;

// DBStats itself is declared in db_stats.hpp, free of <lmdb.h>, so that consumers which only
// report statistics do not have to compile against the LMDB API. This is the one construction
// that genuinely needs it.
inline DBStats make_db_stats(std::string name, const MDB_stat& stat)
{
    return { std::move(name),
             stat.ms_entries,
             stat.ms_psize * (stat.ms_branch_pages + stat.ms_leaf_pages + stat.ms_overflow_pages) };
}

} // namespace bb::lmdblib
