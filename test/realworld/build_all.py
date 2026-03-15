#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
SCRIPT_DIR = ROOT / "test" / "realworld"
WASM_DIR = SCRIPT_DIR / "wasm"


class BuildState:
    def __init__(self) -> None:
        self.pass_count = 0
        self.fail_count = 0
        self.skip_count = 0
        self.errors: list[str] = []

    def log_pass(self, name: str) -> None:
        print(f"  PASS: {name}")
        self.pass_count += 1

    def log_fail(self, name: str, error: str) -> None:
        print(f"  FAIL: {name} - {error}")
        self.fail_count += 1
        self.errors.append(f"  {name}: {error}")

    def log_skip(self, name: str, reason: str) -> None:
        print(f"  SKIP: {name} - {reason}")
        self.skip_count += 1


def up_to_date(src: Path, out: Path, force: bool) -> bool:
    if force or not out.is_file():
        return False
    return src.stat().st_mtime <= out.stat().st_mtime


def run_command(cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> tuple[bool, str]:
    result = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, text=True, check=False)
    if result.returncode == 0:
        return True, ""
    return False, (result.stderr or result.stdout).strip() or f"exit {result.returncode}"


def build_c(state: BuildState, force: bool) -> None:
    print("=== Building C programs (wasi-sdk) ===")
    wasi_sdk = Path(os.environ.get("WASI_SDK_PATH", ""))
    cc = wasi_sdk / "bin" / ("clang.exe" if sys.platform == "win32" else "clang")
    sysroot = wasi_sdk / "share" / "wasi-sysroot"
    sources = sorted((SCRIPT_DIR / "c").glob("*.c"))
    if not cc.is_file():
        state.log_skip("c_*", "WASI_SDK_PATH not set or clang not found")
        return
    for src in sources:
        name = src.stem
        out = WASM_DIR / f"c_{name}.wasm"
        if up_to_date(src, out, force):
            state.log_skip(f"c_{name}", "up to date")
            continue
        ok, err = run_command([str(cc), f"--sysroot={sysroot}", "-O2", "-o", str(out), str(src), "-lm"])
        if ok:
            state.log_pass(f"c_{name}")
        else:
            state.log_fail(f"c_{name}", err)


def build_cpp(state: BuildState, force: bool) -> None:
    print("\n=== Building C++ programs (wasi-sdk) ===")
    wasi_sdk = Path(os.environ.get("WASI_SDK_PATH", ""))
    cxx = wasi_sdk / "bin" / ("clang++.exe" if sys.platform == "win32" else "clang++")
    sysroot = wasi_sdk / "share" / "wasi-sysroot"
    sources = sorted((SCRIPT_DIR / "cpp").glob("*.cpp"))
    if not cxx.is_file():
        state.log_skip("cpp_*", "WASI_SDK_PATH not set or clang++ not found")
        return
    for src in sources:
        name = src.stem
        out = WASM_DIR / f"cpp_{name}.wasm"
        if up_to_date(src, out, force):
            state.log_skip(f"cpp_{name}", "up to date")
            continue
        ok, err = run_command([str(cxx), f"--sysroot={sysroot}", "-O2", "-fno-exceptions", "-o", str(out), str(src)])
        if ok:
            state.log_pass(f"cpp_{name}")
        else:
            state.log_fail(f"cpp_{name}", err)


def build_go(state: BuildState, force: bool) -> None:
    print("\n=== Building Go programs (wasip1/wasm) ===")
    go = shutil.which("go")
    if go is None:
        state.log_skip("go_*", "go not found")
        return
    for directory in sorted((SCRIPT_DIR / "go").glob("*/")):
        name = directory.name
        out = WASM_DIR / f"go_{name}.wasm"
        src = directory / "main.go"
        if up_to_date(src, out, force):
            state.log_skip(f"go_{name}", "up to date")
            continue
        env = os.environ.copy()
        env["GOOS"] = "wasip1"
        env["GOARCH"] = "wasm"
        ok, err = run_command([go, "build", "-o", str(out), "."], cwd=directory, env=env)
        if ok:
            state.log_pass(f"go_{name}")
        else:
            state.log_fail(f"go_{name}", err)


def build_rust(state: BuildState, force: bool) -> None:
    print("\n=== Building Rust programs (wasm32-wasip1) ===")
    cargo = shutil.which("cargo")
    rustup = shutil.which("rustup")
    if cargo is None or rustup is None:
        state.log_skip("rust_*", "cargo or rustup not found")
        return
    installed = subprocess.run([rustup, "target", "list", "--installed"], capture_output=True, text=True, check=False)
    if installed.returncode != 0 or "wasm32-wasip1" not in installed.stdout.split():
        state.log_skip("rust_*", "cargo or wasm32-wasip1 target not found")
        return
    for directory in sorted((SCRIPT_DIR / "rust").glob("*/")):
        cargo_toml = directory / "Cargo.toml"
        if not cargo_toml.is_file():
            continue
        name = directory.name
        out = WASM_DIR / f"rust_{name}.wasm"
        if up_to_date(cargo_toml, out, force):
            state.log_skip(f"rust_{name}", "up to date")
            continue
        ok, err = run_command([cargo, "build", "--manifest-path", str(cargo_toml), "--target", "wasm32-wasip1", "--release", "--quiet"], cwd=SCRIPT_DIR)
        if ok:
            built = directory / "target" / "wasm32-wasip1" / "release" / f"{name}.wasm"
            shutil.copy2(built, out)
            state.log_pass(f"rust_{name}")
        else:
            state.log_fail(f"rust_{name}", err)


def build_tinygo(state: BuildState, force: bool) -> None:
    print("\n=== Building TinyGo programs (wasip1) ===")
    tinygo = shutil.which("tinygo")
    if tinygo is None:
        state.log_skip("tinygo_*", "tinygo not found")
        return
    for directory in sorted((SCRIPT_DIR / "tinygo").glob("*/")):
        name = directory.name
        out = WASM_DIR / f"tinygo_{name}.wasm"
        src = directory / "main.go"
        if up_to_date(src, out, force):
            state.log_skip(f"tinygo_{name}", "up to date")
            continue
        ok, err = run_command([tinygo, "build", "-o", str(out), "-target=wasip1", "-scheduler=none", "."], cwd=directory)
        if ok:
            state.log_pass(f"tinygo_{name}")
        else:
            state.log_fail(f"tinygo_{name}", err)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build all real-world wasm test programs.")
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    WASM_DIR.mkdir(parents=True, exist_ok=True)
    state = BuildState()

    build_c(state, args.force)
    build_cpp(state, args.force)
    build_go(state, args.force)
    build_rust(state, args.force)
    build_tinygo(state, args.force)

    print("\n=== Summary ===")
    print(f"PASS: {state.pass_count}  FAIL: {state.fail_count}  SKIP: {state.skip_count}")
    if state.errors:
        print("Errors:")
        for error in state.errors:
            print(error)
    print(f"\nWasm files in {WASM_DIR}:")
    files = sorted(WASM_DIR.glob("*.wasm"))
    if not files:
        print("  (none)")
    else:
        for wasm in files:
            print(f"  {wasm.name}")
    return state.fail_count


if __name__ == "__main__":
    sys.exit(main())
