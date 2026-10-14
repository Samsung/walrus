(module
  ;; zero parameters and zero results
  (type $t1 (func))
  ;; empty parameter sequence and a nonempty result sequence
  (type $t2 (func (result i32)))
  ;; nonempty parameter sequence and an empty result sequence
  (type $t3 (func (param i32)))
  ;; nonempty parameter and result sequences
  (type $t4 (func (param i32) (result i32)))

  (func $f1 (type $t1))
  (func $f2 (type $t2)
    i32.const 0
  )
  (func $f3 (type $t3)
    local.get 0
    drop
  )
  (func $f4 (type $t4)
    local.get 0
  )

  (export "f1" (func $f1))
  (export "f2" (func $f2))
  (export "f3" (func $f3))
  (export "f4" (func $f4))
)
