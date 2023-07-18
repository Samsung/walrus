(module
  (memory 3)
  (func (export "func1")(result i32) 
    memory.size
  )

  (func (export "func2")(param i32)(result i32)
    local.get 0 
    memory.grow
  )
)
(assert_return (invoke "func1") (i32.const 3))
(assert_return (invoke "func2" (i32.const 5)) (i32.const 3))
(assert_return (invoke "func1") (i32.const 8))

(module
  (memory 0)
  (func (export "grow") (param i32) (result i32) (memory.grow (local.get 0)))
)
(assert_return (invoke "grow" (i32.const 0x10000)) (i32.const 0))
