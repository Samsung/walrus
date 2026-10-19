;; Regression for Samsung/escargot issue #1716.
;; https://github.com/Samsung/escargot/issues/1716
(module
  (type (;0;) (func (param i32 i32 i32 i32 i32 i32) (result i32 i32 v128 v128 i32)))
  (type (;1;) (func (param i32 i32 i32 i32 i32) (result i32 i32 i32 i32 i32)))
  (type (;2;) (func (param i32 i32 i32 i32) (result i32 i32 i32)))
  (func (;0;) (type 2) (param i32 i32 i32 i32) (result i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 v128 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 v128 v128)
    i32.const 0
    local.get 2
    local.get 0
    local.get 0
    local.get 1
    local.get 2
    loop (type 0) (param i32 i32 i32 i32 i32 i32) (result i32 i32 v128 v128 i32) ;; label = @1
      local.set 4
      local.set 5
      local.set 6
      local.set 7
      local.set 8
      local.get 6
      local.get 2
      local.get 5
      local.get 1
      loop (type 1) (param i32 i32 i32 i32 i32) (result i32 i32 i32 i32 i32) ;; label = @2
        local.set 9
        local.set 10
        local.set 11
        local.set 12
        local.set 13
        local.get 12
        local.get 13
        local.get 10
        local.get 13
        local.get 0
      end
      local.set 15
      local.set 16
      local.set 17
      local.set 18
      local.get 4
      local.get 7
      local.get 19
      local.get 5
      local.get 20
      local.get 21
      br_if 0 (;@1;)
      local.set 22
      local.set 23
      local.set 24
      local.set 25
      local.set 26
      local.get 23
      local.get 29
      local.get 30
      local.get 27
    end
    local.set 28
    local.set 29
    local.set 30
    local.get 28
  )
)
