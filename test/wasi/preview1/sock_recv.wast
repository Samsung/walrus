(module
  (import "wasi_snapshot_preview1" "sock_recv"
    (func $sock_recv (param i32 i32 i32 i32 i32 i32) (result i32)))

  (memory 1)
  (export "memory" (memory 0))

  (func (export "test_sock_recv_invalid_fd") (result i32)
    i32.const 100
    i32.const 400
    i32.store

    i32.const 104
    i32.const 5
    i32.store

    i32.const 99
    i32.const 100
    i32.const 1
    i32.const 0
    i32.const 200
    i32.const 204
    call $sock_recv
  )

  (func (export "test_sock_recv_invalid_pointer") (result i32)
    i32.const 99
    i32.const 65535
    i32.const 1
    i32.const 0
    i32.const 200
    i32.const 204
    call $sock_recv
  )
)

(assert_return
  (invoke "test_sock_recv_invalid_fd")
  (i32.const 8))

(assert_return
  (invoke "test_sock_recv_invalid_pointer")
  (i32.const 28))