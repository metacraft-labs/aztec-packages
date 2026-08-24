{
  description = "aztec-packages (metacraft-labs fork) — dev shell for the AVM_WASM barretenberg build";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      perSystem =
        { pkgs, ... }:
        let
          wasi-sdk = pkgs.callPackage ./nix/wasi-sdk.nix { };

          # barretenberg's CMake configure step shells out to
          # scripts/remake-constants.sh, which invokes `clang-format-20` by that
          # exact versioned name and aborts the configure if it is missing.
          # nixpkgs installs it unversioned, so provide the alias.
          clang-format-20 = pkgs.writeShellScriptBin "clang-format-20" ''
            exec ${pkgs.llvmPackages_20.clang-tools}/bin/clang-format "$@"
          '';
        in
        {
          packages.wasi-sdk = wasi-sdk;

          # The version barretenberg currently pins. Not in any dev shell and not
          # used by any build here — it is the negative control the M4 checks
          # execute against (it cannot link C++ exceptions) and the toolchain the
          # "before" half of the barretenberg.wasm comparison is built with.
          packages.wasi-sdk-27 = pkgs.callPackage ./nix/wasi-sdk.nix { version = "27.0"; };

          devShells.default = pkgs.mkShell {
            packages = [
              # barretenberg C++ — both the native build (which backs the
              # differential oracle) and the wasm one.
              pkgs.cmake
              pkgs.ninja
              pkgs.clang_20
              pkgs.llvmPackages_20.clang-tools
              clang-format-20
              pkgs.ccache
              pkgs.pkg-config

              # wasi-sdk 33, NOT the 27 that cpp/bootstrap.sh downloads: 27
              # cannot compile C++ exceptions, and the AVM signals reverts by
              # throwing. See nix/wasi-sdk.nix.
              wasi-sdk
              pkgs.wasmtime
              pkgs.binaryen
              pkgs.wabt

              # yarn-project. .nvmrc pins v24, packageManager pins yarn 4.x.
              pkgs.nodejs_24
              pkgs.yarn-berry_4

              # What bootstrap.sh / ci3 reach for.
              pkgs.git
              pkgs.jq
              pkgs.curl
              pkgs.python3
              pkgs.parallel
              pkgs.gnumake
              pkgs.gnused
              pkgs.gawk
            ]
            # The M4 checks build barretenberg.wasm with wasi-sdk 27 as well as 33,
            # and the UNPATCHED `wasm` preset hardcodes `/opt/wasi-sdk` in its
            # environment block — that is one of the things the patch fixes. bwrap
            # binds the chosen SDK there so both halves of the comparison run the
            # preset verbatim, with the toolchain bytes as the only difference.
            ++ pkgs.lib.optionals pkgs.stdenv.hostPlatform.isLinux [ pkgs.bubblewrap ];

            WASI_SDK_PATH = "${wasi-sdk}";
            WASI_SDK_PREFIX = "${wasi-sdk}";

            # nixpkgs is on cmake 4.x, which hard-errors on any subproject
            # declaring cmake_minimum_required below 3.5. barretenberg itself
            # asks for 3.24, but several of its FetchContent'd dependencies are
            # older. This keeps them configurable without patching them.
            CMAKE_POLICY_VERSION_MINIMUM = "3.5";

            shellHook = ''
              # ---- compiler cache -------------------------------------------------
              # ccache was in the package list from the start and NOTHING invoked it:
              # barretenberg's CMake has no ccache integration of its own (no
              # COMPILER_LAUNCHER, no cmake/ccache.cmake), and only one verification lib
              # passed the launcher on the command line. A cache on PATH but not on the
              # compiler launcher path looks solved and caches nothing, which is worse
              # than not having one. These six exports are what put it on that path.
              #
              # CMAKE_<LANG>_COMPILER_LAUNCHER is read by CMake as the default for the
              # cache variable of the same name, so it reaches every configure site —
              # `cmake --preset wasm` included — without editing a single build script,
              # and it applies to the wasi-sdk toolchain exactly as it does to the
              # native one. It does NOT appear in compile_commands.json (CMake keeps the
              # launcher out of the compile database), so the checks that read the
              # compile database see the same bare compiler command as before.
              #
              # CCACHE_BASEDIR rewrites absolute paths under it to paths relative to the
              # compile's working directory BEFORE hashing, which is what lets the same
              # upstream translation unit built in ~/.cache/aztec-m9-observer and in
              # ~/.cache/aztec-m13-final share one cache entry. It cannot make two
              # different trees look alike: the hash is over the source CONTENT, so a
              # tree that differs misses and gets its own object.
              #
              # CCACHE_COMPILERCHECK is deliberately NOT the `mtime` default. Every
              # binary in /nix/store has mtime 1970, so `mtime` discriminates two
              # toolchains by SIZE alone -- and M4's whole point is that a wasi-sdk
              # masquerading as another version is exactly the mutation a check reading
              # the cheap identifier misses. `%compiler% --version` measured at 10.8 ms
              # per cache hit against 2.3 ms for `mtime` (200 hits, three interleaved
              # rounds): about 8.5 s over a thousand-translation-unit build.
              #
              # sloppiness is left EMPTY on purpose. Every relaxation there is a licence
              # to return an object for a compile that was not quite the same one, and
              # this tree's neutrality evidence is built on base-versus-patched builds.
              export CCACHE_DIR="''${CCACHE_DIR:-$HOME/.cache/ccache}"
              export CCACHE_BASEDIR="''${CCACHE_BASEDIR:-$HOME}"
              export CCACHE_MAXSIZE="''${CCACHE_MAXSIZE:-60G}"
              export CCACHE_COMPILERCHECK="''${CCACHE_COMPILERCHECK:-%compiler% --version}"
              export CMAKE_C_COMPILER_LAUNCHER="''${CMAKE_C_COMPILER_LAUNCHER:-ccache}"
              export CMAKE_CXX_COMPILER_LAUNCHER="''${CMAKE_CXX_COMPILER_LAUNCHER:-ccache}"

              echo "aztec-packages (metacraft-labs): node $(node --version), wasi-sdk $(head -1 ${wasi-sdk}/VERSION), $(cmake --version | head -1), ccache $(ccache --version | head -1 | awk '{print $3}') at $CCACHE_DIR"
            '';
          };
        };
    };
}
