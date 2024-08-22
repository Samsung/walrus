(module
  (func $useless_locals (export "useless_locals")(param i32)(result i32)
      (local i32 i32)
      i32.const 42
  )
)

(assert_return (invoke "useless_locals" (i32.const 222)) (i32.const 42))
