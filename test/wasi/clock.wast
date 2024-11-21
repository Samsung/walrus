(module
  (import "wasi_snapshot_preview1" "clock_res_get" (func $clock_res_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "clock_time_get" (func $clock_time_get (param i32 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
  (memory 1)
  (export "_start" (func $_start))
  (func $_start
    (call $clock_res_get (i32.const 0) (i32.const 0))
    (if
      (then
        i32.const 1
        call $proc_exit
      )
    )

    (call $clock_time_get (i32.const 0) (i64.const 1000000) (i32.const 0))
    i32.eqz
    i32.eqz
    call $proc_exit
  )
)

(assert_return (invoke "_start"))
