(module
(func $f1 (param i64 i64) (result i64 i64) (local i64)
  local.get 0
  i64.const 0x32100000123
  i64.add

  local.get 1
  i64.const 0x2100000012
  i64.sub
)

(func (export "test1") (result i64 i64)
  i64.const 0x23400000234
  i64.const 0x23400000234
  call $f1
  (; 5862630359895, 2280627634722 ;)
)
)

(assert_return (invoke "test1") (i64.const 5862630359895) (i64.const 2280627634722))
