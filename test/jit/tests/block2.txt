(module
(func $test1 (local i32)
  block
    i32.const 1
    if (result i32)
      i32.const 2
    else
      i32.const 3
      if (result i32)
        i32.const 2
      else
        i32.const 2
        i32.const 5
        br_if 1
      end

      local.set 0
      br 1
    end

    local.set 0
  end
)

(func $test2 (local i32)
  block
    i32.const 1
    if (result i32)
      local.get 0
    else
      i32.const 3
      if (result i32)
        local.get 0
      else
        local.get 0
        i32.const 5
        br_if 1
      end

      local.set 0
      br 1
    end

    local.set 0
  end
)

(func $test3 (local i32)
  block
    i32.const 1
    if (result i32)
      i32.const 2
    else
      i32.const 3
      if (result i32)
        local.get 0
      else
        local.get 0
        i32.const 5
        br_if 1
      end

      local.set 0
      br 1
    end

    local.set 0
  end
)

(func $test4 (local i32 i32)
  block
    i32.const 1
    if (result i32)
      i32.const 2
    else
      i32.const 3
      if (result i32)
        local.get 0
      else
        local.get 0
        i32.const 5
        br_if 1
      end

      local.set 0
      br 1
    end

    local.set 1
  end
)
)
