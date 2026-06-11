(module
  (func (export "test_block") (param $cond i32) (result i32)
    (local $x i32)

    (block $outer
      (block $inner
        (local.get $cond)
        (br_if $outer)
      )

      (local.set $x (i32.const 42))
    )

    (local.get $x)
  )

  (func (export "test_function_body_br") (param $cond i32) (result i32)
    (local $x i32)

    (block $outer
      (block $inner
        (local.get $x)
        (local.get $cond)
        (br_if 2)
        (drop)
      )

      (local.set $x (i32.const 42))
    )

    (local.get $x)
  )
)

(assert_return (invoke "test_block" (i32.const 0)) (i32.const 42))
(assert_return (invoke "test_block" (i32.const 1)) (i32.const 0))

(assert_return (invoke "test_function_body_br" (i32.const 0)) (i32.const 42))
(assert_return (invoke "test_function_body_br" (i32.const 1)) (i32.const 0))
