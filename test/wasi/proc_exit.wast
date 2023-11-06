(module
  (import "wasi_snapshot_preview1" "proc_exit" (func $wasi_proc_exit (param i32)))

  (func (export "proc_exit")
    i32.const 0
    call $wasi_proc_exit
  )
)

(assert_return (invoke "proc_exit"))
