(module
  (tag $e0)

  (func $throw
    throw $e0
  )

  (func $throw_ref (param exnref)
    local.get 0
    throw_ref
  )

  (func (export "multi-throw") (local exnref exnref exnref)
    (block $b (result exnref)
      (try_table (catch_ref $e0 $b)
        call $throw
      )
      unreachable
    )
    local.set 0

    (block $b (result exnref)
      (try_table (catch_ref $e0 $b)
        local.get 0
        call $throw_ref
      )
      unreachable
    )
    local.set 1

    (block $b (result exnref)
      (try_table (catch_ref $e0 $b)
        local.get 0
        call $throw_ref
      )
      unreachable
    )
    local.set 2
  )
)

(assert_return (invoke "multi-throw"))
