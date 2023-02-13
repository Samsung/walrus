(module
(func (export "test1") (result i32 i32) (local i32 i32)
  i32.const 0xff00000
  local.set 0
  i32.const 16
  local.set 1

  i32.const 5
  local.get 0
  local.get 1
  select

  i32.const 10
  local.get 0
  local.get 1
  i32.const 14
  i32.eq
  select

  (; 5, 267386880 ;)
)

(func (export "test2") (result i64 i64) (local i64 i32)
  i64.const 0xff00ff000000
  local.set 0
  i32.const 16
  local.set 1

  i64.const 5
  local.get 0
  local.get 1
  select

  local.get 0
  i64.const 10
  local.get 1
  i32.const 16
  i32.eq
  select

  (; 5, 280379743272960 ;)
)
)

(assert_return (invoke "test1") (i32.const 5) (i32.const 267386880))
(assert_return (invoke "test2") (i64.const 5) (i64.const 280379743272960))
