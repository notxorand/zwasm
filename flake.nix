{
  description = "zwasm - Zig WebAssembly runtime (library + CLI)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };

        # Zig 0.15.2 binary (per-architecture URLs and hashes)
        zigArchInfo = {
          "aarch64-darwin" = {
            url = "https://ziglang.org/download/0.15.2/zig-aarch64-macos-0.15.2.tar.xz";
            sha256 = "1csy5ch8aym67w06ffmlwamrzkfq8zwv4kcl6bcpc5vn1cbhd31g";
          };
          "x86_64-darwin" = {
            url = "https://ziglang.org/download/0.15.2/zig-x86_64-macos-0.15.2.tar.xz";
            sha256 = ""; # untested
          };
          "x86_64-linux" = {
            url = "https://ziglang.org/download/0.15.2/zig-x86_64-linux-0.15.2.tar.xz";
            sha256 = "0skmy2qjg2z4bsxnkdzqp1hjzwwgnvqhw4qjfnsdpv6qm23p4wm0";
          };
          "aarch64-linux" = {
            url = "https://ziglang.org/download/0.15.2/zig-aarch64-linux-0.15.2.tar.xz";
            sha256 = ""; # untested
          };
        }.${system} or (throw "Unsupported system: ${system}");

        zigSrc = builtins.fetchTarball {
          url = zigArchInfo.url;
          sha256 = zigArchInfo.sha256;
        };

        zigBin = pkgs.runCommand "zig-0.15.2-wrapper" {} ''
          mkdir -p $out/bin
          ln -s ${zigSrc}/zig $out/bin/zig
          ln -s ${zigSrc}/lib $out/lib
        '';

        # wasi-sdk 30 binary (for C/C++ → wasm32-wasi compilation)
        wasiSdkArchInfo = {
          "aarch64-darwin" = {
            url = "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-30/wasi-sdk-30.0-arm64-macos.tar.gz";
            sha256 = "0f2zqwxzdf6fjzjjcycvrk1mjg2w29lk19lpjc7sddnxwgdrzf5l";
          };
          "x86_64-linux" = {
            url = "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-30/wasi-sdk-30.0-x86_64-linux.tar.gz";
            sha256 = "145cf587396n01zgf43hzdpdmivh3sr4fx9sfs8g5p0fw45clys1";
          };
          "x86_64-darwin" = {
            url = "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-30/wasi-sdk-30.0-x86_64-macos.tar.gz";
            sha256 = ""; # untested
          };
          "aarch64-linux" = {
            url = "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-30/wasi-sdk-30.0-arm64-linux.tar.gz";
            sha256 = ""; # untested
          };
        }.${system} or null;

        wasiSdkSrc = if wasiSdkArchInfo != null then
          builtins.fetchTarball {
            url = wasiSdkArchInfo.url;
            sha256 = wasiSdkArchInfo.sha256;
          }
        else null;

        wasiSdkBin = if wasiSdkSrc != null then
          pkgs.runCommand "wasi-sdk-30-wrapper" {} ''
            mkdir -p $out/bin $out/share
            ln -s ${wasiSdkSrc}/bin/* $out/bin/
            ln -s ${wasiSdkSrc}/share/wasi-sysroot $out/share/wasi-sysroot
            ln -s ${wasiSdkSrc}/lib $out/lib
          ''
        else null;

      in {
        devShells.default = pkgs.mkShell {
          name = "zwasm";

          buildInputs = with pkgs; [
            # Compiler
            zigBin

            # Wasm runtimes (benchmark comparison targets)
            wasmtime

            # JS/Wasm runtimes
            bun
            nodejs

            # Data processing
            yq-go
            jq

            # Benchmarking
            hyperfine

            # Wasm build tools
            tinygo
            wasm-tools  # json-from-wast (spec test conversion), component inspection

            # Real-world wasm compilation toolchains
            go          # GOOS=wasip1 GOARCH=wasm (Go 1.21+)
            # Rust: use system rustup (rustup target add wasm32-wasip1)
            # wasi-sdk: provided via custom fetch below

            # Utilities
            gnused
            coreutils
            python3
          ] ++ pkgs.lib.optionals (wasiSdkBin != null) [ wasiSdkBin ];

          shellHook = ''
            ${if wasiSdkSrc != null then ''
              export WASI_SDK_PATH="${wasiSdkSrc}"
            '' else ""}
          '';
        };
      }
    );
}
