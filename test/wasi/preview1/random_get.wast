(module
  (import "wasi_snapshot_preview1" "random_get" (func $random (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
  (memory 1)
  (export "_start" (func $_start))
  (func $_start
    (call $random (i32.const 10) (i32.const 5))
    i32.eqz
    (if
      (then
        i32.const 0
        call $proc_exit
      )
      (else
        i32.const 1
        call $proc_exit
      )
    )
  )
)

(assert_return (invoke "_start"))
