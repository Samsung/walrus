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
