(module
  (import "wasi_snapshot_preview1" "proc_raise" (func $wasi_proc_raise (param i32) (result i32)))

  (func (export "proc_raise") (result i32)
    i32.const 0 ;; illegal signal ID
    call $wasi_proc_raise
  )
)

(assert_return (invoke "proc_raise") (i32.const 52))
