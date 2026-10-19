;; Regression for Samsung/escargot issue #1720.
;; https://github.com/Samsung/escargot/issues/1720
(module
  (type (;0;) (func (param f32 f64 i64 i32 f64 i64) (result i32)))
  (type (;1;) (func (result exnref)))
  (type (;2;) (func (result exnref)))
  (type (;3;) (func))
  (type (;4;) (func (result v128 i64)))
  (type (;5;) (func (param f64 f32 v128 f32) (result i64 f32 f64 i64)))
  (type (;6;) (func))
  (type (;7;) (func))
  (type (;8;) (func (param f64) (result f64 i64 v128 f32)))
  (type (;9;) (func))
  (type (;10;) (func (param i32) (result i32)))
  (type (;11;) (func (result i32)))
  (type (;12;) (func (result i32)))
  (memory (;0;) 1)
  (func (;0;) (type 0) (param f32 f64 i64 i32 f64 i64) (result i32)
    (local v128 v128 i32 i32 f32 v128 f32 f64 exnref exnref i64 f64 f32 i64 i64 v128 f32 v128 i32 v128 exnref exnref i32 i32 f64 f32 i64 exnref f64 i64 v128 i32 i32 i32 f64 f32 i32 f32 v128 i64 f64 i64 f64 i32)
    block (type 1) (result exnref) ;; label = @1
      block (type 2) (result exnref) ;; label = @2
        block (type 3) ;; label = @3
          try_table (type 4) (result v128 i64) (catch_all_ref 1 (;@2;)) (catch_all_ref 2 (;@1;)) (catch_all 0 (;@3;)) ;; label = @4
            local.get 1
            local.get 0
            local.get 7
            local.set 11
            local.set 12
            local.set 13
            local.get 14
            local.get 9
            br_if 3 (;@1;)
            local.set 15
            local.get 5
            local.get 0
            local.get 4
            local.get 5
            local.set 16
            local.set 17
            local.set 18
            local.set 19
            local.get 7
            local.get 19
          end
          local.set 20
          local.set 21
        end
        local.get 6
        f32x4.extract_lane 2
        local.set 22
        local.get 7
        local.set 25
        ref.null exn
      end
      local.set 27
      local.get 6
      i8x16.extract_lane_u 7
      local.set 29
      local.get 0
      f64.promote_f32
      local.set 30
      f32.const 0x1.6d38p+13 (;=11687;)
      i64.trunc_f32_s
      local.set 32
      local.get 27
    end
    local.get 38
    local.get 5
    i64.const 331490
    memory.atomic.wait64 offset=10004
    br 0
  )
  (func (;1;) (type 10) (param i32) (result i32)
    i32.const -63
    if (type 12) (result i32) ;; label = @1
      i32.const 668265263
    else
      i32.const 374761393
    end
  )
)
