(module
(memory 1)

(func $matrix (export "start") (result i32 i32 i32)
(local i32 i32 i32 i32) ;;i, j, sumOfRuns, sumOfArr

  i32.const 0
  local.set 2
  i32.const 0
  local.set 3
  ;; 1000x1000 matrix
  block $end
    i32.const 0
    local.set 0
    loop
      local.get 0
      i32.const 10 ;;size of matrix
      i32.eq
      if
        br $end
      end
      i32.const 0
      local.set 1

      block $loop2
        loop
          local.get 1
          i32.const 10 ;;size of matrix
          i32.gt_s
          if
            br $loop2
          end

          local.get 2
          i32.const 1
          i32.add
          local.set 2

          local.get 0
          i32.const 4
          i32.mul
          i32.const 10
          i32.mul
          local.get 1
          i32.const 4
          i32.mul
          i32.add

          local.get 0
          i32.const 10
          i32.mul
          local.get 1
          i32.add
          i32.store

          local.get 0
          i32.const 10
          i32.mul
          local.get 1
          i32.add
          local.get 3
          i32.add
          local.set 3

          local.get 1
          i32.const 1
          i32.add
          local.set 1
          br 0
        end
      end

      local.get 0
      i32.const 1
      i32.add
      local.set 0
      br 0
    end
  end
  local.get 0
  local.get 1
  local.get 2
)

(func $check (export "check") (param i32) (result i32)
  local.get 0
  i32.load
)

)

(assert_return (invoke "start") (i32.const 10) (i32.const 11) (i32.const 110))
(assert_return (invoke "check" (i32.const 4)) (i32.const 1))
(assert_return (invoke "check" (i32.const 0)) (i32.const 0))
(assert_return (invoke "check" (i32.const 8)) (i32.const 2))
(assert_return (invoke "check" (i32.const 400)) (i32.const 100))
(assert_return (invoke "check" (i32.const 40)) (i32.const 10))
