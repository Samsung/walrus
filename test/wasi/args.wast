(module
  (import "wasi_snapshot_preview1" "args_get" (func $wasi_args_get (param i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "args_sizes_get" (func $wasi_args_sizes_get (param i32 i32) (result i32)))
  (memory 1)

  (export "memory" (memory 0))
  (data (i32.const 32) "Hello" ) ;; Memory[32] = "Hello,"
  (data (i32.const 48) "World!" ) ;; Memory[48] = "World!"

  (func (export "check_args")(result i32)
  (local $i i32)
  (local $mismatched_char_count i32)
  (local $argc i32)
  (local $argv i32)
  (local $argv_buf i32)
  (local $value_addr i32)
  (local $expected_addr i32)
    ;; Memory[0] = args count
    ;; Memory[4] = args size in characters
    i32.const 0
    local.tee $argc
    i32.const 4
    call $wasi_args_sizes_get
    drop

    i32.const 128 ;; argv[] (list of pointers to the strings)
    local.tee $argv
    i32.const 192 ;; argv_buf (the buffer the the strings themselves)
    local.tee $argv_buf
    call $wasi_args_get
    drop

    i32.const 0
    local.set $mismatched_char_count

    ;; Check if each arg matches the expect values of "Hello," and "World!"
    i32.const 1  ;; start at 1, skip argv[0] (name of currently running file)
    local.set $i ;; for each arg
    (loop $for_each_arg

      ;; get start of arg string realtive to argv_buf start
      local.get $i
      i32.const 4
      i32.mul
      local.get $argv
      i32.add
      i32.load
      local.get $argv_buf
      i32.add
      local.set $value_addr

      ;; get start of expected value addr
      local.get $i
      i32.const 1
      i32.gt_u
      (if (result i32)
        (then
          i32.const 48
        )
        (else
          i32.const 32
        )
      )
      local.set $expected_addr

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

          ;; *($expected_addr) == 0 or *($value_addr) == 0
          local.get $expected_addr
          i32.load8_u
          i32.const 0
          i32.eq

          local.get $value_addr
          i32.load8_u
          i32.const 0
          i32.eq

          i32.add
          i32.const 0
          i32.eq ;; if neither is \0, then increment both addresses, and continue looping
                 ;; if either is \0, then break
          (if
            (then
              local.get $expected_addr
              i32.const 1
              i32.add
              local.set $expected_addr

              local.get $value_addr
              i32.const 1
              i32.add
              local.set $value_addr

              br $for_each_char
            )
          )
        )

      local.get $i
      i32.const 1
      i32.add
      local.tee $i

      ;; $i < $argc
      local.get $argc
      i32.load
      i32.lt_u
      br_if $for_each_arg
    )
    local.get $mismatched_char_count
  )
)

(assert_return (invoke "check_args") (i32.const 0))
