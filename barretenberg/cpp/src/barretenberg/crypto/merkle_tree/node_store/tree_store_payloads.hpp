// === AUDIT STATUS ===
// internal:    { status: Complete, auditors: [Nishat], commit: 22d6fc368da0fbe5412f4f7b2890a052aa48d803 }
// external_1:  { status: not started, auditors: [], commit: }
// external_2:  { status: not started, auditors: [], commit: }
// =====================

#pragma once

// The records a content-addressed tree store persists. These are plain serialisable
// structs: they describe *what* is stored, not *where*. They were previously declared in
// lmdb_store/lmdb_tree_store.hpp, which forced every consumer of the tree vocabulary to
// compile against lmdb.h even though none of them touch a database.

#include "barretenberg/common/log.hpp"
#include "barretenberg/crypto/merkle_tree/types.hpp"
#include "barretenberg/ecc/curves/bn254/fr.hpp"
#include "barretenberg/serialize/msgpack.hpp"
#include <optional>
#include <ostream>
#include <stdexcept>
#include <vector>

namespace bb::crypto::merkle_tree {

struct BlockPayload {

    index_t size;
    block_number_t blockNumber;
    fr root;

    SERIALIZATION_FIELDS(size, blockNumber, root)

    bool operator==(const BlockPayload& other) const
    {
        return size == other.size && blockNumber == other.blockNumber && root == other.root;
    }
};

inline std::ostream& operator<<(std::ostream& os, const BlockPayload& block)
{
    os << "BlockPayload{size: " << std::dec << block.size << ", blockNumber: " << std::dec << block.blockNumber
       << ", root: " << block.root << "}";
    return os;
}

struct NodePayload {
    std::optional<fr> left;
    std::optional<fr> right;
    uint64_t ref;

    SERIALIZATION_FIELDS(left, right, ref)

    bool operator==(const NodePayload& other) const
    {
        return left == other.left && right == other.right && ref == other.ref;
    }
};

struct BlockIndexPayload {
    std::vector<block_number_t> blockNumbers;

    SERIALIZATION_FIELDS(blockNumbers)

    bool operator==(const BlockIndexPayload& other) const { return blockNumbers == other.blockNumbers; }

    bool is_empty() const { return blockNumbers.empty(); }

    block_number_t get_min_block_number() { return blockNumbers[0]; }

    void delete_block(const block_number_t& blockNumber)
    {
        // Shouldn't be possible, but no need to do anything here
        if (blockNumbers.empty()) {
            return;
        }

        // If the size is 1, the blocknumber must match that in index 0, if it does remove it
        if (blockNumbers.size() == 1) {
            if (blockNumbers[0] == blockNumber) {
                blockNumbers.pop_back();
            }
            return;
        }

        // we have 2 entries, we must verify that the block number is equal to the item in index 1
        if (blockNumbers[1] != blockNumber) {
            throw std::runtime_error(format("Unable to delete block number ",
                                            blockNumber,
                                            " for retrieval by index, current max block number at that index: ",
                                            blockNumbers[1]));
        }
        // It is equal, decrement it, we know that the new block number must have been added previously
        --blockNumbers[1];

        // We have modified the high block. If it is now equal to the low block then pop it
        if (blockNumbers[0] == blockNumbers[1]) {
            blockNumbers.pop_back();
        }
    }

    void add_block(const block_number_t& blockNumber)
    {
        // If empty, just add the block number
        if (blockNumbers.empty()) {
            blockNumbers.emplace_back(blockNumber);
            return;
        }

        // If the size is 1, then we must be adding the block 1 larger than that in index 0
        if (blockNumbers.size() == 1) {
            if (blockNumber != blockNumbers[0] + 1) {
                // We can't accept a block number for this index that does not immediately follow the block before
                throw std::runtime_error(format("Unable to store block number ",
                                                blockNumber,
                                                " for retrieval by index, current max block number at that index: ",
                                                blockNumbers[0]));
            }
            blockNumbers.emplace_back(blockNumber);
            return;
        }

        // Size must be 2 here, if larger, this is an error
        if (blockNumbers.size() != 2) {
            throw std::runtime_error(format("Unable to store block number ",
                                            blockNumber,
                                            " for retrieval by index, block numbers is of invalid size: ",
                                            blockNumbers.size()));
        }

        // If the size is 2, then we must be adding the block 1 larger than that in index 1
        if (blockNumber != blockNumbers[1] + 1) {
            // We can't accept a block number for this index that does not immediately follow the block before
            throw std::runtime_error(format("Unable to store block number ",
                                            blockNumber,
                                            " for retrieval by index, current max block number at that index: ",
                                            blockNumbers[1]));
        }
        blockNumbers[1] = blockNumber;
    }
};

} // namespace bb::crypto::merkle_tree
