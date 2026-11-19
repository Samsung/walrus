;; The result of i64x2.abs stays in memory across a call, so it is
;; computed into a register other than its operand.

(module
  (data $d "\01")
  (func $nop)
  (func (export "abs_data_drop") (param v128) (result v128)
    (i64x2.abs (local.get 0))
    (data.drop $d))
  (func (export "abs_call") (param v128) (result v128)
    (i64x2.abs (local.get 0))
    (call $nop))
)

(assert_return (invoke "abs_data_drop" (v128.const i64x2 -5 -7)) (v128.const i64x2 5 7))
(assert_return (invoke "abs_data_drop" (v128.const i64x2 5 -7)) (v128.const i64x2 5 7))
(assert_return (invoke "abs_call" (v128.const i64x2 -5 7)) (v128.const i64x2 5 7))
(assert_return (invoke "abs_call" (v128.const i64x2 0x8000000000000000 -1)) (v128.const i64x2 0x8000000000000000 1))
