(module
  (import "wasi_snapshot_preview1" "path_symlink"
    (func $path_symlink
      (param i32 i32 i32 i32 i32)
      (result i32)))

  (import "wasi_snapshot_preview1" "path_unlink_file"
    (func $path_unlink_file
      (param i32 i32 i32)
      (result i32)))

  (memory 1)
  (export "memory" (memory 0))
  (export "create_symlink" (func $create_symlink))
  (export "remove_symlink" (func $remove_symlink))

  (data (i32.const 300) "text.txt")
  (data (i32.const 400) "symlink.txt")

  (func $create_symlink (result i32)
    i32.const 300
    i32.const 8
    i32.const 3
    i32.const 400
    i32.const 11
    call $path_symlink
  )

  (func $remove_symlink (result i32)
    i32.const 3
    i32.const 400
    i32.const 11
    call $path_unlink_file
  )
)

(assert_return
  (invoke "create_symlink")
  (i32.const 0)
)

(assert_return
  (invoke "remove_symlink")
  (i32.const 0)
)