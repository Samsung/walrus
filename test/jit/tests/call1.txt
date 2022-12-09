(module
(func $f1 (param i32 i32 i32) (result i32 i32)
  local.get 0
  local.get 1
  i32.add
  local.get 2
)

(func $test1 (export "test1") (result i32 i32)
   i32.const 1
   i32.const 2
   i32.const 3
   call $f1 (; 1 2 3 -> 3 3 ;)
)

(func $test2 (export "test2") (result i32 i32)
   i32.const 0
   i32.eqz
   i32.const 1
   i32.const 2
   i32.const 3
   i32.add (; 5 ;)
   i32.const 4
   i32.const 5
   i32.add (; 9 ;)
   call $f1 (; 1 5 9 -> 6 9 ;)
     i32.const 1
     i32.add (; 10 ;)
   call $f1 (; 1 6 10 -> 7 10 ;)
)
)
