(component
  (type (;0;)
    (instance
      (export (;0;) "error" (type (sub resource)))
    )
  )
  (import "wasi:io/error@0.2.6" (instance (;0;) (type 0)))
  (alias export 0 "error" (type (;1;)))
  (type (;2;)
    (instance
      (export (;0;) "input-stream" (type (sub resource)))
      (alias outer 1 1 (type (;1;)))
      (export (;2;) "error" (type (eq 1)))
      (type (;3;) (own 2))
      (type (;4;) (variant (case "last-operation-failed" 3) (case "closed")))
      (export (;5;) "stream-error" (type (eq 4)))
      (type (;6;) (borrow 0))
      (type (;7;) (list u8))
      (type (;8;) (result 7 (error 5)))
      (type (;9;) (func (param "self" 6) (param "len" u64) (result 8)))
      (export (;0;) "[method]input-stream.read" (func (type 9)))
    )
  )
  (import "wasi:io/streams@0.2.6" (instance (;1;) (type 2)))
  (alias export 1 "input-stream" (type (;3;)))
  (type (;4;)
    (instance
      (export (;0;) "descriptor" (type (sub resource)))
      (type (;1;) u64)
      (export (;2;) "filesize" (type (eq 1)))
      (alias outer 1 3 (type (;3;)))
      (export (;4;) "input-stream" (type (eq 3)))
      (type (;5;) (enum "access" "would-block" "already" "bad-descriptor" "busy" "deadlock" "quota" "exist" "file-too-large" "illegal-byte-sequence" "in-progress" "interrupted" "invalid" "io" "is-directory" "loop" "too-many-links" "message-size" "name-too-long" "no-device" "no-entry" "no-lock" "insufficient-memory" "insufficient-space" "not-directory" "not-empty" "not-recoverable" "unsupported" "no-tty" "no-such-device" "overflow" "not-permitted" "pipe" "read-only" "invalid-seek" "text-file-busy" "cross-device"))
      (export (;6;) "error-code" (type (eq 5)))
      (type (;7;) (flags "symlink-follow"))
      (export (;8;) "path-flags" (type (eq 7)))
      (type (;9;) (flags "create" "directory" "exclusive" "truncate"))
      (export (;10;) "open-flags" (type (eq 9)))
      (type (;11;) (flags "read" "write" "file-integrity-sync" "data-integrity-sync" "requested-write-sync" "mutate-directory"))
      (export (;12;) "descriptor-flags" (type (eq 11)))
      (type (;13;) (borrow 0))
      (type (;14;) (own 4))
      (type (;15;) (result 14 (error 6)))
      (type (;16;) (func (param "self" 13) (param "offset" 2) (result 15)))
      (export (;0;) "[method]descriptor.read-via-stream" (func (type 16)))
      (type (;17;) (own 0))
      (type (;18;) (result 17 (error 6)))
      (type (;19;) (func (param "self" 13) (param "path-flags" 8) (param "path" string) (param "open-flags" 10) (param "flags" 12) (result 18)))
      (export (;1;) "[method]descriptor.open-at" (func (type 19)))
    )
  )
  (import "wasi:filesystem/types@0.2.6" (instance (;2;) (type 4)))
  (alias export 2 "descriptor" (type (;5;)))
  (type (;6;)
    (instance
      (alias outer 1 5 (type (;0;)))
      (export (;1;) "descriptor" (type (eq 0)))
      (type (;2;) (own 1))
      (type (;3;) (tuple 2 string))
      (type (;4;) (list 3))
      (type (;5;) (func (result 4)))
      (export (;0;) "get-directories" (func (type 5)))
    )
  )
  (import "wasi:filesystem/preopens@0.2.6" (instance (;3;) (type 6)))
  (core module (;0;)
    (type (;0;) (func (param i32 i64 i32)))
    (type (;1;) (func (param i32 i32 i32 i32 i32 i32 i32)))
    (type (;2;) (func (param i32)))
    (type (;3;) (func (param i32 i32 i32 i32) (result i32)))
    (type (;4;) (func))
    (type (;5;) (func (param i32 i64 i32)))
    (import "wasi:filesystem/types@0.2.6" "[method]descriptor.read-via-stream" (func $read-via-stream (;0;) (type 0)))
    (import "wasi:filesystem/types@0.2.6" "[method]descriptor.open-at" (func $open-at (;1;) (type 1)))
    (import "wasi:filesystem/preopens@0.2.6" "get-directories" (func $get-directories (;2;) (type 2)))
    (import "wasi:io/streams@0.2.6" "[method]input-stream.read" (func $input-stream-read (;3;) (type 5)))
    (memory (;0;) 1)
    (global $heap (;0;) (mut i32) i32.const 4096)
    (export "memory" (memory 0))
    (export "cabi_realloc" (func 4))
    (export "run" (func 5))
    (export "cabi_post_run" (func 6))
    (export "_initialize" (func 7))
    (func (;4;) (type 3) (param $old-ptr i32) (param $old-size i32) (param $align i32) (param $new-size i32) (result i32)
      (local $ptr i32)
      global.get $heap
      local.get $align
      i32.const 1
      i32.sub
      i32.add
      local.get $align
      i32.const 1
      i32.sub
      i32.const -1
      i32.xor
      i32.and
      local.tee $ptr
      local.get $new-size
      i32.add
      global.set $heap
      local.get $ptr
    )
    (func (;5;) (type 4)
      (local $directory i32) (local $file i32)
      i32.const 0
      call $get-directories
      i32.const 0
      i32.load
      i32.load
      local.set $directory
      local.get $directory
      i32.const 0
      i32.const 1024
      i32.const 10
      i32.const 0
      i32.const 1
      i32.const 16
      call $open-at
      i32.const 20
      i32.load
      local.set $file
      local.get $file
      i64.const 65536
      i32.const 32
      call $read-via-stream
      i32.const 36
      i32.load
      i64.const 1048576
      i32.const 1024
      call $input-stream-read
    )
    (func (;6;) (type 4))
    (func (;7;) (type 4))
    (data (;0;) (i32.const 1024) "inside.txt")
    (@producers
      (processed-by "wit-component" "0.235.0")
    )
  )
  (core module (;1;)
    (type (;0;) (func (param i32 i64 i32)))
    (type (;1;) (func (param i32 i32 i32 i32 i32 i32 i32)))
    (type (;2;) (func (param i32)))
    (table (;0;) 4 4 funcref)
    (export "0" (func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.read-via-stream"))
    (export "1" (func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.open-at"))
    (export "2" (func $indirect-wasi:filesystem/preopens@0.2.6-get-directories))
    (export "3" (func $"indirect-wasi:io/streams@0.2.6-[method]input-stream.read"))
    (export "$imports" (table 0))
    (func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.read-via-stream" (;0;) (type 0) (param i32 i64 i32)
      local.get 0
      local.get 1
      local.get 2
      i32.const 0
      call_indirect (type 0)
    )
    (func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.open-at" (;1;) (type 1) (param i32 i32 i32 i32 i32 i32 i32)
      local.get 0
      local.get 1
      local.get 2
      local.get 3
      local.get 4
      local.get 5
      local.get 6
      i32.const 1
      call_indirect (type 1)
    )
    (func $indirect-wasi:filesystem/preopens@0.2.6-get-directories (;2;) (type 2) (param i32)
      local.get 0
      i32.const 2
      call_indirect (type 2)
    )
    (func $"indirect-wasi:io/streams@0.2.6-[method]input-stream.read" (;3;) (type 0) (param i32 i64 i32)
      local.get 0
      local.get 1
      local.get 2
      i32.const 3
      call_indirect (type 0)
    )
    (@producers
      (processed-by "wit-component" "0.235.0")
    )
  )
  (core module (;2;)
    (type (;0;) (func (param i32 i64 i32)))
    (type (;1;) (func (param i32 i32 i32 i32 i32 i32 i32)))
    (type (;2;) (func (param i32)))
    (import "" "0" (func (;0;) (type 0)))
    (import "" "1" (func (;1;) (type 1)))
    (import "" "2" (func (;2;) (type 2)))
    (import "" "3" (func (;3;) (type 0)))
    (import "" "$imports" (table (;0;) 4 4 funcref))
    (elem (;0;) (i32.const 0) func 0 1 2 3)
    (@producers
      (processed-by "wit-component" "0.235.0")
    )
  )
  (core instance (;0;) (instantiate 1))
  (alias core export 0 "0" (core func (;0;)))
  (alias core export 0 "1" (core func (;1;)))
  (core instance (;1;)
    (export "[method]descriptor.read-via-stream" (func 0))
    (export "[method]descriptor.open-at" (func 1))
  )
  (alias core export 0 "2" (core func (;2;)))
  (core instance (;2;)
    (export "get-directories" (func 2))
  )
  (alias core export 0 "3" (core func (;3;)))
  (core instance (;3;)
    (export "[method]input-stream.read" (func 3))
  )
  (core instance (;4;) (instantiate 0
      (with "wasi:filesystem/types@0.2.6" (instance 1))
      (with "wasi:filesystem/preopens@0.2.6" (instance 2))
      (with "wasi:io/streams@0.2.6" (instance 3))
    )
  )
  (alias core export 4 "memory" (core memory (;0;)))
  (alias core export 0 "$imports" (core table (;0;)))
  (alias export 2 "[method]descriptor.read-via-stream" (func (;0;)))
  (alias core export 4 "cabi_realloc" (core func (;4;)))
  (core func (;5;) (canon lower (func 0) (memory 0)))
  (alias export 2 "[method]descriptor.open-at" (func (;1;)))
  (core func (;6;) (canon lower (func 1) (memory 0) string-encoding=utf8))
  (alias export 3 "get-directories" (func (;2;)))
  (core func (;7;) (canon lower (func 2) (memory 0) (realloc 4) string-encoding=utf8))
  (alias export 1 "[method]input-stream.read" (func (;3;)))
  (core func (;8;) (canon lower (func 3) (memory 0) (realloc 4)))
  (core instance (;5;)
    (export "$imports" (table 0))
    (export "0" (func 5))
    (export "1" (func 6))
    (export "2" (func 7))
    (export "3" (func 8))
  )
  (core instance (;6;) (instantiate 2
      (with "" (instance 5))
    )
  )
  (alias core export 4 "_initialize" (core func (;9;)))
  (core module (;3;)
    (type (;0;) (func))
    (import "" "" (func (;0;) (type 0)))
    (start 0)
  )
  (core instance (;7;)
    (export "" (func 9))
  )
  (core instance (;8;) (instantiate 3
      (with "" (instance 7))
    )
  )
  (type (;7;) (func))
  (alias core export 4 "run" (core func (;10;)))
  (alias core export 4 "cabi_post_run" (core func (;11;)))
  (func (;4;) (type 7) (canon lift (core func 10) (post-return 11)))
  (export (;5;) "run" (func 4))
  (@producers
    (processed-by "wit-component" "0.235.0")
  )
)
