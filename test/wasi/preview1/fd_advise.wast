(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))  
  (import "wasi_snapshot_preview1" "fd_advise" (func $fd_advise (param i32 i64 i64 i32) (result i32)))

  (memory $mem 1)
  (data (i32.const 300) "./write_to_this.txt") ;; Filename in current directory (where Walrus is ran from)

  (func (export "test_advise") (param i32) (result i32) ;; advice is passed as param, return is if the function returned succesfully
    i32.const 3 ;; Directory file descriptior, by default 3 is the first opened directory
    i32.const 1 ;; lookupflags: directory
    i32.const 300 ;; Offset of file name in memory
    i32.const 19 ;; Length of file name
    i32.const 0 ;; oflags: none
    i64.const 128 ;; rights: fd_advise
    i64.const 128 ;; rights_inheriting: fd_advise
    i32.const 0 ;; fdflags: none
    i32.const 0 ;; Offset to store at the opened file descriptor in memory
    call $path_open

    i32.eqz ;; fail if file could not be opened
    (if
      (then)
      (else
        i32.const 1
        return
      )
    )
    
    i32.const 0
    i32.load ;; Get the file descriptor
    i64.const 0 ;; Start for 0 offset
    i64.const 0 ;; For the entire file
    local.get 0 ;; Advise
    call $fd_advise
    return
  )

  (export "memory" (memory 0))
)

(assert_return (invoke "test_advise" (i32.const 0)) (i32.const 0))
(assert_return (invoke "test_advise" (i32.const 1)) (i32.const 0))
(assert_return (invoke "test_advise" (i32.const 2)) (i32.const 0))
(assert_return (invoke "test_advise" (i32.const 3)) (i32.const 0))
(assert_return (invoke "test_advise" (i32.const 4)) (i32.const 0))
(assert_return (invoke "test_advise" (i32.const 5)) (i32.const 0))

(assert_return (invoke "test_advise" (i32.const 6)) (i32.const 28)) ;; Returns invalid function argument, as it should

