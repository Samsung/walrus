(module
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open
      (param i32 i32 i32 i32 i32 i64 i64 i32 i32)
      (result i32)))

  (import "wasi_snapshot_preview1" "fd_fdstat_set_rights"
    (func $fd_fdstat_set_rights
      (param i32 i64 i64)
      (result i32)))

  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close
      (param i32)
      (result i32)))

  (memory 1)

  (data (i32.const 32) "./fd_fdstat_set_rights.txt")

  (func (export "test_fdstat_set_rights") (result i32)
    (local $fd i32)
    (local $error i32)

    i32.const 3
    i32.const 0
    i32.const 32
    i32.const 26
    i32.const 1
    i64.const 0x42
    i64.const 0
    i32.const 0
    i32.const 0
    call $path_open
    local.set $error

    local.get $error
    if
      local.get $error
      return
    end

    i32.const 0
    i32.load
    local.set $fd

    local.get $fd
    i64.const 0x2
    i64.const 0
    call $fd_fdstat_set_rights
    local.set $error

    local.get $fd
    call $fd_close
    drop

    local.get $error
  )

  (func (export "test_fdstat_set_rights_badfd") (result i32)
    i32.const 999
    i64.const 0
    i64.const 0
    call $fd_fdstat_set_rights
  )
)

(assert_return
  (invoke "test_fdstat_set_rights")
  (i32.const 0)
)

(assert_return
  (invoke "test_fdstat_set_rights_badfd")
  (i32.const 8)
)