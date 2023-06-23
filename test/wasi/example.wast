(module
  (import "wasi_snapshot_preview1" "test" (func $test))
  (import "wasi_snapshot_preview1" "printI32" (func $print (param i32)))
  (import "wasi_snapshot_preview1" "writeI32" (func $write (result i32)))

  (func (export "start")
    call $test
    call $write
    call $print
  )
)


(assert_return (invoke "start"))
