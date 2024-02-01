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
(assert_return (invoke "grow" (i32.const 0x1000)) (i32.const 0))
(assert_return (invoke "grow" (i32.const 0x10000)) (i32.const -1))

(module
  (memory 1)
  (func (export "loadInt64") (param $offset i32) (result i64)
    (i64.load (local.get $offset))
  )
  (func (export "storeInt64") (param $offset i32) (param $value i64)
    (i64.store (local.get $offset) (local.get $value))
  )
  (func (export "loadDouble") (param $offset i32) (result f64)
    (f64.load (local.get $offset))
  )
  (func (export "storeDouble") (param $offset i32) (param $value f64)
    (f64.store (local.get $offset) (local.get $value))
  )
  (func (export "loadUint8") (param $offset i32) (result i32)
    (i32.load8_u (local.get $offset))
  )
)
;; memory is 64 kilobyte aligned, therefore index 1 will not be four byte aligned.
(assert_return (invoke "storeInt64" (i32.const 0x1) (i64.const 0x0123456789abcdef)))
(assert_return (invoke "loadInt64" (i32.const 0x1)) (i64.const 0x0123456789abcdef))
(assert_return (invoke "loadUint8" (i32.const 0x1)) (i32.const 0xef))
(assert_return (invoke "storeDouble" (i32.const 0x1) (f64.const 0xfedcba9876543210)))
(assert_return (invoke "loadDouble" (i32.const 0x1)) (f64.const 0xfedcba9876543210))
