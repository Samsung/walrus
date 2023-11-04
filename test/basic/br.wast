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
  (func (export "br_if_cmp")(param i32 i32 i64 f32 f64)(result i32 i32 i32 i32)
    block (result i32)
      i32.const -1
      local.get 0
      br_if 0
      drop
      local.get 1
      i32.const 100
      i32.eq
    end
    block (result i32)
      i32.const -1
      local.get 0
      br_if 0
      drop
      local.get 2
      i64.const 100
      i64.eq
    end
    block (result i32)
      i32.const -1
      local.get 0
      br_if 0
      drop
      local.get 3
      f32.const 100.0
      f32.eq
    end
    block (result i32)
      i32.const -1
      local.get 0
      br_if 0
      drop
      local.get 4
      f64.const 100.0
      f64.eq
    end
  )
)

(assert_return (invoke "br0") (i32.const 1)(i32.const 2)(i32.const 3))
(assert_return (invoke "br_if0"(i32.const 1)) (i32.const 1)(i32.const 2)(i32.const 3))
(assert_return (invoke "check") (i32.const 7))
(assert_return (invoke "br_if0"(i32.const 0)) (i32.const 1)(i32.const 2)(i32.const 7))
(assert_return (invoke "check") (i32.const 107))
(assert_return (invoke "br0_1"(i32.const 1))(i32.const 100))
(assert_return (invoke "br0_1"(i32.const 0))(i32.const 200))
(assert_return (invoke "br_if_cmp"(i32.const 0)(i32.const 100)(i64.const 100)(f32.const 100.0)(f64.const 100.0))
   (i32.const 1)(i32.const 1)(i32.const 1)(i32.const 1))
