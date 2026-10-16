(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_fdstat_set_flags" (func $fd_fdstat_set_flags (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_seek" (func $fd_seek (param i32 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_tell" (func $fd_tell (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close" (func $fd_close (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_unlink_file" (func $path_unlink_file (param i32 i32 i32) (result i32)))
  (memory 1 1)
  (data (i32.const 100) "./set_flags.txt")
  (data (i32.const 120) "./text.txt")
  (data (i32.const 300) "abcde")
  (func $write (param $offset i32) (param $length i32) (result i32)
    i32.const 200
    local.get $offset
    i32.store
    i32.const 204
    local.get $length
    i32.store
    i32.const 0
    i32.load
    i32.const 200
    i32.const 1
    i32.const 400
    call $fd_write
  )
  (func $rewind (result i32)
    i32.const 0
    i32.load
    i64.const 0
    i32.const 0
    i32.const 500
    call $fd_seek
  )
  (func $tell (result i32)
    i32.const 0
    i32.load
    i32.const 500
    call $fd_tell
  )
  (func $set_flags_and_write (result i32)
    (local $errno i32)
    i32.const 300
    i32.const 3
    call $write
    local.tee $errno
    if
      local.get $errno
      return
    end
    call $rewind
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 0
    i32.load
    i32.const 1
    call $fd_fdstat_set_flags
    local.tee $errno
    i32.const 52
    i32.eq
    if
      i32.const 0
      return
    end
    local.get $errno
    if
      local.get $errno
      return
    end
    i32.const 303
    i32.const 2
    call $write
    local.tee $errno
    if
      local.get $errno
      return
    end
    call $tell
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 500
    i64.load
    i64.const 5
    i64.ne
    if
      i32.const 100
      return
    end
    i32.const 0
    i32.load
    i32.const 0
    call $fd_fdstat_set_flags
    local.tee $errno
    if
      local.get $errno
      return
    end
    call $rewind
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 300
    i32.const 1
    call $write
    local.tee $errno
    if
      local.get $errno
      return
    end
    call $tell
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 500
    i64.load
    i64.const 1
    i64.ne
    if
      i32.const 101
      return
    end
    i32.const 0
  )
  (func (export "setFlags") (result i32)
    (local $errno i32)
    i32.const 3
    i32.const 1
    i32.const 100
    i32.const 15
    i32.const 9
    i64.const 0x6c
    i64.const 0x6c
    i32.const 0
    i32.const 0
    call $path_open
    local.tee $errno
    if
      local.get $errno
      return
    end
    call $set_flags_and_write
    local.set $errno
    i32.const 0
    i32.load
    call $fd_close
    drop
    i32.const 3
    i32.const 100
    i32.const 15
    call $path_unlink_file
    drop
    local.get $errno
  )
  (func (export "setFlagsBadFd") (result i32)
    (local $errno i32)
    i32.const 99
    i32.const 1
    call $fd_fdstat_set_flags
    local.tee $errno
    i32.const 52
    i32.eq
    if
      i32.const 8
      return
    end
    local.get $errno
  )
  (func (export "setFlagsWithoutRight") (result i32)
    (local $errno i32)
    i32.const 3
    i32.const 1
    i32.const 120
    i32.const 10
    i32.const 0
    i64.const 0x2
    i64.const 0x2
    i32.const 0
    i32.const 0
    call $path_open
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 0
    i32.load
    i32.const 1
    call $fd_fdstat_set_flags
    local.set $errno
    i32.const 0
    i32.load
    call $fd_close
    drop
    local.get $errno
    i32.const 52
    i32.eq
    if
      i32.const 76
      return
    end
    local.get $errno
  )
)
(assert_return (invoke "setFlags") (i32.const 0))
(assert_return (invoke "setFlagsBadFd") (i32.const 8))
(assert_return (invoke "setFlagsWithoutRight") (i32.const 76))
