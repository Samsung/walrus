;; A local written only inside a block that a br_if can leave early must
;; still read as zero on the early-exit path.
(module
  (func $dirty (param i32) (result i32)
    (local i32 i32 i32)
    (local.set 1 (i32.const 0x5a5a5a5a))
    (local.set 2 (i32.const 0x5a5a5a5a))
    (local.set 3 (i32.const 0x5a5a5a5a))
    (i32.add (local.get 1) (i32.add (local.get 2) (local.get 3))))
  (func $check (param i32) (result i32)
    (local i32 i32 i32)
    (block
      (br_if 0 (local.get 0))
      (local.set 1 (i32.const 7)))
    (local.get 1))
  (func (export "test") (param i32) (result i32)
    (drop (call $dirty (i32.const 0)))
    (call $check (local.get 0)))
)
(assert_return (invoke "test" (i32.const 1)) (i32.const 0))
(assert_return (invoke "test" (i32.const 0)) (i32.const 7))
