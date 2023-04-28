(module
(func (export "test1") (result i64 i64 i64 i64 i64 i64) (local i64)
  i64.const 0
  local.set 0

  i64.const 0
  i64.clz

  local.get 0
  i64.ctz

  i64.const 0x800000000000
  local.tee 0
  i64.clz

  local.get 0
  i64.ctz

  i64.const 0x100
  local.tee 0
  i64.clz

  local.get 0
  i64.ctz
  (; 64, 64, 16, 47, 55, 8 ;)
)
)

(assert_return (invoke "test1") (i64.const 64) (i64.const 64) (i64.const 16) (i64.const 47) (i64.const 55) (i64.const 8))
