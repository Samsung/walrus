(module

(func (export "br_if_cmp")(param i32 i32 i64 f32 f64)(result i32 i32 i32 i32)
  block (result i32)
    i32.const -1
    local.get 0
    br_if 0
    drop
    local.get 1
    i32.const 100
    i32.eq
  end
  block (result i32)
    i32.const -1
    local.get 0
    br_if 0
    drop
    local.get 2
    i64.const 100
    i64.eq
  end
  block (result i32)
    i32.const -1
    local.get 0
    br_if 0
    drop
    local.get 3
    f32.const 100.0
    f32.eq
  end
  block (result i32)
    i32.const -1
    local.get 0
    br_if 0
    drop
    local.get 4
    f64.const 100.0
    f64.eq
  end
)

(func (export "test10") (param i32) (result i32)
   i32.const 6
   i32.const 7
   block (result i32)
     local.get 0
     local.get 0
     i32.eqz
     br_if 0

     i32.const 5
     i32.le_s
   end
   select
)

)

(assert_return (invoke "test10" (i32.const 0) ) (i32.const 7)  ) 
(assert_return (invoke "br_if_cmp"(i32.const 0)(i32.const 100)(i64.const 100)(f32.const 100.0)(f64.const 100.0))
  (i32.const 1)(i32.const 1)(i32.const 1)(i32.const 1))


