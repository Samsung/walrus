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
)

(assert_return (invoke "test1") (i32.const 2147483649) (i64.const 18446744071562067969) (i64.const 2147483649))
