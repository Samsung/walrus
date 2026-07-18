(module
  (import "wasi_snapshot_preview1" "path_link"
    (func $path_link
      (param i32 i32 i32 i32 i32 i32 i32)
      (result i32)))

  (memory 1)
  (export "memory" (memory 0))
  (export "create_link" (func $create_link))

  (data (i32.const 300) "text.txt")
  (data (i32.const 400) "linked.txt")

  (func $create_link (result i32)
    i32.const 3
    i32.const 0
    i32.const 300
    i32.const 8
    i32.const 3
    i32.const 400
    i32.const 10
    call $path_link
  )
)

(assert_return
  (invoke "create_link")
  (i32.const 0)
)
