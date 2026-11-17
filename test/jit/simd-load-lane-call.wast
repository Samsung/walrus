;; The result of a lane load stays in memory across a call, so the
;; vector operand must be copied to another register first.

(module
  (memory 1)
  (data (i32.const 0) "\11\22\33\44\55\66\77\88")
  (func $nop)
  (func (export "load8") (param v128) (result v128)
    (v128.load8_lane 1 (i32.const 0) (local.get 0))
    (call $nop))
  (func (export "load16") (param v128) (result v128)
    (v128.load16_lane 1 (i32.const 0) (local.get 0))
    (call $nop))
  (func (export "load32") (param v128) (result v128)
    (v128.load32_lane 1 (i32.const 0) (local.get 0))
    (call $nop))
  (func (export "load64") (param v128) (result v128)
    (v128.load64_lane 1 (i32.const 0) (local.get 0))
    (call $nop))
)

(assert_return (invoke "load8" (v128.const i8x16 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16))
  (v128.const i8x16 1 0x11 3 4 5 6 7 8 9 10 11 12 13 14 15 16))
(assert_return (invoke "load16" (v128.const i16x8 1 2 3 4 5 6 7 8))
  (v128.const i16x8 1 0x2211 3 4 5 6 7 8))
(assert_return (invoke "load32" (v128.const i32x4 1 2 3 4))
  (v128.const i32x4 1 0x44332211 3 4))
(assert_return (invoke "load64" (v128.const i64x2 1 2))
  (v128.const i64x2 1 0x8877665544332211))
