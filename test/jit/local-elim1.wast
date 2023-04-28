(module
(func $test1 (local i32 i32)
  local.get 0
  local.set 1

  i32.const 5
  local.set 0

  local.get 1
  i32.const 5
  i32.add
  local.set 0
)

(func $test2 (local i32)
  i32.const 1
  if (result i32)
    local.get 0
  else
    local.get 0
  end

  local.set 0
)

(func $test3 (local i32)
  i32.const 1
  if (result i32)
    i32.const 3
  else
    i32.const 3
  end

  local.set 0
)

(func $test4 (local i32)
  i32.const 1
  if (result i32)
    local.get 0
  else
    i32.const 3
  end

  local.set 0
)
)
