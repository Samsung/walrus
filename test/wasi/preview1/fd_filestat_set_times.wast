(module
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_filestat_set_times"
    (func $fd_filestat_set_times (param i32 i64 i64 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close (param i32) (result i32)))

  (memory 1)
  (data (i32.const 300) "./write_to_this.txt")  ;; 19 bytes, file path

  ;; Open file, call fd_filestat_set_times with ATIM_NOW|MTIM_NOW, close.
  ;; Returns errno of fd_filestat_set_times.
  (func (export "test_filestat_set_times") (result i32)
    ;; --- path_open ---
    i32.const 3
    i32.const 1
    i32.const 300
    i32.const 19
    i32.const 9         ;; oflags: creat|trunc
    i64.const 8396864   ;; FD_WRITE(64) | PATH_OPEN(8192) | FD_FILESTAT_SET_TIMES(8388608)
    i64.const 8396864
    i32.const 0
    i32.const 0         ;; store opened fd at memory[0]
    call $path_open
    i32.eqz
    (if (then) (else i32.const 1 return))  ;; abort if open failed

    ;; --- fd_filestat_set_times: test target ---
    i32.const 0
    i32.load            ;; fd
    i64.const 0         ;; atim (ignored: ATIM_NOW is set)
    i64.const 0         ;; mtim (ignored: MTIM_NOW is set)
    i32.const 10        ;; fst_flags: ATIM_NOW(2) | MTIM_NOW(8) = 10
    call $fd_filestat_set_times
    ;; stash errno at memory[700] so we can close before returning
    i32.const 700
    i32.store

    ;; --- fd_close ---
    i32.const 0
    i32.load
    call $fd_close
    drop

    i32.const 700
    i32.load            ;; return stashed errno
  )

  ;; Call fd_filestat_set_times with an invalid fd. Expects errno::badf (8).
  (func (export "test_filestat_set_times_invalid_fd") (result i32)
    i32.const 99        ;; non-existent fd
    i64.const 0
    i64.const 0
    i32.const 0
    call $fd_filestat_set_times
  )

  (export "memory" (memory 0))
)

;; Valid fd: fd_filestat_set_times should succeed (errno::success = 0)
(assert_return (invoke "test_filestat_set_times") (i32.const 0))

;; Invalid fd: should return errno::badf (8)
(assert_return (invoke "test_filestat_set_times_invalid_fd") (i32.const 8))
