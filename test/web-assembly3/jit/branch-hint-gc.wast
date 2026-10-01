(module
  (type $s (struct (field i32)))

  (func $make (param $k i32) (result anyref)
    local.get $k
    i32.eqz
    if
      ref.null any
      return
    end
    local.get $k
    i32.const 1
    i32.eq
    if
      i32.const 7
      ref.i31
      return
    end
    i32.const 9
    struct.new $s
  )

  (func $null_taken (param $r (ref null $s)) (result i32)
    (block $l
      local.get $r
      (@metadata.code.branch_hint "\01")
      br_on_null $l
      struct.get $s 0
      return
    )
    i32.const -1
  )

  (func $null_not_taken (param $r (ref null $s)) (result i32)
    (block $l
      local.get $r
      (@metadata.code.branch_hint "\00")
      br_on_null $l
      struct.get $s 0
      return
    )
    i32.const -1
  )

  (func $non_null_taken (param $r (ref null $s)) (result i32)
    (block $l (result (ref $s))
      local.get $r
      (@metadata.code.branch_hint "\01")
      br_on_non_null $l
      i32.const -1
      return
    )
    struct.get $s 0
  )

  (func $non_null_not_taken (param $r (ref null $s)) (result i32)
    (block $l (result (ref $s))
      local.get $r
      (@metadata.code.branch_hint "\00")
      br_on_non_null $l
      i32.const -1
      return
    )
    struct.get $s 0
  )

  (func $cast_generic_taken (param $r anyref) (result i32)
    (block $l (result i31ref)
      local.get $r
      (@metadata.code.branch_hint "\01")
      br_on_cast $l anyref (ref i31)
      drop
      i32.const -1
      return
    )
    i31.get_s
  )

  (func $cast_generic_not_taken (param $r anyref) (result i32)
    (block $l (result i31ref)
      local.get $r
      (@metadata.code.branch_hint "\00")
      br_on_cast $l anyref (ref i31)
      drop
      i32.const -1
      return
    )
    i31.get_s
  )

  (func $cast_defined_taken (param $r anyref) (result i32)
    (block $l (result (ref $s))
      local.get $r
      (@metadata.code.branch_hint "\01")
      br_on_cast $l anyref (ref $s)
      drop
      i32.const -1
      return
    )
    struct.get $s 0
  )

  (func $cast_defined_not_taken (param $r anyref) (result i32)
    (block $l (result (ref $s))
      local.get $r
      (@metadata.code.branch_hint "\00")
      br_on_cast $l anyref (ref $s)
      drop
      i32.const -1
      return
    )
    struct.get $s 0
  )

  (func $cast_fail_generic_taken (param $r anyref) (result i32)
    (block $l (result anyref)
      local.get $r
      (@metadata.code.branch_hint "\01")
      br_on_cast_fail $l anyref (ref i31)
      i31.get_s
      return
    )
    drop
    i32.const -1
  )

  (func $cast_fail_generic_not_taken (param $r anyref) (result i32)
    (block $l (result anyref)
      local.get $r
      (@metadata.code.branch_hint "\00")
      br_on_cast_fail $l anyref (ref i31)
      i31.get_s
      return
    )
    drop
    i32.const -1
  )

  (func $cast_fail_defined_taken (param $r anyref) (result i32)
    (block $l (result anyref)
      local.get $r
      (@metadata.code.branch_hint "\01")
      br_on_cast_fail $l anyref (ref $s)
      struct.get $s 0
      return
    )
    drop
    i32.const -1
  )

  (func $cast_fail_defined_not_taken (param $r anyref) (result i32)
    (block $l (result anyref)
      local.get $r
      (@metadata.code.branch_hint "\00")
      br_on_cast_fail $l anyref (ref $s)
      struct.get $s 0
      return
    )
    drop
    i32.const -1
  )

  (func $return_cast_fail (param $r anyref) (result anyref)
    local.get $r
    (@metadata.code.branch_hint "\00")
    br_on_cast_fail 0 anyref (ref i31)
    drop
    ref.null any
  )

  (func (export "null") (param $k i32) (result i32)
    (call $null_taken (ref.cast (ref null $s) (call $make (local.get $k))))
    (call $null_not_taken (ref.cast (ref null $s) (call $make (local.get $k))))
    i32.add
  )

  (func (export "non_null") (param $k i32) (result i32)
    (call $non_null_taken (ref.cast (ref null $s) (call $make (local.get $k))))
    (call $non_null_not_taken (ref.cast (ref null $s) (call $make (local.get $k))))
    i32.add
  )

  (func (export "cast_generic") (param $k i32) (result i32)
    (call $cast_generic_taken (call $make (local.get $k)))
    (call $cast_generic_not_taken (call $make (local.get $k)))
    i32.add
  )

  (func (export "cast_defined") (param $k i32) (result i32)
    (call $cast_defined_taken (call $make (local.get $k)))
    (call $cast_defined_not_taken (call $make (local.get $k)))
    i32.add
  )

  (func (export "cast_fail_generic") (param $k i32) (result i32)
    (call $cast_fail_generic_taken (call $make (local.get $k)))
    (call $cast_fail_generic_not_taken (call $make (local.get $k)))
    i32.add
  )

  (func (export "cast_fail_defined") (param $k i32) (result i32)
    (call $cast_fail_defined_taken (call $make (local.get $k)))
    (call $cast_fail_defined_not_taken (call $make (local.get $k)))
    i32.add
  )

  (func (export "return_cast_fail") (param $k i32) (result i32)
    (ref.is_null (call $return_cast_fail (call $make (local.get $k))))
  )
)

(assert_return (invoke "null" (i32.const 0)) (i32.const -2))
(assert_return (invoke "null" (i32.const 2)) (i32.const 18))
(assert_return (invoke "non_null" (i32.const 0)) (i32.const -2))
(assert_return (invoke "non_null" (i32.const 2)) (i32.const 18))
(assert_return (invoke "cast_generic" (i32.const 0)) (i32.const -2))
(assert_return (invoke "cast_generic" (i32.const 1)) (i32.const 14))
(assert_return (invoke "cast_generic" (i32.const 2)) (i32.const -2))
(assert_return (invoke "cast_defined" (i32.const 0)) (i32.const -2))
(assert_return (invoke "cast_defined" (i32.const 1)) (i32.const -2))
(assert_return (invoke "cast_defined" (i32.const 2)) (i32.const 18))
(assert_return (invoke "cast_fail_generic" (i32.const 0)) (i32.const -2))
(assert_return (invoke "cast_fail_generic" (i32.const 1)) (i32.const 14))
(assert_return (invoke "cast_fail_generic" (i32.const 2)) (i32.const -2))
(assert_return (invoke "cast_fail_defined" (i32.const 0)) (i32.const -2))
(assert_return (invoke "cast_fail_defined" (i32.const 1)) (i32.const -2))
(assert_return (invoke "cast_fail_defined" (i32.const 2)) (i32.const 18))
(assert_return (invoke "return_cast_fail" (i32.const 0)) (i32.const 1))
(assert_return (invoke "return_cast_fail" (i32.const 1)) (i32.const 1))
(assert_return (invoke "return_cast_fail" (i32.const 2)) (i32.const 0))
