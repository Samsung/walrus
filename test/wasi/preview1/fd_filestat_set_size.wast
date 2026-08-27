(module
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_filestat_set_size"
    (func $fd_filestat_set_size (param i32 i64) (result i32)))
  (import "wasi_snapshot_preview1" "fd_seek"
    (func $fd_seek (param i32 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close (param i32) (result i32)))

  (memory 1)

  ;; File path.
  (data (i32.const 300) "./write_to_this.txt")

(func (export "test_filestat_set_size") (result i32)
  (local $fd i32)
  (local $errno i32)

  ;; Open the test file.
  i32.const 3
  i32.const 0
  i32.const 300
  i32.const 19
  i32.const 0
  i64.const 4194308
  ;; FD_SEEK | FD_FILESTAT_SET_SIZE
  ;; 4 | 4194304 = 4194308
  i64.const 0
  i32.const 0
  i32.const 0
  call $path_open
  local.set $errno

  ;; Return path_open errno if opening failed.
  local.get $errno
  if
    local.get $errno
    return
  end

  ;; Load the opened file descriptor from memory address 0.
  i32.const 0
  i32.load
  local.set $fd

  ;; Set the file size to 7 bytes.
  local.get $fd
  i64.const 7
  call $fd_filestat_set_size
  local.set $errno

  ;; Return fd_filestat_set_size errno if it failed.
  local.get $errno
  if
    local.get $errno
    return
  end

  ;; Seek to the end of the file.
  ;; fd_seek writes the resulting offset to memory address 1000.
  local.get $fd
  i64.const 0
  i32.const 2
  ;; WHENCE_END
  i32.const 1000
  call $fd_seek
  local.set $errno

  ;; Return fd_seek errno if it failed.
  local.get $errno
  if
    local.get $errno
    return
  end

  ;; The end position must be 7.
  i32.const 1000
  i64.load
  i64.const 7
  i64.ne

  if
    i32.const 1
    return
  end

  ;; Close the file.
  local.get $fd
  call $fd_close
  drop

  ;; Success.
  i32.const 0)

  (func (export "test_filestat_set_size_invalid_fd") (result i32)
    i32.const 99
    i64.const 7
    call $fd_filestat_set_size))

(assert_return (invoke "test_filestat_set_size") (i32.const 0))
(assert_return (invoke "test_filestat_set_size_invalid_fd") (i32.const 8))

