(module
  (import "wasi_snapshot_preview1" "poll_oneoff"
    (func $poll_oneoff (param i32 i32 i32 i32) (result i32)))

  (memory 1 1)

  (func (export "pollInOutOfBounds") (result i32)
    i32.const 65488 ;; in
    i32.const 0     ;; out
    i32.const 1000  ;; nsubscriptions
    i32.const 100   ;; nevents
    call $poll_oneoff)

  (func (export "pollOutOutOfBounds") (result i32)
    i32.const 0
    i32.const 65504
    i32.const 1000
    i32.const 100
    call $poll_oneoff)

  (func (export "pollValid") (result i32)
    i32.const 0
    i32.const 200
    i32.const 1
    i32.const 100
    call $poll_oneoff))

(assert_return (invoke "pollInOutOfBounds") (i32.const 28))
(assert_return (invoke "pollOutOutOfBounds") (i32.const 28))
(assert_return (invoke "pollValid") (i32.const 0))
