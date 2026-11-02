(module
  (func (export "check_types")
    (param (ref nofunc) (ref i31) (ref eq) (ref struct) (ref array) (ref any))
    (result (ref null nofunc))
    ref.null nofunc
  )
  (func (export "return_null_nofunc") (result (ref null nofunc))
    ref.null nofunc
  )
)
