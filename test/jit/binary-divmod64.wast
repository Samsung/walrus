(module
(func (export "test1") (result i64 i64 i64 i64 i64) (local i64 i64 i64)
  i64.const 9876543210
  local.set 0
  i64.const 123456789123456
  local.set 1
  i64.const -1
  local.set 2

  i64.const 1234567812345678
  local.get 0
  i64.div_s

  local.get 1
  i64.const 567567567
  i64.rem_s

  i64.const -34567893456789
  local.get 0
  i64.div_s

  local.get 1
  local.get 0
  i64.rem_s

  i64.const -0x7fffffffffffffff
  local.get 2
  i64.div_s

  (; 124999, 59517183, 18446744073709548117, 9875541666, 9223372036854775807 ;)
)

(func (export "test2") (result i64 i64 i64 i64 i64) (local i64 i64 i64)
  i64.const 7654376543
  local.set 0
  i64.const 23456782345678
  local.set 1
  i64.const 1
  local.set 2

  i64.const 1234567812345678
  local.get 0
  i64.div_u

  local.get 1
  i64.const 87654321
  i64.rem_u

  i64.const 0xff00000000000000
  local.get 0
  i64.div_u

  local.get 1
  local.get 0
  i64.rem_u

  i64.const 0xffffffffffffffff
  local.get 2
  i64.div_s

  (; 161289, 47774473, 2400546455, 3772617926, 18446744073709551615 ;)
)

(func (export "test3") (result i64) (local i64)
  i64.const 0
  local.set 0

  i64.const 5
  local.get 0
  i64.div_s
  (; exception ;)
)

(func (export "test4") (result i64) (local i64)
  i64.const -1
  local.set 0

  i64.const 0x8000000000000000
  local.get 0
  i64.div_s
  (; exception ;)
)

(func (export "test5") (result i64) (local i64)
  i64.const 0
  local.set 0

  i64.const 5
  local.get 0
  i64.rem_s
  (; exception ;)
)

(func (export "test6") (result i64) (local i64)
  i64.const -1
  local.set 0

  i64.const 0x8000000000000000
  local.get 0
  i64.rem_s
  (; exception ;)
)

(func (export "test7") (result i64) (local i64)
  i64.const 0
  local.set 0

  i64.const 5
  local.get 0
  i64.div_u
  (; exception ;)
)

(func (export "test8") (result i64) (local i64)
  i64.const 0
  local.set 0

  i64.const 5
  local.get 0
  i64.rem_u
  (; exception ;)
)
)

(assert_return (invoke "test1") (i64.const 124999) (i64.const 59517183) (i64.const 18446744073709548117) (i64.const 9875541666) (i64.const 9223372036854775807))
(assert_return (invoke "test2") (i64.const 161289) (i64.const 47774473) (i64.const 2400546455) (i64.const 3772617926) (i64.const 18446744073709551615))
(assert_trap (invoke "test3") "integer divide by zero")
(assert_trap (invoke "test5") "integer divide by zero")
(assert_trap (invoke "test7") "integer divide by zero")
(assert_trap (invoke "test8") "integer divide by zero")

