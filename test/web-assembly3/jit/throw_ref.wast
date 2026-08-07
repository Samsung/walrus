(module
  (func (export "throw") (param (ref exn) (ref noexn) exnref nullexnref)
                (result (ref null exn) (ref null noexn))
    local.get 0
    throw_ref
  )
)
