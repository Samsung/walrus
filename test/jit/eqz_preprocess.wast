;; Samsung/escargot#1754: the final eqz in preprocessing must not be
;; mistaken for an instruction inside v128.load64_lane in code generation.
;; The extra assignment after the branch aligns the stale offset with the
;; lane instruction on builds that emit a shorter preprocessing sequence.
(module
  (memory 3)
  (func (export "if") (param i32 f32 f32 f64 i32 i64 i64) (result f32)
    (local v128 i32 v128 i32 i64 i32 i32)
    v128.const i32x4 0 0 0 0
    local.set 7
    i32.const 1
    local.set 8
    local.get 8
    local.get 7
    v128.load64_lane 0 0
    local.set 9
    local.get 0
    if
    else
    end
    i32.const 2
    local.set 13
    local.get 0
    i32.eqz
    local.set 10
    local.get 12
    local.get 11
    i64.const 0
    local.get 2
    return)
  (func (export "br_if") (param i32 f32 f32 f64 i32 i64 i64) (result f32)
    (local v128 i32 v128 i32 i64 i32 i32)
    v128.const i32x4 0 0 0 0
    local.set 7
    i32.const 1
    local.set 8
    local.get 8
    local.get 7
    v128.load64_lane 0 0
    local.set 9
    block
      local.get 0
      br_if 0
    end
    i32.const 2
    local.set 13
    local.get 0
    i32.eqz
    local.set 10
    local.get 12
    local.get 11
    i64.const 0
    local.get 2
    return))

(assert_return (invoke "if" (i32.const 0) (f32.const 1) (f32.const 42)
                          (f64.const 0) (i32.const 0) (i64.const 0) (i64.const 0))
               (f32.const 42))
(assert_return (invoke "if" (i32.const 1) (f32.const 1) (f32.const 42)
                          (f64.const 0) (i32.const 0) (i64.const 0) (i64.const 0))
               (f32.const 42))
(assert_return (invoke "br_if" (i32.const 0) (f32.const 1) (f32.const 42)
                             (f64.const 0) (i32.const 0) (i64.const 0) (i64.const 0))
               (f32.const 42))
(assert_return (invoke "br_if" (i32.const 1) (f32.const 1) (f32.const 42)
                             (f64.const 0) (i32.const 0) (i64.const 0) (i64.const 0))
               (f32.const 42))
