(module
  (import "wasi_snapshot_preview1" "fd_write" (func $wasi_fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "environ_get" (func $wasi_environ_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "environ_sizes_get" (func $wasi_environ_sizes_get (param i32 i32) (result i32)))
  (memory 1)
  
  (export "memory" (memory 0))
  (export "environ" (func $environ))
  (data (i32.const 100) "500" )

  (func $environ
  (local $i i32)
    i32.const 0 ;; environment variables count
    i32.const 12 ;; environment variables overall size in characters
    call $wasi_environ_sizes_get
    drop

    i32.const 500 ;; envp
    i32.const 500 ;; envp[0]
    call $wasi_environ_get
    drop

    ;; Memory + 50 = 500, start of output string.
    i32.const 50
    i32.const 500
    i32.store

    ;; Memory + 54 = size of output string.
    i32.const 54
    i32.const 12
    i32.load
    i32.store
    
    ;; Replace '\0' with '\n' for readable printing.
    i32.const 0
    local.set $i
    (loop $loop
      i32.const 500
      local.get $i
      i32.add
      i32.load8_u

      i32.eqz
      (if
        (then
          i32.const 500
          local.get $i
          i32.add
          i32.const 10
          i32.store8
        )
      )

      local.get $i
      i32.const 1
      i32.add
      local.tee $i

      i32.const 12
      i32.load
      i32.lt_u
      br_if $loop
    )

    (call $wasi_fd_write
          (i32.const 1)  ;;file descriptor
          (i32.const 50) ;;offset of str offset
          (i32.const 1)  ;;iovec length
          (i32.const 200) ;;result offset
    )
    drop
  )
)

(assert_return (invoke "environ"))
