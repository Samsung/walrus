(module
(func (export "test1") (result i32 i64 i64) (local i64 i32)
  i64.const 0x7fffffff80000001
  local.set 0

  i32.const 0x80000001
  local.set 1

  local.get 0
  i32.wrap_i64

  local.get 1
  i64.extend_i32_s

  local.get 1
  i64.extend_i32_u

  (; 2147483649, 18446744071562067969, 2147483649 ;)
)

(func (export "test2") (result i32)
  f32.const 1234
  i32.trunc_f32_u
)

(func (export "test3") (result i32)
  f32.const 4567
  i32.trunc_sat_f32_u
)

(func (export "test4") (param i32) (result f32 i32)
  local.get 0
  f32.reinterpret_i32
  i32.const 0xaaaa
  i32.popcnt
)
)

(assert_return (invoke "test1") (i32.const 2147483649) (i64.const 18446744071562067969) (i64.const 2147483649))
(assert_return (invoke "test2") (i32.const 1234))
(assert_return (invoke "test3") (i32.const 4567))
(assert_return (invoke "test4" (i32.const 0x45580000)) (f32.const 3456.0) (i32.const 8))
