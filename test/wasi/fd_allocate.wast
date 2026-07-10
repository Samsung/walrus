(module
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open
      (param i32 i32 i32 i32 i32 i64 i64 i32 i32)
      (result i32)))

  (import "wasi_snapshot_preview1" "fd_allocate"
    (func $fd_allocate
      (param i32 i64 i64)
      (result i32)))

  (memory 1)
  (export "memory" (memory 0))

  ;; Store the test file path at memory offset 300.
  (data (i32.const 300) "./write_to_this.txt")

  (func (export "test_allocate") (result i32)
    (local $fd i32)
    (local $errno i32)

    i32.const 3       ;; First preopened directory.
    i32.const 0       ;; No lookup flags.
    i32.const 300     ;; Pointer to the file path.
    i32.const 19      ;; Length of "./write_to_this.txt".
    i32.const 0       ;; No open flags.
    i64.const 256     ;; Request FD_ALLOCATE right.
    i64.const 0       ;; No inheriting rights.
    i32.const 0       ;; No file descriptor flags.
    i32.const 0       ;; Store the opened file descriptor at memory[0].
    call $path_open

    local.set $errno

    ;; Return immediately if path_open failed.
    local.get $errno
    if
      local.get $errno
      return
    end

    ;; Load the opened file descriptor from memory[0].
    i32.const 0
    i32.load
    local.set $fd

    ;; Call fd_allocate(fd, offset = 0, len = 1).
    ;; The returned errno becomes the function result.
    local.get $fd
    i64.const 0
    i64.const 1
    call $fd_allocate
  )
)

;; fd_allocate should succeed and return errno 0.
(assert_return (invoke "test_allocate") (i32.const 0))

