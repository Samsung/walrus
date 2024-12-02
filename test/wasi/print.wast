(module
    (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))

    (memory $memory 1 1)
    (data (i32.const 100) "Hello ")
    (data (i32.const 200) "World!\n")

    (func (export "writeToStdout") (result i32 i32)
      ;; Write iovs into memory from offset 16
      i32.const 16
      i32.const 100
      i32.store

      i32.const 20
      i32.const 6
      i32.store

      i32.const 24
      i32.const 200
      i32.store

      i32.const 28
      i32.const 7
      i32.store

      i32.const 1 ;; stdout
      i32.const 16 ;; iovs offset
      i32.const 2 ;; iovsLen
      i32.const 8 ;; nwritten

      call $fd_write

      i32.const 8
      i32.load
    )

    (func (export "badWrite") (result i32 i32 i32 i32)
      i32.const 100000 ;; invalid descriptor
      i32.const 8 ;; iovs offset
      i32.const 1 ;; iovsLen
      i32.const 4 ;; nwritten

      call $fd_write

      i32.const 1 ;; stdout
      i32.const 65529 ;; bad iovs offset
      i32.const 1 ;; iovsLen
      i32.const 8 ;; nwritten

      call $fd_write

      i32.const 1 ;; stdout
      i32.const 16 ;; iovs offset
      i32.const 1 ;; iovsLen
      i32.const 65533 ;; bad nwritten

      call $fd_write

      i32.const 1 ;; stdout
      i32.const 8 ;; iovs offset
      i32.const 8192 ;; bad iovsLen
      i32.const 0 ;; nwritten

      call $fd_write
    )
)

(assert_return (invoke "writeToStdout") (i32.const 0) (i32.const 13))
(assert_return (invoke "badWrite") (i32.const 8) (i32.const 28) (i32.const 28) (i32.const 28))
