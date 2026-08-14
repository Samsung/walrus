(component
  (type $not-func s8)

  (core module $m
    (func (export "f"))
  )

  (core instance $i
    (instantiate $m)
  )

  (alias core export $i "f"
    (core func $f)
  )

  (func
    (type $not-func)
    (canon lift (core func $f))
  )
)