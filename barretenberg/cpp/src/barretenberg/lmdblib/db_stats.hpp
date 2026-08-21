#pragma once

// Reported statistics for a single database. Deliberately free of <lmdb.h>: DBStats is
// embedded in structs that are reported all the way up to the world-state and AVM layers,
// and none of those need the LMDB API to describe how many items a database holds.
//
// The MDB_stat-based construction lives in lmdblib/types.hpp, next to the code that has an
// MDB_stat to offer.

#include "barretenberg/serialize/msgpack.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

namespace bb::lmdblib {

struct DBStats {
    std::string name;
    uint64_t numDataItems;
    uint64_t totalUsedSize;

    DBStats() = default;
    DBStats(const DBStats& other) = default;
    DBStats(DBStats&& other) noexcept { *this = std::move(other); }
    ~DBStats() = default;
    DBStats(std::string name, uint64_t numDataItems, uint64_t totalUsedSize)
        : name(std::move(name))
        , numDataItems(numDataItems)
        , totalUsedSize(totalUsedSize)
    {}

    SERIALIZATION_FIELDS(name, numDataItems, totalUsedSize)

    bool operator==(const DBStats& other) const
    {
        return name == other.name && numDataItems == other.numDataItems && totalUsedSize == other.totalUsedSize;
    }

    DBStats& operator=(const DBStats& other) = default;

    DBStats& operator=(DBStats&& other) noexcept
    {
        if (this != &other) {
            name = std::move(other.name);
            numDataItems = other.numDataItems;
            totalUsedSize = other.totalUsedSize;
        }
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const DBStats& stats)
    {
        os << "DB " << stats.name << ", num items: " << stats.numDataItems
           << ", total used size: " << stats.totalUsedSize;
        return os;
    }
};

} // namespace bb::lmdblib
