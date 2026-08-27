(component
  (type $ty-wasi:io/streams@0.2.6 (;0;)
    (instance
      (export (;0;) "input-stream" (type (sub resource)))
    )
  )
  (import "wasi:io/streams@0.2.6" (instance $wasi:io/streams@0.2.6 (;0;) (type $ty-wasi:io/streams@0.2.6)))
  (alias export $wasi:io/streams@0.2.6 "input-stream" (type $input-stream (;1;)))
  (type $ty-wasi:filesystem/types@0.2.6 (;2;)
    (instance
      (export (;0;) "descriptor" (type (sub resource)))
      (type (;1;) u64)
      (export (;2;) "filesize" (type (eq 1)))
      (alias outer 1 $input-stream (type (;3;)))
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
  (import "wasi:filesystem/types@0.2.6" (instance $wasi:filesystem/types@0.2.6 (;1;) (type $ty-wasi:filesystem/types@0.2.6)))
  (alias export $wasi:filesystem/types@0.2.6 "descriptor" (type $descriptor (;3;)))
  (type $ty-wasi:filesystem/preopens@0.2.6 (;4;)
    (instance
      (alias outer 1 $descriptor (type (;0;)))
      (export (;1;) "descriptor" (type (eq 0)))
      (type (;2;) (own 1))
      (type (;3;) (tuple 2 string))
      (type (;4;) (list 3))
      (type (;5;) (func (result 4)))
      (export (;0;) "get-directories" (func (type 5)))
    )
  )
  (import "wasi:filesystem/preopens@0.2.6" (instance $wasi:filesystem/preopens@0.2.6 (;2;) (type $ty-wasi:filesystem/preopens@0.2.6)))
  (core module $main (;0;)
    (type (;0;) (func (param i32 i64 i32)))
    (type (;1;) (func (param i32 i32 i32 i32 i32 i32 i32)))
    (type (;2;) (func (param i32)))
    (type (;3;) (func (param i32 i32 i32 i32) (result i32)))
    (type (;4;) (func))
    (import "wasi:filesystem/types@0.2.6" "[method]descriptor.read-via-stream" (func $read-via-stream (;0;) (type 0)))
    (import "wasi:filesystem/types@0.2.6" "[method]descriptor.open-at" (func $open-at (;1;) (type 1)))
    (import "wasi:filesystem/preopens@0.2.6" "get-directories" (func $get-directories (;2;) (type 2)))
    (memory (;0;) 1)
    (global $heap (;0;) (mut i32) i32.const 4096)
    (export "memory" (memory 0))
    (export "cabi_realloc" (func 3))
    (export "run" (func 4))
    (export "cabi_post_run" (func 5))
    (export "_initialize" (func 6))
    (func (;3;) (type 3) (param $old-ptr i32) (param $old-size i32) (param $align i32) (param $new-size i32) (result i32)
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
    (func (;4;) (type 4)
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
      i32.const 0
      i32.const 16
      call $open-at
      i32.const 20
      i32.load
      local.set $file
      local.get $file
      i64.const 65536
      i32.const 32
      call $read-via-stream
    )
    (func (;5;) (type 4))
    (func (;6;) (type 4))
    (data (;0;) (i32.const 1024) "inside.txt")
    (@producers
      (processed-by "wit-component" "0.254.0")
    )
  )
  (core module $wit-component-shim-module (;1;)
    (type (;0;) (func (param i32 i64 i32)))
    (type (;1;) (func (param i32 i32 i32 i32 i32 i32 i32)))
    (type (;2;) (func (param i32)))
    (table (;0;) 3 3 funcref)
    (export "0" (func 0))
    (export "1" (func 1))
    (export "2" (func 2))
    (export "$imports" (table 0))
    (func (;0;) (type 0) (param i32 i64 i32)
      local.get 0
      local.get 1
      local.get 2
      i32.const 0
      call_indirect (type 0)
    )
    (func (;1;) (type 1) (param i32 i32 i32 i32 i32 i32 i32)
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
    (func (;2;) (type 2) (param i32)
      local.get 0
      i32.const 2
      call_indirect (type 2)
    )
    (@producers
      (processed-by "wit-component" "0.254.0")
    )
  )
  (core module $wit-component-fixup (;2;)
    (type (;0;) (func (param i32 i64 i32)))
    (type (;1;) (func (param i32 i32 i32 i32 i32 i32 i32)))
    (type (;2;) (func (param i32)))
    (import "" "0" (func (;0;) (type 0)))
    (import "" "1" (func (;1;) (type 1)))
    (import "" "2" (func (;2;) (type 2)))
    (import "" "$imports" (table (;0;) 3 3 funcref))
    (elem (;0;) (i32.const 0) func 0 1 2)
    (@producers
      (processed-by "wit-component" "0.254.0")
    )
  )
  (core instance $wit-component-shim-instance (;0;) (instantiate $wit-component-shim-module))
  (alias core export $wit-component-shim-instance "0" (core func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.read-via-stream" (;0;)))
  (alias core export $wit-component-shim-instance "1" (core func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.open-at" (;1;)))
  (core instance $wasi:filesystem/types@0.2.6 (;1;)
    (export "[method]descriptor.read-via-stream" (func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.read-via-stream"))
    (export "[method]descriptor.open-at" (func $"indirect-wasi:filesystem/types@0.2.6-[method]descriptor.open-at"))
  )
  (alias core export $wit-component-shim-instance "2" (core func $indirect-wasi:filesystem/preopens@0.2.6-get-directories (;2;)))
  (core instance $wasi:filesystem/preopens@0.2.6 (;2;)
    (export "get-directories" (func $indirect-wasi:filesystem/preopens@0.2.6-get-directories))
  )
  (core instance $main (;3;) (instantiate $main
      (with "wasi:filesystem/types@0.2.6" (instance $wasi:filesystem/types@0.2.6))
      (with "wasi:filesystem/preopens@0.2.6" (instance $wasi:filesystem/preopens@0.2.6))
    )
  )
  (alias core export $main "memory" (core memory $memory (;0;)))
  (alias core export $wit-component-shim-instance "$imports" (core table $"shim table" (;0;)))
  (alias export $wasi:filesystem/types@0.2.6 "[method]descriptor.read-via-stream" (func $"[method]descriptor.read-via-stream" (;0;)))
  (alias core export $main "cabi_realloc" (core func $realloc (;3;)))
  (core func $"#core-func4 indirect-wasi:filesystem/types@0.2.6-[method]descriptor.read-via-stream" (;4;) (canon lower (func $"[method]descriptor.read-via-stream") (memory $memory)))
  (alias export $wasi:filesystem/types@0.2.6 "[method]descriptor.open-at" (func $"[method]descriptor.open-at" (;1;)))
  (core func $"#core-func5 indirect-wasi:filesystem/types@0.2.6-[method]descriptor.open-at" (;5;) (canon lower (func $"[method]descriptor.open-at") (memory $memory) string-encoding=utf8))
  (alias export $wasi:filesystem/preopens@0.2.6 "get-directories" (func $get-directories (;2;)))
  (core func $"#core-func6 indirect-wasi:filesystem/preopens@0.2.6-get-directories" (;6;) (canon lower (func $get-directories) (memory $memory) (realloc $realloc) string-encoding=utf8))
  (core instance $fixup-args (;4;)
    (export "$imports" (table $"shim table"))
    (export "0" (func $"#core-func4 indirect-wasi:filesystem/types@0.2.6-[method]descriptor.read-via-stream"))
    (export "1" (func $"#core-func5 indirect-wasi:filesystem/types@0.2.6-[method]descriptor.open-at"))
    (export "2" (func $"#core-func6 indirect-wasi:filesystem/preopens@0.2.6-get-directories"))
  )
  (core instance $fixup (;5;) (instantiate $wit-component-fixup
      (with "" (instance $fixup-args))
    )
  )
  (alias core export $main "_initialize" (core func $start (;7;)))
  (core module $start-shim-module (;3;)
    (type (;0;) (func))
    (import "" "" (func (;0;) (type 0)))
    (start 0)
  )
  (core instance $start-shim-args (;6;)
    (export "" (func $start))
  )
  (core instance $start-shim-instance (;7;) (instantiate $start-shim-module
      (with "" (instance $start-shim-args))
    )
  )
  (type (;5;) (func))
  (alias core export $main "run" (core func $core-run (;8;)))
  (alias core export $main "cabi_post_run" (core func $cabi_post_run (;9;)))
  (func $run (;3;) (type 5) (canon lift (core func $core-run) (post-return $cabi_post_run)))
  (export $"#func4 run" (;4;) "run" (func $run))
  (@producers
    (processed-by "wit-component" "0.254.0")
  )
)
