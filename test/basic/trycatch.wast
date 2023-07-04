(module
  (tag $e0)
  (tag $e1)
  (func (export "sss") (param i32)(result i32)
    (try
      (do
        (local.get 0)
        (i32.const 1)
        (if (i32.eq)
          (then (throw $e0) )
        )
      )
      (catch $e0
         (i32.const 100)
         return
      )
    )
    (try
      (do
        (local.get 0)
        (i32.const 2)
        (if (i32.eq)
          (then (throw $e1) )
        )
      )
      (catch $e1
         (i32.const 200)
         return
      )
    )
    (i32.const 300)
  )

  (func (export "dead-code")(result i32)
    (block (result i32)
      i32.const 5

      (try (param i32) (result i32)
        (do
           br 1
        )
        (catch $e0
           i32.const 1
           br 1
        )
        (catch_all
           i32.const 2
           br 1
        )
      )

      i32.const 6
      i32.add
    )
  )
)

(assert_return (invoke "sss" (i32.const 1))(i32.const 100))
(assert_return (invoke "sss" (i32.const 2))(i32.const 200))
(assert_return (invoke "sss" (i32.const 3))(i32.const 300))
(assert_return (invoke "dead-code")(i32.const 5))
