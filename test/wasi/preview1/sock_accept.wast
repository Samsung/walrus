(module
  (import "wasi_snapshot_preview1" "sock_accept"
    (func $sock_accept (param i32 i32 i32) (result i32)))

  (memory 1)
  (export "memory" (memory 0))

  ;; Call sock_accept with a non-existent listening socket fd.
  (func (export "test_sock_accept_invalid_fd") (result i32)
    i32.const 99   ;; non-existent socket fd
    i32.const 0    ;; no fd flags
    i32.const 0    ;; address where the accepted fd would be written
    call $sock_accept
  )

  ;; Pass an output pointer that does not have enough remaining memory
  ;; for a 4-byte file descriptor.
  (func (export "test_sock_accept_invalid_pointer") (result i32)
    i32.const 99
    i32.const 0
    i32.const 65535
    call $sock_accept
  )
)

;; Invalid listening socket fd: errno::badf.
(assert_return (invoke "test_sock_accept_invalid_fd") (i32.const 8))

;; Invalid output memory pointer: errno::inval.
(assert_return (invoke "test_sock_accept_invalid_pointer") (i32.const 28))

