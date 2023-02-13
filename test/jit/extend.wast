(module
(func (export "test1") (result i32 i32 i32 i32) (local i32 i32 i32 i32)
  i32.const 0x8880
  local.set 0

  i32.const 0xae7f
  local.set 1

  i32.const 0x228000
  local.set 2

  i32.const 0xae7fff
  local.set 3

  local.get 0
  i32.extend8_s

  local.get 1
  i32.extend8_s

  local.get 2
  i32.extend16_s

  local.get 3
  i32.extend16_s

  (; 4294967168, 127, 4294934528, 32767 ;)
)

(func (export "test2") (result i64 i64 i64 i64) (local i64 i64 i64 i64)
  i64.const 0x8880
  local.set 0

  i64.const 0xae7f
  local.set 1

  i64.const 0x228000
  local.set 2

  i64.const 0xae7fff
  local.set 3

  local.get 0
  i64.extend8_s

  local.get 1
  i64.extend8_s

  local.get 2
  i64.extend16_s

  local.get 3
  i64.extend16_s

  (; 18446744073709551488, 127, 18446744073709518848, 32767 ;)
)

(func (export "test3") (result i64 i64) (local i64 i64)
  i64.const 0x8880000000
  local.set 0

  i64.const 0xae7fffffff
  local.set 1

  local.get 0
  i64.extend32_s

  local.get 1
  i64.extend32_s

  (; 18446744071562067968, 2147483647 ;)
)
)

(assert_return (invoke "test1") (i32.const 4294967168) (i32.const 127) (i32.const 4294934528) (i32.const 32767))
(assert_return (invoke "test2") (i64.const 18446744073709551488) (i64.const 127) (i64.const 18446744073709518848) (i64.const 32767))
(assert_return (invoke "test3") (i64.const 18446744071562067968) (i64.const 2147483647))
