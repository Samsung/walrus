(module
  (type (;0;) (func))
  (import "spectest" "print_i32" (func $print_i32 (param i32)))

  (func $simple (export "t1")(result i32)
      (local i32)
      local.get 0
      local.tee 0
      return 
  )

  (func $foo (export "t2") (param i32) (result i32)
      (local i32)
      local.get 1 ;;1
      local.get 1 ;;2
      i32.const 1 ;;3
      i32.add ;;2
      local.set 1
      local.get 1
      drop
  )

  (func $bar (export "t3") (result i32)
      (local i32)
      local.get 0 ;;1
      local.get 0 ;;2
      i32.const 1 ;;3
      i32.add ;;2
      local.set 0
      local.tee 0
      return 
  )

)

(assert_return (invoke "t1") (i32.const 0))
(assert_return (invoke "t2" (i32.const 100)) (i32.const 0))
(assert_return (invoke "t3") (i32.const 0))
