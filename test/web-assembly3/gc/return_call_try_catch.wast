(module
  (tag $e0 (export "e0"))

  (type $-i32 (func (result i32)))
  (func $const-i32 (result i32) (i32.const 1))
  (global $const-i32 (ref $-i32) (ref.func $const-i32))

  (table funcref
    (elem
      $const-i32
    )
  )

  (func (export "return_call") (result i32)
    (local i32 i64)
    (try (result i32)
      (do
        return_call $const-i32

        i32.const 1
        local.set 0
        i64.const 2
        local.set 1
      )
      (catch $e0
        return_call $const-i32

        local.set 0
        i32.const -1
      )
    )
  )

  (func (export "return_call_indirect") (result i32)
    (local i32 i64)
    (try (result i32)
      (do
        (return_call_indirect (type $-i32) (i32.const 0))

        i32.const 1
        local.set 0
        i64.const 2
        local.set 1
      )
      (catch $e0
        (return_call_indirect (type $-i32) (i32.const 0))

        local.set 0
        i32.const -1
      )
    )
  )

  (func (export "return_call_ref") (result i32)
    (local i32 i64)
    (try (result i32)
      (do
        global.get $const-i32
        return_call_ref $-i32

        i32.const 1
        local.set 0
        i64.const 2
        local.set 1
      )
      (catch $e0
        global.get $const-i32
        return_call_ref $-i32

        local.set 0
        i32.const -1
      )
    )
  )
)

(assert_return (invoke "return_call") (i32.const 1))
(assert_return (invoke "return_call_indirect") (i32.const 1))
(assert_return (invoke "return_call_ref") (i32.const 1))
