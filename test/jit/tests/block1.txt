(module
(func $test1
  block
    i32.const 1
    i32.const 2
    i32.const 3
    br 0

    drop
    i32.add
    drop

    i32.const 1
    if
      loop
        i32.const 2
        if (result i32)
          i32.const 3
        else
          i32.const 4
        end
        br 1
      end
    else
      i32.const 5
      i32.const 6
      if (param i32) (result i32)
        drop
        i32.const 7
      end
      drop
    end
  end
)

(func $test2
  i32.const 5
  loop (param i32)
    br 0
  end
)
)
