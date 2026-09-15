(module
  (tag $except0 (param i32))

  (func $throw (param i32)
    local.get 0
    throw $except0
  )

  (func (export "swapped") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      local.get 1
      i32.add
    else
      local.get 1
      local.get 1
      i32.mul
    end
  )

  (func (export "kept") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\00")
    if (result i32)
      local.get 1
      local.get 1
      i32.add
    else
      local.get 1
      local.get 1
      i32.mul
    end
  )

  (func (export "nested") (param i32 i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      (@metadata.code.branch_hint "\01")
      if (result i32)
        local.get 2
        i32.const 1
        i32.add
      else
        local.get 2
        i32.const 2
        i32.add
      end
    else
      local.get 1
      (@metadata.code.branch_hint "\00")
      if (result i32)
        local.get 2
        i32.const 3
        i32.add
      else
        local.get 2
        i32.const 4
        i32.add
      end
    end
  )

  (func (export "inloop") (param i32) (result i32)
    (local i32 i32)
    block $out
      loop $top
        local.get 2
        local.get 0
        i32.ge_s
        br_if $out
        local.get 2
        i32.const 1
        i32.and
        (@metadata.code.branch_hint "\01")
        if
          local.get 1
          i32.const 10
          i32.add
          local.set 1
        else
          local.get 1
          i32.const 1
          i32.add
          local.set 1
        end
        local.get 2
        i32.const 1
        i32.add
        local.set 2
        br $top
      end
    end
    local.get 1
  )

  (func (export "noelse") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if
      local.get 1
      i32.const 100
      i32.add
      local.set 1
    end
    local.get 1
  )

  (func (export "elsebr") (param i32 i32) (result i32)
    block $done (result i32)
      local.get 0
      (@metadata.code.branch_hint "\01")
      if (result i32)
        local.get 1
        local.get 1
        i32.add
      else
        local.get 1
        local.get 1
        i32.mul
        br $done
      end
    end
  )

  (func (export "tryouter") (param i32 i32) (result i32)
    try (result i32)
      local.get 0
      (@metadata.code.branch_hint "\01")
      if (result i32)
        local.get 1
        call $throw
        i32.const 0
      else
        local.get 1
        i32.const 1
        i32.add
      end
    catch $except0
      i32.const 1000
      i32.add
    end
  )

  (func (export "tryinthen") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      i32.const 1
      i32.add
      try (result i32)
        local.get 1
        call $throw
        i32.const 0
      catch $except0
        i32.const 10
        i32.add
      end
      i32.add
    else
      local.get 1
      i32.const 2
      i32.mul
    end
  )

  (func (export "tryinboth") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      i32.const 1
      i32.add
      try (result i32)
        local.get 1
        call $throw
        i32.const 0
      catch $except0
        i32.const 100
        i32.add
      end
      i32.add
    else
      local.get 1
      i32.const 2
      i32.add
      try (result i32)
        local.get 1
        call $throw
        i32.const 0
      catch $except0
        i32.const 200
        i32.add
      end
      i32.add
    end
  )

  (func (export "nestedtry") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      i32.const 1
      i32.add
      try (result i32)
        try (result i32)
          local.get 1
          call $throw
          i32.const 0
        catch $except0
          i32.const 1
          i32.add
          call $throw
          i32.const 0
        end
      catch $except0
        i32.const 1000
        i32.add
      end
      i32.add
    else
      local.get 1
      i32.const 2
      i32.add
      try (result i32)
        local.get 1
        call $throw
        i32.const 0
      catch $except0
        i32.const 2000
        i32.add
      end
      i32.add
    end
  )

  (func (export "elsetry") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      i32.const 4
      i32.mul
    else
      try (result i32)
        local.get 1
        call $throw
        i32.const 0
      catch $except0
        i32.const 300
        i32.add
      end
      i32.const 1
      i32.add
    end
  )
  (func (export "elseterm") (param i32 i32) (result i32)
    block $out (result i32)
      local.get 0
      (@metadata.code.branch_hint "\01")
      if (result i32)
        local.get 1
        i32.const 4
        i32.mul
      else
        local.get 1
        i32.const 100
        i32.add
        br $out
      end
      i32.const 1
      i32.add
    end
  )

  (func (export "elsereturn") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      i32.const 4
      i32.mul
    else
      local.get 1
      i32.const 7
      i32.add
      return
    end
  )

  (func (export "elseunreachable") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      local.get 1
      i32.const 4
      i32.mul
    else
      unreachable
    end
  )

  (func (export "thenblock") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      block (result i32)
        local.get 1
        i32.const 4
        i32.mul
      end
    else
      local.get 1
      i32.const 9
      i32.add
    end
  )

  (func (export "thentry") (param i32 i32) (result i32)
    local.get 0
    (@metadata.code.branch_hint "\01")
    if (result i32)
      try (result i32)
        local.get 1
        call $throw
        i32.const 0
      catch $except0
        i32.const 400
        i32.add
      end
    else
      local.get 1
      i32.const 9
      i32.add
    end
  )
)

(assert_return (invoke "swapped" (i32.const 0) (i32.const 5)) (i32.const 25))
(assert_return (invoke "swapped" (i32.const 1) (i32.const 5)) (i32.const 10))
(assert_return (invoke "kept" (i32.const 0) (i32.const 5)) (i32.const 25))
(assert_return (invoke "kept" (i32.const 1) (i32.const 5)) (i32.const 10))
(assert_return (invoke "nested" (i32.const 1) (i32.const 1) (i32.const 0)) (i32.const 1))
(assert_return (invoke "nested" (i32.const 1) (i32.const 0) (i32.const 0)) (i32.const 2))
(assert_return (invoke "nested" (i32.const 0) (i32.const 1) (i32.const 0)) (i32.const 3))
(assert_return (invoke "nested" (i32.const 0) (i32.const 0) (i32.const 0)) (i32.const 4))
(assert_return (invoke "inloop" (i32.const 0)) (i32.const 0))
(assert_return (invoke "inloop" (i32.const 1)) (i32.const 1))
(assert_return (invoke "inloop" (i32.const 4)) (i32.const 22))
(assert_return (invoke "inloop" (i32.const 5)) (i32.const 23))
(assert_return (invoke "noelse" (i32.const 0) (i32.const 7)) (i32.const 7))
(assert_return (invoke "noelse" (i32.const 1) (i32.const 7)) (i32.const 107))
(assert_return (invoke "elsebr" (i32.const 0) (i32.const 5)) (i32.const 25))
(assert_return (invoke "elsebr" (i32.const 1) (i32.const 5)) (i32.const 10))
(assert_return (invoke "tryouter" (i32.const 1) (i32.const 5)) (i32.const 1005))
(assert_return (invoke "tryouter" (i32.const 0) (i32.const 5)) (i32.const 6))
(assert_return (invoke "tryinthen" (i32.const 1) (i32.const 5)) (i32.const 21))
(assert_return (invoke "tryinthen" (i32.const 0) (i32.const 5)) (i32.const 10))
(assert_return (invoke "tryinboth" (i32.const 1) (i32.const 5)) (i32.const 111))
(assert_return (invoke "tryinboth" (i32.const 0) (i32.const 5)) (i32.const 212))
(assert_return (invoke "nestedtry" (i32.const 1) (i32.const 5)) (i32.const 1012))
(assert_return (invoke "nestedtry" (i32.const 0) (i32.const 5)) (i32.const 2012))
(assert_return (invoke "elsetry" (i32.const 1) (i32.const 5)) (i32.const 20))
(assert_return (invoke "elsetry" (i32.const 0) (i32.const 5)) (i32.const 306))
(assert_return (invoke "elseterm" (i32.const 1) (i32.const 5)) (i32.const 21))
(assert_return (invoke "elseterm" (i32.const 0) (i32.const 5)) (i32.const 105))
(assert_return (invoke "elsereturn" (i32.const 1) (i32.const 5)) (i32.const 20))
(assert_return (invoke "elsereturn" (i32.const 0) (i32.const 5)) (i32.const 12))
(assert_return (invoke "elseunreachable" (i32.const 1) (i32.const 5)) (i32.const 20))
(assert_return (invoke "thenblock" (i32.const 1) (i32.const 5)) (i32.const 20))
(assert_return (invoke "thenblock" (i32.const 0) (i32.const 5)) (i32.const 14))
(assert_return (invoke "thentry" (i32.const 1) (i32.const 5)) (i32.const 405))
(assert_return (invoke "thentry" (i32.const 0) (i32.const 5)) (i32.const 14))
