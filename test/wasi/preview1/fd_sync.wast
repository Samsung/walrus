(module
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write"
    (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_sync"
    (func $fd_sync (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close (param i32) (result i32)))

  (memory 1)
  (data (i32.const 200) "fd_sync test\n")       ;; 13 bytes, written to file
  (data (i32.const 300) "./write_to_this.txt")  ;; 19 bytes, file path

  ;; Open file, write data, call fd_sync, close. Returns errno of fd_sync.
  (func (export "test_sync") (result i32)
    ;; --- path_open ---
    i32.const 3
    i32.const 1
    i32.const 300
    i32.const 19
    i32.const 9       ;; oflags: creat|trunc
    i64.const 8320    ;; fd_sync(64) | fd_write(64) | path_open(8192) = 8320
    i64.const 8320
    i32.const 0
    i32.const 0       ;; store opened fd at memory[0]
    call $path_open
    i32.eqz
    (if (then) (else i32.const 1 return))  ;; abort if open failed

    ;; --- set up iovec at offset 500: (buf=200, len=13) ---
    i32.const 500
    i32.const 200
    i32.store
    i32.const 504
    i32.const 13
    i32.store

    ;; --- fd_write ---
    i32.const 0
    i32.load          ;; fd
    i32.const 500     ;; iovec array
    i32.const 1       ;; iovec count
    i32.const 600     ;; nwritten output
    call $fd_write
    drop

    ;; --- fd_sync: test target ---
    i32.const 0
    i32.load          ;; fd
    call $fd_sync
    ;; stash errno at memory[700] so we can close before returning
    i32.const 700
    i32.store

    ;; --- fd_close ---
    i32.const 0
    i32.load
    call $fd_close
    drop

    i32.const 700
    i32.load          ;; return stashed errno
  )

  ;; Call fd_sync with an invalid fd. Expects errno::badf (8).
  (func (export "test_sync_invalid_fd") (result i32)
    i32.const 99      ;; non-existent fd
    call $fd_sync
  )

  (export "memory" (memory 0))
)

;; Valid fd: fd_sync should succeed (errno::success = 0)
(assert_return (invoke "test_sync") (i32.const 0))

;; Invalid fd: should return errno::badf (8)
(assert_return (invoke "test_sync_invalid_fd") (i32.const 8))
