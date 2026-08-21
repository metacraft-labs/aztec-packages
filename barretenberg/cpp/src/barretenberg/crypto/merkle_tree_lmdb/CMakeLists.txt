# The LMDB-backed half of crypto_merkle_tree: the persistent tree store and the
# content-addressed trees built on it. Split out so that consumers which only need the tree
# vocabulary (world_state_reference, vm2_sim, aztec) do not link — or compile against — LMDB.
barretenberg_module(
    crypto_merkle_tree_lmdb
    crypto_merkle_tree
    lmdblib
)
# Need -fPIC because it's linked into the nodejs_module shared library
set_target_properties(crypto_merkle_tree_lmdb_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (NOT FUZZING)
    # but the tests use poseidon
    target_link_libraries(crypto_merkle_tree_lmdb_tests PRIVATE stdlib_poseidon2)
endif()
