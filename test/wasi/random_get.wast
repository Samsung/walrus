(module
  (import "wasi_snapshot_preview1" "random_get" (func $random (param i32 i32) (result i32)))
  (memory 1)
  (export "_start" (func $_start))
  (func $_start
    (call $random (i32.const 10) (i32.const 5))
    drop
  )
)

(assert_return (invoke "_start"))
