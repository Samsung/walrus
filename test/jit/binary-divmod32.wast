(module
(func (export "test1") (result i32 i32 i32 i32 i32) (local i32 i32 i32)
  i32.const 4321
  local.set 0
  i32.const 1234567
  local.set 1
  i32.const -1
  local.set 2

  i32.const 1234567
  local.get 0
  i32.div_s

  local.get 1
  i32.const 5432
  i32.rem_s

  i32.const -1234567
  local.get 0
  i32.div_s

  local.get 1
  local.get 0
  i32.rem_s

  i32.const -0x7fffffff
  local.get 2
  i32.div_s

  (; 285, 1503, 4294967011, 3082, 2147483647 ;)
)

(func (export "test2") (result i32 i32 i32 i32 i32) (local i32 i32 i32)
  i32.const 4321
  local.set 0
  i32.const 1234567
  local.set 1
  i32.const 1
  local.set 2

  i32.const 1234567
  local.get 0
  i32.div_u

  local.get 1
  i32.const 5432
  i32.rem_u

  i32.const 0xff000000
  local.get 0
  i32.div_u

  local.get 1
  local.get 0
  i32.rem_u

  i32.const 0xffffffff
  local.get 2
  i32.div_s

  (; 285, 1503, 990092, 3082, 4294967295 ;)
)

(func (export "test3") (result i32) (local i32)
  i32.const 0
  local.set 0

  i32.const 5
  local.get 0
  i32.div_s
  (; exception ;)
)

(func (export "test4") (result i32) (local i32)
  i32.const -1
  local.set 0

  i32.const 0x80000000
  local.get 0
  i32.div_s
  (; exception ;)
)

(func (export "test5") (result i32) (local i32)
  i32.const 0
  local.set 0

  i32.const 5
  local.get 0
  i32.rem_s
  (; exception ;)
)

(func (export "test6") (result i32) (local i32)
  i32.const -1
  local.set 0

  i32.const 0x80000000
  local.get 0
  i32.rem_s
  (; exception ;)
)

(func (export "test7") (result i32) (local i32)
  i32.const 0
  local.set 0

  i32.const 5
  local.get 0
  i32.div_u
  (; exception ;)
)

(func (export "test8") (result i32) (local i32)
  i32.const 0
  local.set 0

  i32.const 5
  local.get 0
  i32.rem_u
  (; exception ;)
)
)

(assert_return (invoke "test1") (i32.const 285) (i32.const 1503) (i32.const 4294967011) (i32.const 3082) (i32.const 2147483647))
(assert_return (invoke "test2") (i32.const 285) (i32.const 1503) (i32.const 990092) (i32.const 3082) (i32.const 4294967295))
(assert_trap (invoke "test3") "integer divide by zero")
(assert_trap (invoke "test5") "integer divide by zero")
(assert_trap (invoke "test7") "integer divide by zero")
(assert_trap (invoke "test8") "integer divide by zero")

