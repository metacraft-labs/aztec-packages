#!/usr/bin/env bash

set -e

# run commands relative to parent directory
cd $(dirname $0)/..

# ContentAddressedCacheTest lives in crypto_merkle_tree; everything that needs a persisted
# (LMDB) store lives in crypto_merkle_tree_lmdb.
DEFAULT_TESTS=PersistedIndexedTreeTest.*:PersistedAppendOnlyTreeTest.*:LMDBTreeStoreTest.*:PersistedContentAddressedIndexedTreeTest.*:PersistedContentAddressedAppendOnlyTreeTest.*:ContentAddressedCacheTest.*
TEST=${1:-$DEFAULT_TESTS}
PRESET=${PRESET:-clang20}

cmake --build --preset $PRESET --target crypto_merkle_tree_tests crypto_merkle_tree_lmdb_tests
./build/bin/crypto_merkle_tree_tests --gtest_filter=$TEST
./build/bin/crypto_merkle_tree_lmdb_tests --gtest_filter=$TEST
