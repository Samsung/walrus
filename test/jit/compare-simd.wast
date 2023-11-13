(module
(func (export "test1") (param v128) (result i32 i32 i32 i32 i32)
  block (result i32)
    i32.const 1
    local.get 0
    v128.any_true
    br_if 0
    drop
    i32.const 0
  end

  block (result i32)
    i32.const 1
    local.get 0
    i8x16.all_true
    br_if 0
    drop
    i32.const 0
  end

  block (result i32)
    i32.const 1
    local.get 0
    i16x8.all_true
    br_if 0
    drop
    i32.const 0
  end

  block (result i32)
    i32.const 1
    local.get 0
    i32x4.all_true
    br_if 0
    drop
    i32.const 0
  end

  block (result i32)
    i32.const 1
    local.get 0
    i64x2.all_true
    br_if 0
    drop
    i32.const 0
  end
)

(func (export "test2") (param v128) (result i32 i32 i32 i32 i32)
  block (result i32)
    i32.const 1
    i32.const 0
    local.get 0
    v128.any_true
    select
  end

  block (result i32)
    i32.const 1
    i32.const 0
    local.get 0
    i8x16.all_true
    select
  end

  block (result i32)
    i32.const 1
    i32.const 0
    local.get 0
    i16x8.all_true
    select
  end

  block (result i32)
    i32.const 1
    i32.const 0
    local.get 0
    i32x4.all_true
    select
  end

  block (result i32)
    i32.const 1
    i32.const 0
    local.get 0
    i64x2.all_true
    select
  end
)
)

(assert_return (invoke "test1" (v128.const i64x2 -1 -1))
  (i32.const 1) (i32.const 1) (i32.const 1) (i32.const 1) (i32.const 1))
(assert_return (invoke "test1" (v128.const i64x2 0 0))
  (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 0))

(assert_return (invoke "test2" (v128.const i64x2 -1 -1))
  (i32.const 1) (i32.const 1) (i32.const 1) (i32.const 1) (i32.const 1))
(assert_return (invoke "test2" (v128.const i64x2 0 0))
  (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 0) (i32.const 0))
