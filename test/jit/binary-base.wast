(module
(func (export "test1") (result i64 i64) (local i64)
  i64.const 020030040050060070
  local.set 0
  i64.const 100200300400500600
  local.get 0
  i64.add
  local.tee 0
  i64.const 220330440550660770
  local.get 0
  i64.sub

  (; 120230340450560670, 100100100100100100 ;)
)
)

(assert_return (invoke "test1") (i64.const 120230340450560670) (i64.const 100100100100100100))
