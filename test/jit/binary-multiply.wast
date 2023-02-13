(module

(func (export "test1") (result i64 i64 i64) (local i64)
  i64.const 10000000
  local.set 0

  i64.const 10000000
  local.get 0
  i64.mul

  i64.const -10000000
  local.get 0
  i64.mul

  i64.const -10000000
  local.set 0

  local.get 0
  local.get 0
  i64.mul

  (; 100000000000000, 18446644073709551616, 100000000000000 ;)
)

(func (export "test2") (result i32 i32 i32) (local i32)
  i32.const 10000
  local.set 0

  i32.const 10000
  local.get 0
  i32.mul

  i32.const -10000
  local.get 0
  i32.mul

  i32.const -10000
  local.set 0

  local.get 0
  local.get 0
  i32.mul

  (; 100000000, 4194967296, 100000000 ;)
)
)

(assert_return (invoke "test1") (i64.const 100000000000000) (i64.const 18446644073709551616) (i64.const 100000000000000))
(assert_return (invoke "test2") (i32.const 100000000) (i32.const 4194967296) (i32.const 100000000))
