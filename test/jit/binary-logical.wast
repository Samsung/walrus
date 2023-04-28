(module
(func (export "test1") (result i64 i64 i64) (local i64 i64)
  i64.const 0xff00ff00ff00ff00
  local.set 0
  i64.const 0x0ff00ff00ff00ff0
  local.set 1

  local.get 0
  i64.const 0x0ff00ff00ff00ff0
  i64.and

  i64.const 0xff00ff00ff00ff00
  local.get 1
  i64.or

  local.get 0
  local.get 1
  i64.xor

  (; 1080880403494997760, 18442521884633399280, 17361641481138401520 ;)
)
)

(assert_return (invoke "test1") (i64.const 1080880403494997760) (i64.const 18442521884633399280) (i64.const 17361641481138401520))
