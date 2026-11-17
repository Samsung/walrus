;; all_true checks whole lanes, not only their lowest byte.

(module
  (func (export "i16x8") (param v128) (result i32) (i16x8.all_true (local.get 0)))
  (func (export "i32x4") (param v128) (result i32) (i32x4.all_true (local.get 0)))
  (func (export "i64x2") (param v128) (result i32) (i64x2.all_true (local.get 0)))
  (func (export "i32x4_const") (result i32)
    (i32x4.all_true (v128.const i32x4 0x01010100 0x01010101 0x00000100 0x01000000)))
)

(assert_return (invoke "i16x8" (v128.const i16x8 0x100 1 1 1 1 1 1 0x8000)) (i32.const 1))
(assert_return (invoke "i16x8" (v128.const i16x8 0x100 1 1 1 1 1 1 0)) (i32.const 0))
(assert_return (invoke "i32x4" (v128.const i32x4 0x100 0x10000 0x1000000 1)) (i32.const 1))
(assert_return (invoke "i32x4" (v128.const i32x4 0x100 0x10000 0 1)) (i32.const 0))
(assert_return (invoke "i64x2" (v128.const i64x2 0x100 0x100000000)) (i32.const 1))
(assert_return (invoke "i64x2" (v128.const i64x2 0x8000000000000000 0x1000000000000)) (i32.const 1))
(assert_return (invoke "i64x2" (v128.const i64x2 0x8000000000000000 0)) (i32.const 0))
(assert_return (invoke "i32x4_const") (i32.const 1))
