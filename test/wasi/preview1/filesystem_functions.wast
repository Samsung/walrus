(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_create_directory" (func $path_create_directory (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_remove_directory" (func $path_remove_directory (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_filestat_get" (func $path_filestat_get (param i32 i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_rename" (func $path_rename (param i32 i32 i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "path_unlink_file" (func $path_unlink_file (param i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "poll_oneoff" (func $poll_oneoff (param i32 i32 i32 i32) (result i32)))

  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close" (func $fd_close (param i32) (result i32)))
  
  (memory 1)
  
  (export "create_directory" (func $create_directory))
  (export "rename_directory" (func $rename_directory))
  (export "remove_directory" (func $remove_directory))
  (export "poll" (func $poll))
  (export "create_file_and_filestat" (func $create_file_and_filestat))
  (export "unlink" (func $unlink))

  (data (i32.const 300) "./")
  (data (i32.const 400) "./new_dir")
  (data (i32.const 500) "./renamed_dir")
  (data (i32.const 600) "./some_file.txt")

  ;; helper function
  (func $open_wasi_dir (param i64) (result i32)
    i32.const 3 ;; Directory file descriptior
    i32.const 1 ;;lookupflags: directory
    i32.const 300 ;; Offset of file name in memory
    i32.const 2 ;; Length of file name
    i32.const 0 ;; oflags: none
    local.get 0 ;; rights
    local.get 0 ;; righst inheritings
    i32.const 0 ;; fdflags: none
    i32.const 0 ;; Offset to store at the opened file descriptor in memory
    call $path_open
  )

  (func $create_directory 
    (param i32 i32 i32) ;; fd offset, name offset, name length
    (result i32)
    i64.const 512
    call $open_wasi_dir
    drop
  
    local.get 0
    i32.load ;; fd of the containing dir
    local.get 1 ;; name relative to containing dir
    local.get 2 ;; name length
    call $path_create_directory

    i32.eqz
  )

  (func $rename_directory 
    (param i32 i32 i32 i32 i32) ;; fd, old name, old name length, new name, new name length
    (result i32)
    i64.const 196608
    call $open_wasi_dir
    drop

    local.get 0
    i32.load ;; fd of the containing dir
    local.get 1 ;; old name relative to containing dir
    local.get 2 ;; old name length
    local.get 0
    i32.load ;; new fd location
    local.get 3 ;; new name
    local.get 4 ;; new name length
    call $path_rename

    i32.eqz
  )

  (func $remove_directory 
    (param i32 i32 i32) ;; fd, name, name length
    (result i32)
    i64.const 33554432
    call $open_wasi_dir
    drop

    local.get 0
    i32.load ;; fd of the containing dir
    local.get 1 ;; name of directory
    local.get 2 ;; name length
    call $path_remove_directory

    i32.eqz
  )

  (func $poll 
    (param i32) ;; number of events
    (result i32)
    i64.const 134217762
    call $open_wasi_dir
    drop

    i32.const 1000 ;; offset of subscriptions
    i32.const 1100 ;; offset where events should be stored at
    local.get 0 ;; number of events
    i32.const 1200 ;; offset of number of stored events
    call $poll_oneoff

    i32.eqz
  )

  (func $create_file_and_filestat 
    (param i32 i32 i32) ;; fd offset, name, length
    (result i32)
    i64.const 271360
    call $open_wasi_dir
    drop

    i32.const 0
    i32.load ;; fd
    i32.const 1 ;; dirflags
    i32.const 600 ;; Offset of file name in memory
    i32.const 15 ;; Length of file name
    i32.const 1 ;; oflags: none
    i64.const 271360 ;; rights 
    i64.const 271360 ;; rights inheriting
    i32.const 0 ;; fdflags: none
    i32.const 2000 ;; Offset to store at the opened file descriptor in memory
    call $path_open

    i32.const 2000 ;; fd
    call $fd_close
    drop

    i32.eqz
    (if
      (then)
      (else
        i32.const 0
        return
      )  
    )

    local.get 0
    i32.load ;; fd
    i32.const 1 ;; lookupflags
    local.get 1 ;; name
    local.get 2 ;; name length
    i32.const 2100 ;; offset of stored data
    call $path_filestat_get

    i32.eqz
  )

  (func $unlink
    (param i32 i32 i32) ;; fd, name, name length
    (result i32)
    i64.const 67108864
    call $open_wasi_dir
    drop

    local.get 0
    i32.load ;; fd of containing dir
    local.get 1 ;; name
    local.get 2 ;; name length
    call $path_unlink_file

    i32.eqz
  )

)

;; correct tests
(assert_return (invoke "create_directory" (i32.const 0) (i32.const 400) (i32.const 9)) (i32.const 1))
(assert_return (invoke "rename_directory" (i32.const 0) (i32.const 400) (i32.const 9) (i32.const 500) (i32.const 13)) (i32.const 1))
(assert_return (invoke "remove_directory" (i32.const 0) (i32.const 500) (i32.const 13)) (i32.const 1))
(assert_return (invoke "poll" (i32.const 1)) (i32.const 1))
(assert_return (invoke "create_file_and_filestat" (i32.const 0) (i32.const 600) (i32.const 16)) (i32.const 1))
(assert_return (invoke "unlink" (i32.const 0) (i32.const 600) (i32.const 16)) (i32.const 1))

;; incorrect tests
(assert_return (invoke "create_directory" (i32.const 0) (i32.const 400) (i32.const 0)) (i32.const 0))
(assert_return (invoke "create_directory" (i32.const 0) (i32.const 20000) (i32.const 9)) (i32.const 0))
(assert_return (invoke "create_directory" (i32.const 20000) (i32.const 400) (i32.const 9)) (i32.const 0))

(assert_return (invoke "rename_directory" (i32.const 20000) (i32.const 400) (i32.const 9) (i32.const 500) (i32.const 13)) (i32.const 0))
(assert_return (invoke "rename_directory" (i32.const 0) (i32.const 20000) (i32.const 9) (i32.const 500) (i32.const 13)) (i32.const 0))
(assert_return (invoke "rename_directory" (i32.const 0) (i32.const 400) (i32.const 0) (i32.const 500) (i32.const 13)) (i32.const 0))
(assert_return (invoke "rename_directory" (i32.const 0) (i32.const 400) (i32.const 9) (i32.const 20000) (i32.const 13)) (i32.const 0))
(assert_return (invoke "rename_directory" (i32.const 0) (i32.const 400) (i32.const 9) (i32.const 500) (i32.const 0)) (i32.const 0))

(assert_return (invoke "remove_directory" (i32.const 20000) (i32.const 500) (i32.const 13)) (i32.const 0))
(assert_return (invoke "remove_directory" (i32.const 0) (i32.const 20000) (i32.const 13)) (i32.const 0))
(assert_return (invoke "remove_directory" (i32.const 0) (i32.const 500) (i32.const 0)) (i32.const 0))

(assert_return (invoke "poll" (i32.const 0)) (i32.const 0))

;; if tested with empty name or 0 length name it refers to the containing directory
(assert_return (invoke "create_file_and_filestat" (i32.const 20000) (i32.const 600) (i32.const 16)) (i32.const 0))

(assert_return (invoke "unlink" (i32.const 20000) (i32.const 600) (i32.const 16)) (i32.const 0))
(assert_return (invoke "unlink" (i32.const 0) (i32.const 20000) (i32.const 16)) (i32.const 0))
(assert_return (invoke "unlink" (i32.const 0) (i32.const 600) (i32.const 0)) (i32.const 0))
