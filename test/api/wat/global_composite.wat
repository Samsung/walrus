(module
  (type $T (struct (field i32)))
  (import "env" "g" (global (ref null $T)))
  (func (export "is_null") (result i32)
    global.get 0
    ref.is_null
  )
)
