(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))  
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))

  (memory $memory 1)
  (data (i32.const 200) "Hello World!\n") ;; String we want to write to file
  (data (i32.const 300) "./write_to_this.txt") ;; Filename in current directory (where Walrus is ran from)

  (; This test searches for the file 'write_to_this.txt' in the first opened directory.
          It will only write into the file if the directories were mapped correctly. ;)

  (func $write_to_file
    i32.const 3 ;; Directory file descriptior, by default 3 is the first opened directory
    i32.const 1 ;; lookupflags: directory
    i32.const 300 ;; Offset of file name in memory
    i32.const 19 ;; Length of file name
    i32.const 0 ;; oflags: none
    i64.const 8256 ;; rights: path_open, fd_write
    i64.const 8256 ;; rights_inheriting: path_open, fd_write
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

    i32.const 500 ;; offset of str offset
    i32.const 200 ;; offset of str
    i32.store

    i32.const 504 ;; str length offset
    i32.const 13
    i32.store

    i32.const 0 ;; load file descriptior
    i32.load
    i32.const 500 ;; offset of str offset
    i32.const 1 ;; iovec length
    i32.const 0 ;; offset of written characters
    call $fd_write
    drop
  )

  (export "_start" (func $write_to_file))
  (export "memory" (memory 0))
)
