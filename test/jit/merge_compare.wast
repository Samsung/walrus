(module
(func (export "select_i32_eqz") (param i32 i32) (result i32)
	(local i32)
	local.get 0
	local.get 1

	local.get 0
	i32.eqz
	local.set 2
	local.get 2
	select

	local.get 2
	i32.add
)

(func (export "select_i64_and") (param i64 i64) (result i64)
	(local i32)
	local.get 0
	local.get 1

	i32.const 0
	i32.const 0
	i32.and
	local.set 2
	local.get 2
	select

	local.get 2
	i64.extend_i32_s
	i64.add
)

(func (export "br_if_i32_lt") (param i32 i32) (result i32)
 (local i32)
  block
    local.get 0
    local.get 1
    i32.lt_s
    local.set 2
    local.get 2
    br_if 0
    local.get 2
    return
  end
  i32.const 42
)

(func (export "br_if_i64_gt") (param i64 i64) (result i32)
  (local i32)
  block
    local.get 0
    local.get 1
    i64.gt_u
    local.set 2
    local.get 2
    br_if 0
    local.get 2
    return
  end
  i32.const 42
)

(func (export "select_f32_eq") (param f32 f32) (result f32)
	local.get 0
	local.get 1

	local.get 0
	local.get 0
	f32.eq
	select

	local.get 0
	f32.add
)

(func (export "select_f64_ne") (param f64 f64) (result f64)
	local.get 0
	local.get 1

	local.get 0
	local.get 0
	f64.ne
	select

	local.get 0
	f64.add
)

(func (export "br_if_f32_ge") (param f32 f32) (result i32)
 (local i32)
  block
    local.get 0
    local.get 1
    f32.ge
    local.set 2
    local.get 2
    br_if 0
    local.get 2
    return
  end
  i32.const 42
)

(func (export "br_if_f64_le") (param f64 f64) (result i32)
 (local i32)
  block
    local.get 0
    local.get 1
    f64.le
    local.set 2
    local.get 2
    br_if 0
    local.get 2
    return
  end
  i32.const 42
)

(func (export "select_v128_any_true") (param v128 v128) (result v128)
	(local i32)
	local.get 0
	local.get 1

	local.get 0
	local.get 0
	i64x2.eq
	v128.any_true
	local.set 2
	local.get 2
	select

	local.get 2
	i32.eqz
	drop
	
	local.get 1
	i64x2.add
)

(func (export "br_if_v128_all_true") (param v128 v128) (result v128)
	(local i32)
	(block
		local.get 0
		local.get 1

		local.get 0
		local.get 0
		f32x4.eq
		i32x4.all_true
		local.set 2
		local.get 2
		br_if 0

		v128.const f32x4 0.1 0.1 0.1 0.1
		return
	)

	local.get 2
	i32.eqz
	drop
	
	local.get 0
	local.get 1
	f32x4.add
)
)

(assert_return (invoke "select_i32_eqz" (i32.const 0) (i32.const 1)) (i32.const 1))
(assert_return (invoke "select_i64_and" (i64.const 0) (i64.const 1)) (i64.const 1))
(assert_return (invoke "br_if_i32_lt" (i32.const 1) (i32.const 0)) (i32.const 0))
(assert_return (invoke "br_if_i64_gt" (i64.const 0) (i64.const 1)) (i32.const 0))
(assert_return (invoke "select_f32_eq" (f32.const 0) (f32.const 1)) (f32.const 0))
(assert_return (invoke "select_f64_ne" (f64.const 0) (f64.const 1)) (f64.const 1))
(assert_return (invoke "br_if_f32_ge" (f32.const 0) (f32.const 1)) (i32.const 0))
(assert_return (invoke "br_if_f64_le" (f64.const 1) (f64.const 0)) (i32.const 0))
(assert_return (invoke "select_v128_any_true" (v128.const i64x2 1 1) (v128.const i64x2 0 0)) (v128.const i64x2 1 1))
(assert_return (invoke "br_if_v128_all_true" (v128.const f32x4 1 1 1 1) (v128.const f32x4 1 1 1 1)) (v128.const f32x4 2 2 2 2))
