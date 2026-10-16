(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_readdir" (func $fd_readdir (param i32 i32 i32 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close" (func $fd_close (param i32) (result i32)))
  (memory 1 1)
  (data (i32.const 100) "./readdir")
  (func $open_readdir (param $rights i64) (result i32)
    i32.const 3
    i32.const 1
    i32.const 100
    i32.const 9
    i32.const 2
    local.get $rights
    local.get $rights
    i32.const 0
    i32.const 0
    call $path_open
  )
  (func $check_entry (param $offset i32) (result i32)
    local.get $offset
    i32.load offset=16
    i32.const 5
    i32.ne
    if
      i32.const 101
      return
    end
    local.get $offset
    i32.load offset=25
    i32.const 0x7478742e
    i32.ne
    if
      i32.const 103
      return
    end
    i32.const 0
  )
  (func (export "readdirAll") (result i32)
    (local $errno i32)
    i64.const 0x4000
    call $open_readdir
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 0
    i32.load
    i32.const 1000
    i32.const 512
    i64.const 0
    i32.const 900
    call $fd_readdir
    local.set $errno
    i32.const 0
    i32.load
    call $fd_close
    drop
    local.get $errno
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
    i32.const 900
    i32.load
    i32.const 58
    i32.ne
    if
      i32.const 100
      return
    end
    i32.const 1000
    call $check_entry
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 1029
    call $check_entry
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 1024
    i32.load8_u
    i32.const 1053
    i32.load8_u
    i32.add
    i32.const 195
    i32.ne
    if
      i32.const 104
      return
    end
    i32.const 0
  )
  (func (export "readdirCookie") (result i32)
    (local $errno i32)
    i64.const 0x4000
    call $open_readdir
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 0
    i32.load
    i32.const 1000
    i32.const 512
    i64.const 0
    i32.const 900
    call $fd_readdir
    local.tee $errno
    i32.eqz
    if
      i32.const 0
      i32.load
      i32.const 2000
      i32.const 512
      i32.const 1000
      i64.load
      i32.const 900
      call $fd_readdir
      local.set $errno
    end
    i32.const 0
    i32.load
    call $fd_close
    drop
    local.get $errno
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
    i32.const 900
    i32.load
    i32.const 29
    i32.ne
    if
      i32.const 100
      return
    end
    i32.const 2000
    call $check_entry
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 2024
    i32.load8_u
    i32.const 1053
    i32.load8_u
    i32.ne
    if
      i32.const 105
      return
    end
    i32.const 0
  )
  (func (export "readdirEnd") (result i32)
    (local $errno i32)
    i64.const 0x4000
    call $open_readdir
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 0
    i32.load
    i32.const 1000
    i32.const 512
    i64.const 0
    i32.const 900
    call $fd_readdir
    local.tee $errno
    i32.eqz
    if
      i32.const 0
      i32.load
      i32.const 2000
      i32.const 512
      i32.const 1029
      i64.load
      i32.const 900
      call $fd_readdir
      local.set $errno
    end
    i32.const 0
    i32.load
    call $fd_close
    drop
    local.get $errno
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
    i32.const 900
    i32.load
    if
      i32.const 100
      return
    end
    i32.const 0
  )
  (func $readdir_with_buffer (param $length i32) (result i32)
    (local $errno i32)
    i64.const 0x4000
    call $open_readdir
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 0
    i32.load
    i32.const 1000
    local.get $length
    i64.const 0
    i32.const 900
    call $fd_readdir
    local.set $errno
    i32.const 0
    i32.load
    call $fd_close
    drop
    local.get $errno
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
    i32.const 900
    i32.load
    local.get $length
    i32.ne
    if
      i32.const 100
      return
    end
    i32.const 0
  )
  (func (export "readdirSmallBuffer") (result i32 i32 i32)
    i32.const 10
    call $readdir_with_buffer
    i32.const 30
    call $readdir_with_buffer
    i32.const 40
    call $readdir_with_buffer
  )
  (func $nosys_or (param $errno i32) (param $expected i32) (result i32)
    local.get $errno
    i32.const 52
    i32.eq
    if
      local.get $expected
      return
    end
    local.get $errno
  )
  (func (export "readdirBadFd") (result i32)
    i32.const 99
    i32.const 1000
    i32.const 512
    i64.const 0
    i32.const 900
    call $fd_readdir
    i32.const 8
    call $nosys_or
  )
  (func (export "readdirWithoutRight") (result i32)
    (local $errno i32)
    i64.const 0x2000
    call $open_readdir
    local.tee $errno
    if
      local.get $errno
      return
    end
    i32.const 0
    i32.load
    i32.const 1000
    i32.const 512
    i64.const 0
    i32.const 900
    call $fd_readdir
    local.set $errno
    i32.const 0
    i32.load
    call $fd_close
    drop
    local.get $errno
    i32.const 76
    call $nosys_or
  )
  (func (export "readdirOutOfBounds") (result i32 i32 i32 i32)
    i32.const 3
    i32.const 65500
    i32.const 100
    i64.const 0
    i32.const 900
    call $fd_readdir
    i32.const 3
    i32.const 0
    i32.const 65537
    i64.const 0
    i32.const 900
    call $fd_readdir
    i32.const 3
    i32.const -1
    i32.const 1
    i64.const 0
    i32.const 900
    call $fd_readdir
    i32.const 3
    i32.const 1000
    i32.const 512
    i64.const 0
    i32.const 65534
    call $fd_readdir
  )
)
(assert_return (invoke "readdirAll") (i32.const 0))
(assert_return (invoke "readdirCookie") (i32.const 0))
(assert_return (invoke "readdirEnd") (i32.const 0))
(assert_return (invoke "readdirSmallBuffer") (i32.const 0) (i32.const 0) (i32.const 0))
(assert_return (invoke "readdirBadFd") (i32.const 8))
(assert_return (invoke "readdirWithoutRight") (i32.const 76))
(assert_return (invoke "readdirOutOfBounds") (i32.const 28) (i32.const 28) (i32.const 28) (i32.const 28))
