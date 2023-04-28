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

(func (export "test3") (result f32) (local f32 f32)
    f32.const 3.3
    local.set 0
    f32.const 4.4
    local.set 1

    f32.const 5.5
    f32.const 6.6
    local.get 0
    local.get 1
    f32.eq
    select (result f32)
)

(func (export "test4") (result i32) (local f32 f32)
    f32.const 3.3
    local.set 0
    f32.const 4.4
    local.set 1

    i32.const 5
    i32.const 6
    local.get 0
    local.get 1
    f32.eq

    select
)

(func (export "test5") (result f64) (local f64 f64)
    f64.const 3.3
    local.set 0
    f64.const 4.4
    local.set 1

    f64.const 5.5
    f64.const 6.6
    local.get 0
    local.get 1
    f64.eq
    select (result f64)
)

(func (export "test6") (result i32) (local f64 f64)
    f64.const 3.3
    local.set 0
    f64.const 4.4
    local.set 1

    i32.const 5
    i32.const 6
    local.get 0
    local.get 1
    f64.eq

    select
)
)

(assert_return (invoke "test1") (i32.const 5) (i32.const 267386880))
(assert_return (invoke "test2") (i64.const 5) (i64.const 280379743272960))
(assert_return (invoke "test3") (f32.const 6.6))
(assert_return (invoke "test4") (i32.const 6))
(assert_return (invoke "test5") (f64.const 6.6))
(assert_return (invoke "test6") (i32.const 6))
