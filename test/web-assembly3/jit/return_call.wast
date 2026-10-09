(module
  (tag $e0)
  (type $func (func))

  (func $throw (type $func)
    throw $e0
  )

  (table $t funcref (elem (ref.func $throw) (ref.null func)))

  (func $call_ret (param i32)
    (block $b
      (try_table (catch $e0 $b)
        local.get 0
        (; Must not capture the exception. ;)
        return_call_indirect $t (type $func)
      )
      unreachable
    )
    unreachable
  )

  (func (export "call-ret") (param i32) (result i32)
    (block $b
      (try_table (catch $e0 $b)
        local.get 0
        call $call_ret
      )
      i32.const 123
      return
    )
    i32.const 456
  )
)

(assert_return (invoke "call-ret" (i32.const 0)) (i32.const 456))
(assert_trap (invoke "call-ret" (i32.const 1)) "uninitialized element")
