const std = @import("std");
const builtin = @import("builtin");

pub const windows = std.os.windows;

const page_size = std.heap.page_size_min;

pub const Protection = enum {
    none,
    read_write,
    read_exec,
};

pub const PageError = error{
    OutOfMemory,
    PermissionDenied,
    InvalidAddress,
    Unexpected,
};

// Zig 0.16 trimmed `std.os.windows.kernel32` down to `CreateProcessW` only —
// the VM-management entry points we need for JIT codegen are no longer in
// stdlib. Declare our own externs. Signatures match the Win32 SDK. Constants
// live on `windows.MEM` / `windows.PAGE` which are still stdlib-provided.
extern "kernel32" fn VirtualAlloc(
    lpAddress: ?*anyopaque,
    dwSize: windows.SIZE_T,
    flAllocationType: windows.MEM.ALLOCATE,
    flProtect: windows.PAGE,
) callconv(.winapi) ?*anyopaque;

extern "kernel32" fn VirtualFree(
    lpAddress: *anyopaque,
    dwSize: windows.SIZE_T,
    dwFreeType: windows.MEM.FREE,
) callconv(.winapi) windows.BOOL;

extern "kernel32" fn VirtualProtect(
    lpAddress: *anyopaque,
    dwSize: windows.SIZE_T,
    flNewProtect: windows.PAGE,
    lpflOldProtect: *windows.PAGE,
) callconv(.winapi) windows.BOOL;

pub extern "kernel32" fn DuplicateHandle(
    hSourceProcessHandle: windows.HANDLE,
    hSourceHandle: windows.HANDLE,
    hTargetProcessHandle: windows.HANDLE,
    lpTargetHandle: *windows.HANDLE,
    dwDesiredAccess: windows.DWORD,
    bInheritHandle: windows.BOOL,
    dwOptions: windows.DWORD,
) callconv(.winapi) windows.BOOL;

pub const DUPLICATE_SAME_ACCESS: windows.DWORD = 0x00000002;

extern "kernel32" fn FlushInstructionCache(
    hProcess: windows.HANDLE,
    lpBaseAddress: ?*const anyopaque,
    dwSize: windows.SIZE_T,
) callconv(.winapi) windows.BOOL;

pub extern "kernel32" fn FlushFileBuffers(
    hFile: windows.HANDLE,
) callconv(.winapi) windows.BOOL;

// WASI I/O shims. Zig 0.16 sets `std.c.fd_t = windows.HANDLE` on Windows
// but `std.c.write`/`read`/`lseek`/... are bound to MSVCRT `_write(int fd, …)`,
// so passing a HANDLE where an int fd is expected silently corrupts Windows
// stdio. The functions below present a POSIX-style API and dispatch to
// Win32 `WriteFile`/`ReadFile`/`SetFilePointerEx` on Windows, and keep the
// std.c.* path on POSIX.
extern "kernel32" fn WriteFile(
    hFile: windows.HANDLE,
    lpBuffer: [*]const u8,
    nNumberOfBytesToWrite: windows.DWORD,
    lpNumberOfBytesWritten: *windows.DWORD,
    lpOverlapped: ?*Overlapped,
) callconv(.winapi) windows.BOOL;

extern "kernel32" fn ReadFile(
    hFile: windows.HANDLE,
    lpBuffer: [*]u8,
    nNumberOfBytesToRead: windows.DWORD,
    lpNumberOfBytesRead: *windows.DWORD,
    lpOverlapped: ?*Overlapped,
) callconv(.winapi) windows.BOOL;

extern "kernel32" fn SetFilePointerEx(
    hFile: windows.HANDLE,
    liDistanceToMove: windows.LARGE_INTEGER,
    lpNewFilePointer: ?*windows.LARGE_INTEGER,
    dwMoveMethod: windows.DWORD,
) callconv(.winapi) windows.BOOL;

extern "kernel32" fn CloseHandle(
    hObject: windows.HANDLE,
) callconv(.winapi) windows.BOOL;

extern "kernel32" fn GetLastError() callconv(.winapi) windows.DWORD;

// Flattened OVERLAPPED layout — we only use the Offset/OffsetHigh path.
const Overlapped = extern struct {
    Internal: usize = 0,
    InternalHigh: usize = 0,
    Offset: windows.DWORD = 0,
    OffsetHigh: windows.DWORD = 0,
    hEvent: ?windows.HANDLE = null,
};

const FILE_BEGIN: windows.DWORD = 0;
const FILE_CURRENT: windows.DWORD = 1;
const FILE_END: windows.DWORD = 2;

const ERROR_HANDLE_EOF: windows.DWORD = 38;
const ERROR_BROKEN_PIPE: windows.DWORD = 109;

/// POSIX-style write. Returns bytes written (>= 0) or -1 on error.
pub fn pfdWrite(handle: std.posix.fd_t, buf: []const u8) isize {
    if (builtin.os.tag == .windows) {
        var written: windows.DWORD = 0;
        const ok = WriteFile(handle, buf.ptr, @intCast(buf.len), &written, null);
        if (ok == windows.BOOL.FALSE) return -1;
        return @intCast(written);
    }
    return std.c.write(handle, buf.ptr, buf.len);
}

/// POSIX-style read. Returns bytes read (>= 0, 0 == EOF) or -1 on error.
pub fn pfdRead(handle: std.posix.fd_t, buf: []u8) isize {
    if (builtin.os.tag == .windows) {
        var got: windows.DWORD = 0;
        const ok = ReadFile(handle, buf.ptr, @intCast(buf.len), &got, null);
        if (ok == windows.BOOL.FALSE) {
            const err = GetLastError();
            if (err == ERROR_BROKEN_PIPE or err == ERROR_HANDLE_EOF) return 0;
            return -1;
        }
        return @intCast(got);
    }
    return std.c.read(handle, buf.ptr, buf.len);
}

/// POSIX-style positional read. Does not move the file offset.
pub fn pfdPread(handle: std.posix.fd_t, buf: []u8, offset: u64) isize {
    if (builtin.os.tag == .windows) {
        var ov: Overlapped = .{
            .Offset = @truncate(offset),
            .OffsetHigh = @truncate(offset >> 32),
        };
        var got: windows.DWORD = 0;
        const ok = ReadFile(handle, buf.ptr, @intCast(buf.len), &got, &ov);
        if (ok == windows.BOOL.FALSE) {
            const err = GetLastError();
            if (err == ERROR_BROKEN_PIPE or err == ERROR_HANDLE_EOF) return 0;
            return -1;
        }
        return @intCast(got);
    }
    return std.c.pread(handle, buf.ptr, buf.len, @intCast(offset));
}

/// POSIX-style positional write. Does not move the file offset.
pub fn pfdPwrite(handle: std.posix.fd_t, buf: []const u8, offset: u64) isize {
    if (builtin.os.tag == .windows) {
        var ov: Overlapped = .{
            .Offset = @truncate(offset),
            .OffsetHigh = @truncate(offset >> 32),
        };
        var written: windows.DWORD = 0;
        const ok = WriteFile(handle, buf.ptr, @intCast(buf.len), &written, &ov);
        if (ok == windows.BOOL.FALSE) return -1;
        return @intCast(written);
    }
    return std.c.pwrite(handle, buf.ptr, buf.len, @intCast(offset));
}

/// POSIX-style seek. `whence` uses `std.posix.SEEK.{SET,CUR,END}`.
/// Returns the new offset or -1 on error.
pub fn pfdSeek(handle: std.posix.fd_t, offset: i64, whence: c_int) i64 {
    if (builtin.os.tag == .windows) {
        const method: windows.DWORD = switch (whence) {
            std.posix.SEEK.SET => FILE_BEGIN,
            std.posix.SEEK.CUR => FILE_CURRENT,
            std.posix.SEEK.END => FILE_END,
            else => return -1,
        };
        var new_pos: windows.LARGE_INTEGER = 0;
        const ok = SetFilePointerEx(handle, offset, &new_pos, method);
        if (ok == windows.BOOL.FALSE) return -1;
        return new_pos;
    }
    return std.c.lseek(handle, offset, whence);
}

pub fn pfdClose(handle: std.posix.fd_t) void {
    if (builtin.os.tag == .windows) {
        _ = CloseHandle(handle);
        return;
    }
    _ = std.c.close(handle);
}

pub fn pfdFsync(handle: std.posix.fd_t) i32 {
    if (builtin.os.tag == .windows) {
        return if (FlushFileBuffers(handle) == windows.BOOL.FALSE) -1 else 0;
    }
    return std.c.fsync(handle);
}

pub fn reservePages(size: usize, prot: Protection) PageError![]align(page_size) u8 {
    if (builtin.os.tag == .windows) {
        const addr = VirtualAlloc(null, size, .{ .RESERVE = true }, protectionToWin(prot)) orelse
            return error.OutOfMemory;
        const ptr: [*]align(page_size) u8 = @ptrCast(@alignCast(addr));
        return ptr[0..size];
    }

    const posix = std.posix;
    const buf = posix.mmap(
        null,
        size,
        protectionToPosix(prot),
        .{ .TYPE = .PRIVATE, .ANONYMOUS = true },
        -1,
        0,
    ) catch return error.OutOfMemory;
    return @alignCast(buf);
}

pub fn allocatePages(size: usize, prot: Protection) PageError![]align(page_size) u8 {
    if (builtin.os.tag == .windows) {
        const addr = VirtualAlloc(
            null,
            size,
            .{ .RESERVE = true, .COMMIT = true },
            protectionToWin(prot),
        ) orelse return error.OutOfMemory;
        const ptr: [*]align(page_size) u8 = @ptrCast(@alignCast(addr));
        return ptr[0..size];
    }

    return reservePages(size, prot);
}

pub fn commitPages(region: []align(page_size) u8, prot: Protection) PageError!void {
    if (region.len == 0) return;

    if (builtin.os.tag == .windows) {
        const addr = VirtualAlloc(
            region.ptr,
            region.len,
            .{ .COMMIT = true },
            protectionToWin(prot),
        ) orelse return error.OutOfMemory;
        if (@intFromPtr(addr) != @intFromPtr(region.ptr)) return error.Unexpected;
        return;
    }

    try mprotectPosix(region, protectionToPosix(prot));
}

pub fn protectPages(region: []align(page_size) u8, prot: Protection) PageError!void {
    if (region.len == 0) return;

    if (builtin.os.tag == .windows) {
        var old_protect: windows.PAGE = .{};
        if (VirtualProtect(region.ptr, region.len, protectionToWin(prot), &old_protect) == windows.BOOL.FALSE) {
            return error.PermissionDenied;
        }
        return;
    }

    try mprotectPosix(region, protectionToPosix(prot));
}

fn mprotectPosix(region: []align(page_size) u8, prot: std.posix.PROT) PageError!void {
    if (builtin.link_libc) {
        if (std.c.mprotect(region.ptr, region.len, prot) != 0) return error.PermissionDenied;
        return;
    }
    if (builtin.os.tag == .linux) {
        const rc = std.os.linux.mprotect(region.ptr, region.len, prot);
        switch (std.posix.errno(rc)) {
            .SUCCESS => return,
            else => return error.PermissionDenied,
        }
    }
    return error.Unexpected;
}

pub fn freePages(region: []align(page_size) u8) void {
    if (region.len == 0) return;

    if (builtin.os.tag == .windows) {
        _ = VirtualFree(region.ptr, 0, .{ .RELEASE = true });
        return;
    }

    std.posix.munmap(region);
}

pub fn flushInstructionCache(ptr: [*]const u8, len: usize) void {
    if (len == 0) return;

    if (builtin.os.tag == .windows) {
        _ = FlushInstructionCache(windows.GetCurrentProcess(), ptr, len);
    } else if (builtin.os.tag == .macos) {
        const func = @extern(*const fn ([*]const u8, usize) callconv(.c) void, .{
            .name = "sys_icache_invalidate",
        });
        func(ptr, len);
    } else if (builtin.os.tag == .linux) {
        const func = @extern(*const fn ([*]const u8, [*]const u8) callconv(.c) void, .{
            .name = "__clear_cache",
        });
        func(ptr, ptr + len);
    }
}

pub fn appCacheDir(alloc: std.mem.Allocator, app_name: []const u8) ![]u8 {
    if (builtin.os.tag == .windows) {
        // Zig 0.16 removed `std.fs.getAppDataDir`. Build the path ourselves
        // from %LOCALAPPDATA% (fall back to %APPDATA%).
        const base = (try envPath(alloc, "LOCALAPPDATA")) orelse
            (try envPath(alloc, "APPDATA")) orelse return error.NoCacheDir;
        defer alloc.free(base);
        return std.fmt.allocPrint(alloc, "{s}\\{s}", .{ base, app_name });
    }

    const home_ptr = std.c.getenv("HOME") orelse return error.NoCacheDir;
    const home = std.mem.span(home_ptr);
    return std.fmt.allocPrint(alloc, "{s}/.cache/{s}", .{ home, app_name });
}

pub fn tempDirPath(alloc: std.mem.Allocator) ![]u8 {
    if (builtin.os.tag == .windows) {
        if (try envPath(alloc, "TEMP")) |path| return path;
        if (try envPath(alloc, "TMP")) |path| return path;
        // Fall back to a reasonable Windows default.
        return alloc.dupe(u8, "C:\\Windows\\Temp");
    }
    if (try envPath(alloc, "TMPDIR")) |path| return path;
    return alloc.dupe(u8, "/tmp");
}

fn envPath(alloc: std.mem.Allocator, name: []const u8) !?[]u8 {
    var name_buf: [256]u8 = undefined;
    if (name.len >= name_buf.len) return error.OutOfMemory;
    @memcpy(name_buf[0..name.len], name);
    name_buf[name.len] = 0;
    const val_ptr = std.c.getenv(@ptrCast(&name_buf)) orelse return null;
    const val = std.mem.span(val_ptr);
    if (val.len == 0) return null;
    return try alloc.dupe(u8, val);
}

fn protectionToWin(prot: Protection) windows.PAGE {
    return switch (prot) {
        .none => .{ .NOACCESS = true },
        .read_write => .{ .READWRITE = true },
        .read_exec => .{ .EXECUTE_READ = true },
    };
}

fn protectionToPosix(prot: Protection) std.posix.PROT {
    return switch (prot) {
        .none => .{},
        .read_write => .{ .READ = true, .WRITE = true },
        .read_exec => .{ .READ = true, .EXEC = true },
    };
}
