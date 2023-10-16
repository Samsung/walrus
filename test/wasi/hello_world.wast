(module
  (import "wasi_snapshot_preview1" "fd_write" (func $__wasi_fd_write (param i32 i32 i32 i32) (result i32))) 
  (memory 1)
  
  (export "memory" (memory 0))
  (export "_start" (func $_start))
  (data (i32.const 0) "Hello World!\n")

  (func $_start (result i32)
    (i32.store (i32.const 24) (i32.const 13)) ;; Lenght of "Hello World!\n"
    (i32.store (i32.const 20) (i32.const 0))  ;; memory offset of "Hello World!\n"
    (call $__wasi_fd_write
          (i32.const 1)  ;;file descriptor
          (i32.const 20) ;;offset of str offset
          (i32.const 1)  ;;iovec length
          (i32.const 30) ;;result offset
    )
    drop
    i32.const 32
    i32.load
  )
)

(assert_return (invoke "_start") (i32.const 0))
