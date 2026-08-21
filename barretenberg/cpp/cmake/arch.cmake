if(WASM)
    # Disable SLP vectorization on WASM as it's brokenly slow. To give an idea, with this off it still takes
    # 2m:18s to compile scalar_multiplication.cpp, and with it on I estimate it's 50-100 times longer. I never
    # had the patience to wait it out...
    add_compile_options(-fno-slp-vectorize)
    if(AVM_WASM)
        # The AVM signals a reverted call by throwing, and tx_execution catches it. Under
        # -fno-exceptions and the BB_NO_EXCEPTIONS shim (common/try_catch_shim.hpp) a revert
        # becomes std::abort() on the whole module, so this build needs real exceptions.
        # wasi-sdk 33 supports them; earlier versions do not.
        set(BB_WASM_EH_OPTIONS -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false)
        add_compile_options(${BB_WASM_EH_OPTIONS})
        add_link_options(${BB_WASM_EH_OPTIONS} -lunwind)

        # Gate the toolchain here rather than at link time. wasi-sdk built its libc++abi
        # with -fno-exceptions and shipped no unwinder until 33, and such a sysroot
        # accepts -fwasm-exceptions on the compile line without complaint -- the failure
        # only appears when the first target is linked, and then once per target. Probing
        # once, at configure time, turns that into a sentence that says what to install.
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_FLAGS "-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false")
        set(CMAKE_REQUIRED_LINK_OPTIONS ${BB_WASM_EH_OPTIONS} -lunwind)
        check_cxx_source_compiles("
            struct Revert { int code; };
            __attribute__((noinline)) void raise(int code) { throw Revert{ code }; }
            int main() {
                try { raise(7); } catch (const Revert& r) { return r.code == 7 ? 0 : 1; }
                return 2;
            }" BB_WASM_EXCEPTIONS_SUPPORTED)
        unset(CMAKE_REQUIRED_FLAGS)
        unset(CMAKE_REQUIRED_LINK_OPTIONS)
        if(NOT BB_WASM_EXCEPTIONS_SUPPORTED)
            message(FATAL_ERROR
                "AVM_WASM is ON, but this wasm toolchain cannot compile and link C++ "
                "exceptions: a throw/catch across a noinline boundary does not link.\n"
                "  compiler: ${CMAKE_CXX_COMPILER}\n"
                "  sysroot:  ${CMAKE_SYSROOT}\n"
                "AVM_WASM needs wasi-sdk 33.0 or newer, whose sysroot ships an unwinder "
                "and a libc++abi built with exceptions enabled. Older ones fail this "
                "probe either as \"unable to find library -lunwind\" or as \"undefined "
                "symbol: __cxa_throw\"; the exact line is in "
                "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeConfigureLog.yaml under "
                "BB_WASM_EXCEPTIONS_SUPPORTED.\n"
                "Point WASI_SDK_PREFIX at wasi-sdk 33.0 or newer, or leave AVM_WASM OFF "
                "-- every other wasm configuration is built -fno-exceptions and is "
                "unaffected.")
        endif()
        message(STATUS "AVM_WASM: wasm C++ exceptions probe passed (${CMAKE_CXX_COMPILER})")
    else()
        add_compile_options(-fno-exceptions)
    endif()
endif()

# Target skylake on x86 for AVX2 etc. ARM is handled by the zig wrapper scripts
# which use explicit aarch64 targets to produce generic ARM64 code without
# CPU-specific extensions (e.g. SVE on Graviton) that would break on Apple Silicon.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    add_compile_options(-march=skylake)
endif()
