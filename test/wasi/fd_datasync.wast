(module
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write"
    (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_datasync"
    (func $fd_datasync (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close (param i32) (result i32)))

  (memory 1)
  (data (i32.const 200) "datasync test\n")      ;; 14 bytes, written to file
  (data (i32.const 300) "./write_to_this.txt")  ;; 19 bytes, file path

  ;; Open file, write data, call fd_datasync, close. Returns errno of fd_datasync.
  (func (export "test_datasync") (result i32)
    ;; --- path_open ---
    ;; fd=3 (first preopened dir), lookupflags=1, path@300 len=19
    ;; oflags=9 (creat|trunc), rights=8257 (path_open|fd_write|fd_datasync)
    i32.const 3
    i32.const 1
    i32.const 300
    i32.const 19
    i32.const 9
    i64.const 8257   ;; fd_datasync(1) | fd_write(64) | path_open(8192)
    i64.const 8257
    i32.const 0
    i32.const 0      ;; store opened fd at memory[0]
    call $path_open
    i32.eqz
    (if (then) (else i32.const 1 return))  ;; abort if open failed

    ;; --- set up iovec at offset 500: (buf=200, len=14) ---
    i32.const 500
    i32.const 200
    i32.store
    i32.const 504
    i32.const 14
    i32.store

    ;; --- fd_write ---
    i32.const 0
    i32.load         ;; fd
    i32.const 500    ;; iovec array
    i32.const 1      ;; iovec count
    i32.const 600    ;; nwritten output
    call $fd_write
    drop

    ;; --- fd_datasync: test target ---
    i32.const 0
    i32.load         ;; fd
    call $fd_datasync
    ;; stash errno at memory[700] so we can close before returning
    i32.const 700
    i32.store

    ;; --- fd_close ---
    i32.const 0
    i32.load
    call $fd_close
    drop

    i32.const 700
    i32.load         ;; return stashed errno
  )

  ;; Call fd_datasync with an invalid fd. Expects errno::badf (8).
  (func (export "test_datasync_invalid_fd") (result i32)
    i32.const 99     ;; non-existent fd
    call $fd_datasync
  )

  (export "memory" (memory 0))
)

;; Valid fd: fd_datasync should succeed (errno::success = 0)
(assert_return (invoke "test_datasync") (i32.const 0))

;; Invalid fd: should return errno::badf (8)
(assert_return (invoke "test_datasync_invalid_fd") (i32.const 8))
