(module
  (import "wasi_snapshot_preview1" "proc_exit" (func $exit (param i32)))


  (func (export "start")
    i32.const 0
    call $exit
  )
)

(assert_return (invoke "start"))
