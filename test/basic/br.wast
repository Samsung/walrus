(module
  (memory 7)
  (func (export "br0")(result i32 i32 i32)
    i32.const 1
    i32.const 2
    i32.const 3
    br 0
    i32.const 100 ;; the bytecode is not generated from here to function end
    drop
    drop
    drop
    memory.grow
  )
  (func (export "br_if0")(param i32)(result i32 i32 i32)
    i32.const 1 
    i32.const 2
    i32.const 3
    local.get 0
    br_if 0
    drop
    i32.const 100
    memory.grow
  )
  (func (export "check")(result i32)
    memory.size
  )
  (func (export "br0_1")(param i32)(result i32)
    local.get 0
    (if (then
      (i32.const 100)
      (br 1)
    ))
    i32.const 200
  )
)

(assert_return (invoke "br0") (i32.const 1)(i32.const 2)(i32.const 3))
(assert_return (invoke "br_if0"(i32.const 1)) (i32.const 1)(i32.const 2)(i32.const 3))
(assert_return (invoke "check") (i32.const 7))
(assert_return (invoke "br_if0"(i32.const 0)) (i32.const 1)(i32.const 2)(i32.const 7))
(assert_return (invoke "check") (i32.const 107))
(assert_return (invoke "br0_1"(i32.const 1))(i32.const 100))
(assert_return (invoke "br0_1"(i32.const 0))(i32.const 200))

