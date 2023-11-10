(module
  (memory 3)
  (func (export "func1")(result i32) 
    memory.size
  )

  (func (export "func2")(param i32)(result i32)
    local.get 0 
    memory.grow
  )

  (func (export "load32") (param i32) (result i32)
    local.get 0
    i32.load
  )

  (func (export "load64") (param i32) (result i64)
    local.get 0
    i64.load
  )

  (func (export "store32") (param i32 i32)
    local.get 0
    local.get 1
    i32.store
  )

  (func (export "store64") (param i32 i64)
    local.get 0
    local.get 1
    i64.store
  )
)
(assert_return (invoke "func1") (i32.const 3))
(assert_return (invoke "func2" (i32.const 5)) (i32.const 3))
(assert_return (invoke "func1") (i32.const 8))

;; unaligned store and endianness
(invoke "store64" (i32.const 1) (i64.const 0xdeadcafebeefbabe))
(assert_return (invoke "load64" (i32.const 1)) (i64.const 0xdeadcafebeefbabe))
(assert_return (invoke "load32" (i32.const 1)) (i32.const 0xbeefbabe))
(assert_return (invoke "load32" (i32.const 5)) (i32.const 0xdeadcafe))

(module
  (memory 0)
  (func (export "grow") (param i32) (result i32) (memory.grow (local.get 0)))
)
(assert_return (invoke "grow" (i32.const 0x1000)) (i32.const 0))
(assert_return (invoke "grow" (i32.const 0x10000)) (i32.const -1))
