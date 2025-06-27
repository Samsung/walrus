(module
  (import "wasi_snapshot_preview1" "path_open" (func $path_open (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_close" (func $fd_close (param i32) (result i32)))
  (import "wasi_snapshot_preview1" "fd_filestat_get" (func $fd_filestat_get (param i32 i32) (result i32)))

  (memory 1 1)
  (data (i32.const 100) "./fd_filestat_get.wast")

  (func (export "fd_filestat_get") (result i32 i32 i32)
    i32.const 3 ;; Directory file descriptior, by default 3 is the first opened directory
    i32.const 1 ;; uvwasi_lookupflags_t: UVWASI_LOOKUP_SYMLINK_FOLLOW
    i32.const 100 ;; Offset of file name in memory
    i32.const 23 ;; Length of file name
    i32.const 0 ;; uvwasi_oflags_t: none
    i64.const 0x202000 ;; base uvwasi_rights_t: UVWASI_RIGHT_PATH_OPEN, UVWASI_RIGHT_FD_FILESTAT_GET
    i64.const 0x202000 ;; inherited uvwasi_rights_t: UVWASI_RIGHT_PATH_OPEN, UVWASI_RIGHT_FD_FILESTAT_GET
    i32.const 0 ;; uvwasi_fdflags_t: none
    i32.const 0 ;; Offset to store at the opened file descriptor in memory
    call $path_open

	i32.const 0
	i32.load
	i32.const 1000
	call $fd_filestat_get

    i32.const 0
    i32.load
    call $fd_close
  )

  (func (export "fd_filestat_get_bad") (result i32 i32 i32 i32)
    i32.const 3 ;; Directory file descriptior, by default 3 is the first opened directory
    i32.const 1 ;; uvwasi_lookupflags_t: UVWASI_LOOKUP_SYMLINK_FOLLOW
    i32.const 100 ;; Offset of file name in memory
    i32.const 23 ;; Length of file name
    i32.const 0 ;; uvwasi_oflags_t: none
    i64.const 0x202000 ;; base uvwasi_rights_t: UVWASI_RIGHT_PATH_OPEN, UVWASI_RIGHT_FD_FILESTAT_GET
    i64.const 0x202000 ;; inherited uvwasi_rights_t: UVWASI_RIGHT_PATH_OPEN, UVWASI_RIGHT_FD_FILESTAT_GET
    i32.const 0 ;; uvwasi_fdflags_t: none
    i32.const 0 ;; Offset to store at the opened file descriptor in memory
    call $path_open

	i32.const 0
	i32.load
	i32.const 65535
	call $fd_filestat_get

	i32.const -1
	i32.const 1000
	call $fd_filestat_get

    i32.const 0
    i32.load
    call $fd_close
  )
)

(assert_return (invoke "fd_filestat_get") (i32.const 0) (i32.const 0)  (i32.const 0))
(assert_return (invoke "fd_filestat_get_bad") (i32.const 0) (i32.const 28) (i32.const 8) (i32.const 0))
