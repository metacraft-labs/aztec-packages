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
              echo "aztec-packages (metacraft-labs): node $(node --version), wasi-sdk $(head -1 ${wasi-sdk}/VERSION), $(cmake --version | head -1)"
            '';
          };
        };
    };
}
