// === AUDIT STATUS ===
// internal:    { status: Complete, auditors: [Nishat], commit: 22d6fc368da0fbe5412f4f7b2890a052aa48d803 }
// external_1:  { status: not started, auditors: [], commit: }
// external_2:  { status: not started, auditors: [], commit: }
// =====================

#pragma once
#include "barretenberg/common/log.hpp"
#include "barretenberg/common/serialize.hpp"
#include "barretenberg/crypto/merkle_tree/indexed_tree/indexed_leaf.hpp"
#include "barretenberg/crypto/merkle_tree/node_store/tree_meta.hpp"
#include "barretenberg/crypto/merkle_tree/node_store/tree_store_payloads.hpp"
#include "barretenberg/crypto/merkle_tree/types.hpp"
#include "barretenberg/ecc/curves/bn254/fr.hpp"
#include "barretenberg/lmdblib/lmdb_database.hpp"
#include "barretenberg/lmdblib/lmdb_environment.hpp"
#include "barretenberg/lmdblib/lmdb_read_transaction.hpp"
#include "barretenberg/lmdblib/lmdb_store_base.hpp"
#include "barretenberg/lmdblib/lmdb_write_transaction.hpp"
#include "barretenberg/serialize/msgpack_impl.hpp"
#include "barretenberg/world_state/types.hpp"
#include "lmdb.h"
#include <cstdint>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace bb::crypto::merkle_tree {

using namespace bb::lmdblib;

/**
 * Creates an abstraction against a collection of LMDB databases within a single environment used to store merkle tree
 * data
 */

class LMDBTreeStore : public LMDBStoreBase {
  public:
    using Ptr = std::unique_ptr<LMDBTreeStore>;
    using SharedPtr = std::shared_ptr<LMDBTreeStore>;
    using ReadTransaction = LMDBReadTransaction;
    using WriteTransaction = LMDBWriteTransaction;
    LMDBTreeStore(
        std::string directory, std::string name, uint64_t mapSizeKb, uint64_t maxNumReaders, bool ephemeral = false);
    LMDBTreeStore(const LMDBTreeStore& other) = delete;
    LMDBTreeStore(LMDBTreeStore&& other) = delete;
    LMDBTreeStore& operator=(const LMDBTreeStore& other) = delete;
    LMDBTreeStore& operator=(LMDBTreeStore&& other) = delete;
    ~LMDBTreeStore() override = default;

    const std::string& get_name() const;

    void get_stats(TreeDBStats& stats, ReadTransaction& tx);

    void write_block_data(const block_number_t& blockNumber, const BlockPayload& blockData, WriteTransaction& tx);

    bool read_block_data(const block_number_t& blockNumber, BlockPayload& blockData, ReadTransaction& tx);

    void delete_block_data(const block_number_t& blockNumber, WriteTransaction& tx);

    void write_block_index_data(const block_number_t& blockNumber, const index_t& sizeAtBlock, WriteTransaction& tx);

    // index here is 0 based
    bool find_block_for_index(const index_t& index, block_number_t& blockNumber, ReadTransaction& tx);

    void delete_block_index(const index_t& sizeAtBlock, const block_number_t& blockNumber, WriteTransaction& tx);

    void write_meta_data(const TreeMeta& metaData, WriteTransaction& tx);

    bool read_meta_data(TreeMeta& metaData, ReadTransaction& tx);

    template <typename TxType> bool read_leaf_index(const fr& leafValue, index_t& leafIndex, TxType& tx);

    fr find_low_leaf(const fr& leafValue, index_t& index, const std::optional<index_t>& sizeLimit, ReadTransaction& tx);

    void write_leaf_index(const fr& leafValue, const index_t& leafIndex, WriteTransaction& tx);

    void delete_leaf_index(const fr& leafValue, WriteTransaction& tx);

    bool read_node(const fr& nodeHash, NodePayload& nodeData, ReadTransaction& tx);

    void write_node(const fr& nodeHash, const NodePayload& nodeData, WriteTransaction& tx);

    void increment_node_reference_count(const fr& nodeHash, WriteTransaction& tx);

    void set_or_increment_node_reference_count(const fr& nodeHash, NodePayload& nodeData, WriteTransaction& tx);

    void decrement_node_reference_count(const fr& nodeHash, NodePayload& nodeData, WriteTransaction& tx);

    template <typename LeafType, typename TxType>
    bool read_leaf_by_hash(const fr& leafHash, LeafType& leafData, TxType& tx);

    template <typename LeafType>
    void write_leaf_by_hash(const fr& leafHash, const LeafType& leafData, WriteTransaction& tx);

    void delete_leaf_by_hash(const fr& leafHash, WriteTransaction& tx);

  private:
    std::string _name;
    LMDBDatabase::Ptr _blockDatabase;
    LMDBDatabase::Ptr _nodeDatabase;
    LMDBDatabase::Ptr _leafKeyToIndexDatabase;
    LMDBDatabase::Ptr _leafHashToPreImageDatabase;
    LMDBDatabase::Ptr _indexToBlockDatabase;

    template <typename TxType> bool get_node_data(const fr& nodeHash, NodePayload& nodeData, TxType& tx);
};

template <typename TxType> bool LMDBTreeStore::read_leaf_index(const fr& leafValue, index_t& leafIndex, TxType& tx)
{
    FrKeyType key(leafValue);
    return tx.template get_value<FrKeyType>(key, leafIndex, *_leafKeyToIndexDatabase);
}

template <typename LeafType, typename TxType>
bool LMDBTreeStore::read_leaf_by_hash(const fr& leafHash, LeafType& leafData, TxType& tx)
{
    FrKeyType key(leafHash);
    std::vector<uint8_t> data;
    bool success = tx.template get_value<FrKeyType>(key, data, *_leafHashToPreImageDatabase);
    if (success) {
        msgpack::unpack((const char*)data.data(), data.size()).get().convert(leafData);
    }
    return success;
}

template <typename LeafType>
void LMDBTreeStore::write_leaf_by_hash(const fr& leafHash, const LeafType& leafData, WriteTransaction& tx)
{
    msgpack::sbuffer buffer;
    msgpack::pack(buffer, leafData);
    std::vector<uint8_t> encoded(buffer.data(), buffer.data() + buffer.size());
    FrKeyType key(leafHash);
    tx.put_value<FrKeyType>(key, encoded, *_leafHashToPreImageDatabase);
}

template <typename TxType> bool LMDBTreeStore::get_node_data(const fr& nodeHash, NodePayload& nodeData, TxType& tx)
{
    FrKeyType key(nodeHash);
    std::vector<uint8_t> data;
    bool success = tx.template get_value<FrKeyType>(key, data, *_nodeDatabase);
    if (success) {
        msgpack::unpack((const char*)data.data(), data.size()).get().convert(nodeData);
    }
    return success;
}
} // namespace bb::crypto::merkle_tree
