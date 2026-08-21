# Merkle tree vocabulary and in-memory trees. Header-only, hash-function agnostic, and — as of
# the lmdb_store split — free of any storage backend. The LMDB-backed content-addressed stores
# live in ../merkle_tree_lmdb.
barretenberg_module(
    crypto_merkle_tree
    ecc
)

if (NOT FUZZING)
    # but the tests use poseidon
    target_link_libraries(crypto_merkle_tree_tests PRIVATE stdlib_poseidon2)
endif()
