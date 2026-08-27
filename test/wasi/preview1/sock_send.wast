(module
  (import "wasi_snapshot_preview1" "sock_send"
    (func $sock_send (param i32 i32 i32 i32 i32) (result i32)))

  (memory 1)

  (func (export "test_sock_send_invalid_fd") (result i32)
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
    call $sock_send
  )

  (func (export "test_sock_send_invalid_iovec_pointer") (result i32)
    i32.const 99
    i32.const 65532
    i32.const 1
    i32.const 0
    i32.const 200
    call $sock_send
  )

  (func (export "test_sock_send_invalid_buffer_pointer") (result i32)
    i32.const 100
    i32.const 65534
    i32.store

    i32.const 104
    i32.const 4
    i32.store

    i32.const 99
    i32.const 100
    i32.const 1
    i32.const 0
    i32.const 200
    call $sock_send
  )

  (data (i32.const 400) "hello")

  (export "memory" (memory 0))
)

(assert_return
  (invoke "test_sock_send_invalid_fd")
  (i32.const 8))

(assert_return
  (invoke "test_sock_send_invalid_iovec_pointer")
  (i32.const 28))

(assert_return
  (invoke "test_sock_send_invalid_buffer_pointer")
  (i32.const 28))