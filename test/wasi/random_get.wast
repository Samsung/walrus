(module
  (import "wasi_snapshot_preview1" "random_get" (func $random (param i32 i32) (result i32)))
  (memory 1)
  (export "_start" (func $_start))
  (func $_start (result i32)
    (call $random (i32.const 10) (i32.const 5))
  )
)

(assert_return (invoke "_start") (i32.const 0))
