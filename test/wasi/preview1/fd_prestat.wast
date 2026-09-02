(module
  (import "wasi_snapshot_preview1" "fd_prestat_get" (func $fd_prestat_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_prestat_dir_name" (func $fd_prestat_dir_name (param i32 i32 i32) (result i32)))
  (memory $mem 1)

  (; fd_prestat_get return information about a preopened file,
     meaning one that was passed with '--mapdirs' ;)

  (func (export "test_prestat_get") (param i32) (result i32) 
    local.get 0 ;; file descriptor
    i32.const 0 ;; stored information offset in memory
    call $fd_prestat_get
  )

  (func (export "test_prestat_dir_name") (param i32) (result i32)
    local.get 0 ;; file descriptor
    i32.const 0 ;; stored information offset in memory
    i32.const 128 ;; max lenght of file path
    call $fd_prestat_dir_name
  )
)

(assert_return (invoke "test_prestat_get" (i32.const 3)) (i32.const 0))
(assert_return (invoke "test_prestat_get" (i32.const 4)) (i32.const 8)) ;; Error 8: bad file descriptor because it does not exist
(assert_return (invoke "test_prestat_dir_name" (i32.const 3)) (i32.const 0)) ;; mapped directory
(assert_return (invoke "test_prestat_dir_name" (i32.const 0)) (i32.const 8)) ;; Error 8: bad file descriptor because stdin is not a mapped directory
(assert_return (invoke "test_prestat_dir_name" (i32.const 4)) (i32.const 8)) ;; Error 8: bad file descriptor because it does not exist

