(module
(memory 2)

(func (export "memSize") (result i32)
  memory.size
)

(func (export "memGrow") (param i32) (result i32)
  local.get 0
  memory.grow
)
)

(assert_return (invoke "memSize") (i32.const 2))
(assert_return (invoke "memGrow" (i32.const 3)) (i32.const 2))
(assert_return (invoke "memSize") (i32.const 5))
