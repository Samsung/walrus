(module
(func $f1
  unreachable
)

(func $f2
   call $f1
)

(func $test2 (export "test1")
   call $f2
)
)
