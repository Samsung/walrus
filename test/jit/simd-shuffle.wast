(module

(func (export "test1") (param v128 v128) (result v128)
  local.get 0

  local.get 1
  local.get 1
  i64x2.add

  local.get 1
  v128.not
  local.set 1

  i8x16.shuffle 31 15 29 13 27 11 25 9 23 7 21 5 19 3 17 1
)

(func (export "test2") (param v128 v128) (result v128) (local v128)
  local.get 1
  local.get 0

  local.get 1
  v128.not
  local.set 2

  i8x16.shuffle 30 14 28 12 26 10 24 8 22 6 20 4 18 2 16 0

  local.get 2
  v128.not
  local.set 2
)
)

(assert_return (invoke "test1"
   (v128.const i64x2 0x0807060504030201 0x100f0e0d0c0b0a09)
   (v128.const i64x2 0x1817161514131211 0x201f1e1d1c1b1a19))
   (v128.const i64x2 0x0a340c380e3c1040 0x02240428062c0830))

(assert_return (invoke "test2"
   (v128.const i64x2 0x0807060504030201 0x100f0e0d0c0b0a09)
   (v128.const i64x2 0x1817161514131211 0x201f1e1d1c1b1a19))
   (v128.const i64x2 0x19091b0b1d0d1f0f 0x1101130315051707))
