(module
  (import "wasi_snapshot_preview1" "sock_shutdown"
    (func $sock_shutdown (param i32 i32) (result i32)))

  (memory 1)

  ;; Call sock_shutdown with an invalid fd and how=SHUT_WR(2).
  ;; uvwasi proceeds to fd lookup -> fd not found -> errno::badf (8).
  (func (export "test_sock_shutdown_invalid_fd") (result i32)
    i32.const 99   ;; non-existent fd
    i32.const 2    ;; how: SHUT_WR (the only supported flag)
    call $sock_shutdown
  )

  ;; Call sock_shutdown with how=SHUT_RD(1).
  ;; SHUT_RD is defined but not implemented in uvwasi: (how & ~SHUT_WR) fires -> errno::notsup (58).
  (func (export "test_sock_shutdown_notsup") (result i32)
    i32.const 0    ;; fd (irrelevant: flag check fires first)
    i32.const 1    ;; how: SHUT_RD (defined but not implemented in uvwasi)
    call $sock_shutdown
  )

  (export "memory" (memory 0))
)

;; Invalid fd with valid how: errno::badf (8)
(assert_return (invoke "test_sock_shutdown_invalid_fd") (i32.const 8))

;; Unsupported how flag (SHUT_RD): errno::notsup (58)
(assert_return (invoke "test_sock_shutdown_notsup") (i32.const 58))
