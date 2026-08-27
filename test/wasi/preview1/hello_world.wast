(module
  (import "wasi_snapshot_preview1" "fd_write" (func $wasi_fd_write (param i32 i32 i32 i32) (result i32)))
  (memory 1)
  
  (export "memory" (memory 0))
  (export "hello_world" (func $hello_world))
  (data (i32.const 0) "Hello World!\n")

  (func $hello_world
    (i32.store (i32.const 100) (i32.const 0))
    (i32.store (i32.const 104) (i32.const 13))
    (i32.store (i32.const 200) (i32.const 0))
    (call $wasi_fd_write
          (i32.const 1)  ;;file descriptor
          (i32.const 100) ;;offset of str offset
          (i32.const 1)  ;;iovec length
          (i32.const 200) ;;result offset
    )
    drop
  )
)

(assert_return (invoke "hello_world"))
