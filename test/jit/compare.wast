(module
(func (export "test1") (result i32 i32 i32 i32) (local i64)
  i64.const 0x0
  local.set 0

  i64.const 0xff00000000
  i64.eqz

  i64.const 0x1
  i64.eqz

  local.get 0
  i64.eqz

  local.get 0
  i64.eqz
  if (result i32)
     block (result i32)
       i32.const 3

       local.get 0
       i64.eqz
       br_if 0

       drop
       i32.const 4
    end
  else
    i32.const 6
  end

  (; 0, 0, 1, 3 ;)
)

(func (export "test2") (param i64 i64) (result i32)
   local.get 0
   local.get 1
   i64.lt_s
)
)

(assert_return (invoke "test1") (i32.const 0) (i32.const 0) (i32.const 1) (i32.const 3))
(assert_return (invoke "test2" (i64.const 0x7fffffff) (i64.const 0x80000000)) (i32.const 1))
