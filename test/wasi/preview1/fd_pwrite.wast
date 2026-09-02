(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_pwrite" (func $fd_pwrite (param i32 i32 i32 i64 i32) (result i32)))

  (memory 1)
  
  (export "memory" (memory 0))

  ;; File path.
  (data (i32.const 300) "./write_to_this.txt")

  ;; Data to write.
  (data (i32.const 400) "hello")

  (func (export "test_pwrite") (result i32)
    (local $fd i32)
    (local $errno i32)

    ;; Open the test file.
    i32.const 3       ;; First preopened directory.
    i32.const 0       ;; No lookup flags.
    i32.const 300     ;; Pointer to the file path.
    i32.const 19      ;; Length of "./write_to_this.txt".
    i32.const 0       ;; No open flags.
    i64.const 68      
    i64.const 0      
    i32.const 0       ;; No file descriptor flags.
    i32.const 0       ;; Store the opened fd at memory[0].
    call $path_open

    local.set $errno

    ;; Return immediately if path_open failed.
    local.get $errno
    if
      local.get $errno
      return
    end

    ;; Load the opened file descriptor.
    i32.const 0
    i32.load
    local.set $fd

    ;; Create one ciovec at memory[100].
    ;;
    ;; ciovec.buf     = 400
    ;; ciovec.buf_len = 5
    i32.const 100
    i32.const 400
    i32.store

    i32.const 104
    i32.const 5
    i32.store

    ;; fd_pwrite(
    ;;   fd,
    ;;   iovs = 100,
    ;;   iovs_len = 1,
    ;;   offset = 0,
    ;;   nwritten = 200
    ;; )
    local.get $fd
    i32.const 100
    i32.const 1
    i64.const 0
    i32.const 200
    call $fd_pwrite

    local.set $errno

    ;; Return the errno if fd_pwrite failed.
    local.get $errno
    if
      local.get $errno
      return
    end

    ;; Verify that exactly 5 bytes were written.
    i32.const 200
    i32.load
    i32.const 5
    i32.ne

    if
      i32.const 1
      return
    end

    ;; Success.
    i32.const 0
  )
)

(assert_return (invoke "test_pwrite") (i32.const 0))
