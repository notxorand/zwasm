# Zig 0.16.0 Tips & Pitfalls

Common mistakes and workarounds discovered during development. Most entries
below carry over from 0.15.2 unchanged — 0.16.0's big shift is **"I/O as an
Interface"** (filesystem and I/O routines now take an explicit `io: Io`
argument); see the dedicated section below.

## Filesystem: std.fs is deprecated — use std.Io.Dir with an `io` argument

0.16.0 deprecates the entire `std.fs` module. `std.fs.cwd()`, `openFile`,
`readFileAlloc`, etc. now live on `std.Io.Dir`, and every method that performs
real I/O takes a second positional parameter of type `std.Io` (the interface
vtable). `std.fs.*` stubs still exist but forward to `std.Io.Dir` — prefer the
new path in new code.

```zig
// 0.15.2
const file = try std.fs.cwd().openFile(path, .{});

// 0.16.0
const file = try std.Io.Dir.cwd().openFile(io, path, .{});
```

Acquiring an `io` instance:

```zig
var threaded = std.Io.Threaded.init(allocator);
defer threaded.deinit();
const io = threaded.io();
```

On Linux you can swap `std.Io.Threaded` for `std.Io.Uring`; on macOS/BSD,
`std.Io.Kqueue`. `Threaded` is the portable default and what this project
uses in `wasi.zig`.

> **Common mistake**: writing `std.Io.Dir.cwd().openFile(path, .{})` without
> the `io` argument. The compile error is misleading ("expected 3 arguments,
> found 2") and doesn't name `Io` — if you see it, check the method signature
> in `lib/zig/std/Io/Dir.zig`.

## tagged union comparison: use switch, not ==

## tagged union comparison: use switch, not ==

```zig
// OK
return switch (self) { .nil => true, else => false };
// NG — unreliable for tagged unions
return self == .nil;
```

## ArrayList / HashMap init: use .empty

```zig
var list: std.ArrayList(u8) = .empty;  // not .init(allocator)
defer list.deinit(allocator);
try list.append(allocator, 42);        // allocator passed per call
```

## stdout: buffered writer required

```zig
var buf: [4096]u8 = undefined;
var writer = std.fs.File.stdout().writer(&buf);
const stdout = &writer.interface;
// ... write ...
try stdout.flush();  // don't forget
```

## Use std.Io.Writer (type-erased) instead of anytype for writers

`std.Io.Writer` is the type-erased writer (landed in 0.15, finalized in 0.16).
`GenericWriter` and `fixedBufferStream` are deprecated.

Prefer `*std.Io.Writer` over `anytype` for writer parameters.
This avoids the "unable to resolve inferred error set" problem
with recursive functions, and the error type is a concrete
`error{WriteFailed}` instead of `anyerror`.

```zig
const Writer = std.Io.Writer;

// OK — concrete type, works with recursion, precise error set
pub fn formatPrStr(self: Form, w: *Writer) Writer.Error!void {
    // recursive calls work fine
    try inner.formatPrStr(w);
}

// In tests: use Writer.fixed + w.buffered()
var buf: [256]u8 = undefined;
var w: Writer = .fixed(&buf);
try form.formatPrStr(&w);
try std.testing.expectEqualStrings("expected", w.buffered());
```

Ref: std lib uses `*Writer` throughout (json, fmt, etc.)
Old `anytype` + `anyerror` pattern is no longer needed.

## @branchHint, not @branch

```zig
// OK — hint goes INSIDE the branch body
if (likely_condition) {
    @branchHint(.likely);
    // hot path
} else {
    @branchHint(.unlikely);
    return error.Fail;
}

// NG — @branch(.likely, cond) does not exist
```

## Custom format method: use {f}, not {}

Types with a `format` method cause "ambiguous format string"
compile error when printed with `{}`. Use `{f}` or `{any}`.

```zig
// NG — compile error: ambiguous format string
try w.print("{}", .{my_value});

// OK — explicitly calls format method
try w.print("{f}", .{my_value});

// OK — skips format method, uses default
try w.print("{any}", .{my_value});
```

## 0.16.0 migration gotchas (lessons from v1.10.0)

These bit us during the Zig 0.15.2 → 0.16.0 migration. Each one cost hours.

### `std.posix.timespec` is `void` on Windows

```zig
// Compiles on Mac/Linux, fails on Windows with
//   error: expected type 'void', found 'comptime_int'
const req: std.posix.timespec = .{ .sec = 0, .nsec = ns };
```

Branch on `builtin.os.tag` at comptime and use `kernel32.Sleep(ms)` on
Windows. The POSIX path must be inside the `else` branch — putting it
at top level still tries to semantically analyze the struct literal on
Windows.

### `windows.FALSE` → `windows.BOOL.FALSE`

0.16 made `windows.BOOL` a proper enum. Bare `windows.FALSE` / `windows.TRUE`
no longer exist — use `windows.BOOL.FALSE` / `.TRUE`.

### `std.c.fd_t == windows.HANDLE` breaks `std.c.write` on Windows

On Windows with `link_libc = true`:

```zig
pub const fd_t = switch (native_os) {
    .windows => windows.HANDLE,   // *anyopaque (8 bytes)
    // ...
};
pub extern "c" fn write(fd: fd_t, ...) isize;
```

MSVCRT's `_write` actually takes `int fd` (4 bytes). Calling
`std.c.write(handle, ...)` passes a pointer where an int is expected and
lands on a meaningless fd. All `std.c.{read,write,pread,pwrite,lseek}`
calls are broken on Windows — use `WriteFile`/`ReadFile`/`SetFilePointerEx`
directly, or migrate to `std.Io.File` methods.

### `std.c.Stat` / `std.c.fstat` are empty on Linux

The libc Stat binding is intentionally empty on Linux in 0.16. Use the
statx syscall directly (`std.posix.system.statx`) or `lseek(SEEK_END)` for
just-the-size. The empty-struct compile error is the first hint.

### `@Vector` rejected as runtime index target

0.16 tightened SIMD operand checks. Writing `vec[i]` with a runtime `i`
no longer compiles — cast to `[N]T` first via `@bitCast`, then index,
then cast back.

### `std.Io.Reader.takeLeb128` ≠ WASM LEB128

The 0.16 stdlib LEB128 reader does NOT enforce the "integer too large"
error that the WASM spec requires (10 bytes max for 64-bit LEB128).
Keep the in-repo 0.15-style inline port for spec-conformant decoding.

### Long-lived `std.Io.Threaded.io()` segfaults in `Io.Timestamp.now`

Creating a fresh `std.Io.Threaded` inside a long-running `main()` and
handing out its `.io()` segfaults after many iterations (observed in
`e2e_runner.main` iterating test files). Fix: thread `init.io` from
`std.process.Init` instead. Short-scope per-test-body locals are OK.

### `nix develop --command` re-sets `SDKROOT` inside direnv shells

On macOS with the project's direnv + claude-direnv setup, the flake is
already active and `DEVELOPER_DIR`/`SDKROOT` are deliberately unset.
Wrapping a command in `nix develop --command` re-enters the shell, which
resets `SDKROOT`, which makes `/usr/bin/git` (the Apple xcrun stub) fail
with "tool 'git' not found". Just call tools directly.
