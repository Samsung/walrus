(module
;; (import "spectest" "print_i32" (func $print_i32 (param i32)))

(func $prime (export "prime") (param i32) (result i32)
  ;; input (param), local var, output
  ;; output 1 if prime, 0 if not
  (local i32 i32)

  i32.const 1
  local.set 2 ;; out

  block $exit
    local.get 0
    i32.const 2
    i32.eq ;; input == 2?
    if
      i32.const 1
      local.set 2
      br $exit
    end

    local.get 0
    i32.const 3
    i32.eq ;; input == 3?
    if
      i32.const 1
      local.set 2
      br $exit
    end

    local.get 0
    i32.const 2
    i32.rem_s
    i32.const 0
    i32.eq
    if
      i32.const 0
      local.set 2
      br $exit
    end

    local.get 0
    i32.const 3
    i32.rem_s
    i32.const 0
    i32.eq
    if
      i32.const 0
      local.set 2
      br $exit
    end

    i32.const 5
    local.set 1 ;; local var, i
    loop
      local.get 1
      local.get 1
      i32.mul
      local.get 0
      i32.le_s ;; if i*i <= n
      if
        local.get 0
        local.get 1
        i32.rem_s
        i32.const 0
        i32.eq
        if
          i32.const 0
          local.set 2
          br $exit
        end
        local.get 0
        local.get 1
        i32.const 2
        i32.add
        i32.rem_s
        i32.const 0
        i32.eq
        if
          i32.const 0
          local.set 2
          br $exit
        end
      end

      local.get 1
      i32.const 6
      i32.add
      local.set 1
    end
  end

  local.get 2
  ;; call $print_i32
)
)

(assert_return (invoke "prime" (i32.const 2147483647)) (i32.const 1))
(assert_return (invoke "prime" (i32.const 5)) (i32.const 1))
