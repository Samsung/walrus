(module
  (import "wasi_snapshot_preview1" "args_get" (func $wasi_args_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "args_sizes_get" (func $wasi_args_sizes_get (param i32 i32) (result i32)))

  (memory 1 1)
  (data (i32.const 32) "Hello\00World!\00Lorem ipsum dolor sit amet, consectetur adipiscing elit")

  (func (export "check_args")(result i32)
  (local $i i32)
  (local $mismatched_char_count i32)
  (local $argc i32)
  (local $argv i32)
  (local $argv_buf i32)
  (local $argv_buf_size i32)
  (local $value_addr i32)
  (local $expected_addr i32)
    i32.const 0
    local.set $mismatched_char_count

    i32.const 32
    local.set $expected_addr

    ;; Memory[0] = args count
    i32.const 0
    local.tee $argc
    ;; Memory[4] = args size in characters
    i32.const 4
    local.tee $argv_buf_size
    call $wasi_args_sizes_get
    i32.const 0
    i32.ne
    (if
      (then
        i32.const -1
        return
      )
    )
    ;; Memory[128] = argv[] (list of pointers to the strings)
    i32.const 128
    local.tee $argv
    ;; Memory[192] = argv_buf (the buffer the the strings themselves)
    i32.const 192
    local.tee $argv_buf
    call $wasi_args_get
    i32.const 0
    i32.ne
    (if
      (then
        i32.const -2
        return
      )
    )

    ;; get start of arg string (skip argv[0], which is the program path)
    local.get $argv
    i32.const 4
    i32.add
    i32.load ;; &argv[1]
    local.tee $i ;; start $i with the offset of the first char of argv[1]
    local.get $argv_buf
    i32.add ;; pointer to fist char in the buffer
    local.set $value_addr

    (loop $for_each_char
      ;; *($expected_addr) != *($value_addr)
      local.get $expected_addr
      i32.load8_u
      local.get $value_addr
      i32.load8_u
      i32.ne
      (if
        (then
          ;; $mismatched_char_count += 1
          local.get $mismatched_char_count
          i32.const 1
          i32.add
          local.set $mismatched_char_count
        )
      )

      local.get $value_addr
      i32.const 1
      i32.add
      local.set $value_addr

      local.get $expected_addr
      i32.const 1
      i32.add
      local.set $expected_addr

      local.get $i
      i32.const 1
      i32.add
      local.tee $i

      ;; $i < $argv_buf_size
      local.get $argv_buf_size
      i32.load
      i32.lt_u
      br_if $for_each_char
    )
    local.get $mismatched_char_count
  )
)

(assert_return (invoke "check_args") (i32.const 0))
