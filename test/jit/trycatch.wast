(module
  (tag $except0 (param i32 i64 f32 f64))

  (func $throw1 (param i64 f64)
     local.get 0
     i32.wrap_i64
     local.get 0
     i64.const 10
     i64.add

     local.get 1
     f32.demote_f64
     local.get 1
     f64.const 10
     f64.sub

     throw $except0
  )

  (func $throw2
     i32.const 1234
     i64.const 4321
     f32.const 6789.5
     f64.const -9876.75

     throw $except0
  )

  (func (export "try1") (param i64 f64) (result i32 i64 f32 f64)
    (try
      (do
         local.get 0
         local.get 1
         call $throw1
      )
      (catch $except0
         return
      )
    )

    i32.const 0
    i64.const 0
    f32.const 0
    f64.const 0
  )

  (func (export "try2") (result i32 i64 f32 f64)
    (try
      (do
         call $throw2
      )
      (catch $except0
         return
      )
    )

    i32.const 0
    i64.const 0
    f32.const 0
    f64.const 0
  )
)

(assert_return (invoke "try1" (i64.const 1234567) (f64.const 123456.5))
    (i32.const 1234567) (i64.const 1234577) (f32.const 123456.5) (f64.const 123446.5))
(assert_return (invoke "try2") (i32.const 1234) (i64.const 4321) (f32.const 6789.5) (f64.const -9876.75))
