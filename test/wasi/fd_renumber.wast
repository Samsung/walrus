(module
  (import "wasi_snapshot_preview1" "path_open"
    (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write"
    (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_renumber"
    (func $fd_renumber (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close"
    (func $fd_close (param i32) (result i32)))

  (memory 1)
  (data (i32.const 200) "renumber test\n")       ;; 14 bytes, written to file
  (data (i32.const 300) "./write_to_this.txt")   ;; 19 bytes, file path

  ;; Set up iovec at offset 500: (buf=200, len=14)
  (func $setup_iovec
    i32.const 500
    i32.const 200
    i32.store
    i32.const 504
    i32.const 14
    i32.store
  )

  ;; Open write_to_this.txt and store fd at memory[offset].
  (func $open_file (param $fd_offset i32) (result i32)
    i32.const 3
    i32.const 1
    i32.const 300
    i32.const 19
    i32.const 9       ;; oflags: creat|trunc
    i64.const 8320    ;; fd_write | path_open
    i64.const 8320
    i32.const 0
    local.get $fd_offset
    call $path_open
  )

  ;; Open file (fd stored at memory[0] = "from"),
  ;; renumber it to fd 10 ("to"),
  ;; write using fd 10 to confirm it works.
  ;; Returns errno of fd_renumber.
  (func (export "test_renumber") (result i32)
    ;; open file → fd at memory[0]
    i32.const 0
    call $open_file
    i32.eqz
    (if (then) (else i32.const 1 return))

    call $setup_iovec

    ;; fd_renumber(from=memory[0], to=10)
    i32.const 0
    i32.load          ;; from fd
    i32.const 10      ;; to fd number
    call $fd_renumber
    i32.const 700
    i32.store         ;; stash errno

    ;; write via new fd (10) to confirm renumber worked
    i32.const 10      ;; to fd
    i32.const 500
    i32.const 1
    i32.const 600
    call $fd_write
    drop

    ;; close new fd
    i32.const 10
    call $fd_close
    drop

    i32.const 700
    i32.load          ;; return stashed errno of fd_renumber
  )

  ;; fd_renumber with an invalid "from" fd. Expects errno::badf (8).
  (func (export "test_renumber_invalid_fd") (result i32)
    i32.const 99      ;; non-existent from fd
    i32.const 10
    call $fd_renumber
  )

  (export "memory" (memory 0))
)

;; fd_renumber succeeds (errno::success = 0)
(assert_return (invoke "test_renumber") (i32.const 0))

;; invalid from fd returns errno::badf (8)
(assert_return (invoke "test_renumber_invalid_fd") (i32.const 8))
