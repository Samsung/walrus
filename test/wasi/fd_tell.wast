(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_read" (func $wasi_fd_read (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_seek" (func $wasi_fd_seek (param i32 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_tell" (func $wasi_fd_tell (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
 
  (memory 1)
  
  (export "fd_tell_test" (func $fd_tell_test))
  (data (i32.const 300) "./text.txt")
 
  (; 
    This test searches for the file 'text.txt' in the first opened directory
    and reads 'Hello' from it and return with the file offset.
    It will only read from the file if the directories were mapped correctly.
  ;)
 
  (func $fd_tell_test (result i32)
    i32.const 3 ;; Directory file descriptior, by default 3 is the first opened directory
    i32.const 1 ;;lookupflags: directory
    i32.const 300 ;; Offset of file name in memory
    i32.const 19 ;; Length of file name
    i32.const 0 ;; oflags: none
    i64.const 8228 ;; rights: path_open, fd_seek, fd_tell
    i64.const 8228 ;; rights_inheriting: path_open, fd_seek, fd_tell
    i32.const 0 ;; fdflags: none
    i32.const 0 ;; Offset to store at the opened file descriptor in memory
    call $path_open
    
    i32.eqz ;; fail if file could not be opened
    (if
      (then)
      (else
        i32.const 1
        call $proc_exit
      )
    )
 
    (call $wasi_fd_seek
      (i32.const 0)
      (i32.load)
      (i64.const 5)
	  (i32.const 0)
      (i32.const 1000)
    )

	i32.eqz
    (if
      (then)
      (else
        i32.const 2
        call $proc_exit
      )
    )

	i32.const 0
	i32.load
	i32.const 100
 	call $wasi_fd_tell

	i32.eqz
    (if
      (then)
      (else
        i32.const 3
        call $proc_exit
      )
    )
	
	i32.const 100
	i32.load
  )
)
 
(assert_return (invoke "fd_tell_test") (i32.const 5))

