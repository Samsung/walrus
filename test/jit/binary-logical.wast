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

(func (export "test2") (param i32 i32) (result i32)
  block
    local.get 0
    local.get 1
    i32.and
    br_if 0
    i32.const 5
    return
  end

  i32.const 6
)

(func (export "test3") (param i32 i32) (result i32)
  i32.const 10
  i32.const 11

  local.get 0
  local.get 1
  i32.and
  select
)
)

(assert_return (invoke "test1") (i64.const 1080880403494997760) (i64.const 18442521884633399280) (i64.const 17361641481138401520))
(assert_return (invoke "test2" (i32.const 0x100) (i32.const 0x200)) (i32.const 5))
(assert_return (invoke "test2" (i32.const 0x0ff0) (i32.const 0x1f)) (i32.const 6))
(assert_return (invoke "test3" (i32.const 0xff) (i32.const 0xff00)) (i32.const 11))
(assert_return (invoke "test3" (i32.const 0x1ff) (i32.const 0xff00)) (i32.const 10))
