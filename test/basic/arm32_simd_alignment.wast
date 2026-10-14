;; Bitwise SIMD uses SIMDValue<uint64_t, 2>, the type from the SIGBUS report.
(module
  (func (export "xor") (param i32 v128 v128) (result v128)
    local.get 1 local.get 2 v128.xor)
  (func (export "not") (param i32 v128) (result v128)
    local.get 1 v128.not))
(assert_return (invoke "xor" (i32.const 0)
  (v128.const i64x2 0xaaaaaaaaaaaaaaaa 0x5555555555555555)
  (v128.const i64x2 0x5555555555555555 0xaaaaaaaaaaaaaaaa))
  (v128.const i64x2 -1 -1))
(assert_return (invoke "not" (i32.const 0) (v128.const i64x2 0 -1))
  (v128.const i64x2 -1 0))
