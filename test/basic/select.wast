(module
  (func (export "func1") (param i32) (result i32)
    (i32.eqz (select (i32.const 0) (i32.const 1) (local.get 0)))
  )

  (func (export "func2") (param i32) (result i32)
    (i32.mul
      (select (i32.const 1) (i32.const 2) (local.get 0))
      (select (i32.const 1) (i32.const 2) (local.get 0))
    )
  )
)

(assert_return (invoke "func1" (i32.const 0)) (i32.const 0))
(assert_return (invoke "func1" (i32.const 1)) (i32.const 1))
(assert_return (invoke "func2" (i32.const 0)) (i32.const 4))
(assert_return (invoke "func2" (i32.const 1)) (i32.const 1))
