(module
(func (result i32) (local i32 i32)
  i32.const 1
  local.set 0

  i32.const 5
  if
    i32.const 1
    local.set 0

    local.get 0
    drop
  else
    i32.const 1
    local.set 1
  end

  local.get 1
  drop

  local.get 0
)

(func (local i32)
  i32.const 1
  drop

  i32.const 1
  local.set 0

  loop
    loop
      i32.const 1
      local.set 0

      i32.const 0
      br_if 1

      i32.const 0
      br_if 0
    end
  end
)

(func (local i32)
  loop
    i32.const 0
    br_if 0
  end

  loop
    loop
      i32.const 0
      br_if 1

      i32.const 0
      if
        i32.const 1
        local.set 0
      else
        local.get 0
        br_if 1
      end
    end
  end

  loop
    i32.const 0
    br_if 0
  end
)

(func (local i32)
  loop
    i32.const 0
    if
      i32.const 0
      local.set 0
    else
      i32.const 0
      local.set 0
    end
    local.get 0
    br_if 0
  end
)

(func (local i32 i32 i32)
  i32.const 0
  local.set 2
  local.get 2
  local.set 1
  local.get 1
  local.set 0
  local.get 0
  drop
)

(func (param i32 i32) (local i32 i32 i32)
  i32.const 0
  local.set 4
  local.get 4
  local.set 0
)
)
