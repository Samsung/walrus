(module
  (type (;0;) (func))
  (import "spectest" "print_i32" (func $print_i32 (param i32)))

  (func $simple (export "t1")(result i32)
      (local i32)
      local.get 0
      local.tee 0
      return 
  )

  (func $foo (export "t2") (param i32) (result i32)
      (local i32)
      local.get 1 ;;1
      local.get 1 ;;2
      i32.const 1 ;;3
      i32.add ;;2
      local.set 1
      local.get 1
      drop
  )

  (func $bar (export "t3") (result i32)
      (local i32)
      local.get 0 ;;1
      local.get 0 ;;2
      i32.const 1 ;;3
      i32.add ;;2
      local.set 0
      local.tee 0
      return 
  )

)

(assert_return (invoke "t1") (i32.const 0))
(assert_return (invoke "t2" (i32.const 100)) (i32.const 0))
(assert_return (invoke "t3") (i32.const 0))

(module
  (type (;0;) (func))
  (type (;1;) (func (param i32 i32 i32)))
  (import "spectest" "print_i32" (func $print_i32 (param i32)))

  (global $max i32 (i32.const 3))

  (func $qs (type 1) (param i32 i32 i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    local.get 1
    local.get 2
    i32.lt_s
    if  ;; label = @1
      local.get 0
      local.get 2
      i32.const 2
      i32.shl
      i32.add
      local.set 7
      loop  ;; label = @2
        i32.const 0
        local.set 3
        local.get 2
        local.get 1
        i32.sub
        local.tee 8
        i32.const 0
        i32.gt_s
        if  ;; label = @3
          local.get 7
          i32.load
          local.set 6
          i32.const 0
          local.set 4
          loop  ;; label = @4
            block  ;; label = @5
              local.get 6
              local.get 0
              local.get 1
              local.get 3
              i32.add
              i32.const 2
              i32.shl
              i32.add
              local.tee 5
              i32.load
              local.tee 9
              i32.gt_s
              if  ;; label = @6
                local.get 3
                i32.const 1
                i32.add
                local.set 3
                br 1 (;@5;)
              end
              local.get 5
              local.get 0
              local.get 2
              local.get 4
              i32.sub
              i32.const 2
              i32.shl
              i32.add
              local.tee 5
              i32.const 4
              i32.sub
              local.tee 10
              i32.load
              i32.store
              local.get 5
              local.get 9
              i32.store
              local.get 10
              local.get 6
              i32.store
              local.get 4
              i32.const 1
              i32.add
              local.set 4
            end
            local.get 3
            local.get 4
            i32.add
            local.get 8
            i32.lt_s
            br_if 0 (;@4;)
          end
        end
        local.get 0
        local.get 1
        local.get 1
        local.get 3
        i32.add
        local.tee 1
        i32.const 1
        i32.sub
        call $qs
        local.get 1
        i32.const 1
        i32.add
        local.tee 1
        local.get 2
        i32.lt_s
        br_if 0 (;@2;)
      end
    end)



  (func $print (type 0)
      (local i32 i32)
      global.get $max
      local.set 1
      (loop
        local.get 0
        i32.const 4
        i32.mul
        i32.load
        call $print_i32

        local.get 0
        i32.const 1
        i32.add
        local.tee 0
        local.get 1
        i32.ne
        br_if 0 
      )

  )

  (func $start (type 0)
      (local i32 i32)
      global.get $max
      local.set 1

      (loop
        local.get 0
        i32.const 4
        i32.mul
        local.get 1
        local.get 0
        i32.sub
        i32.store

        local.get 0
        i32.const 1
        i32.add
        local.tee 0
        local.get 1
        i32.ne
        br_if 0 
      )
;;      call $print
      i32.const 0
      i32.const 0
      global.get $max
      call $qs
;;      call $print
  )


  (memory (;0;) 256 256)

  (start $start)

  (func (export "read") (param i32) (result i32)
    (i32.load (local.get 0))
  )

)

(assert_return (invoke "read" (i32.const 0)) (i32.const 0))
(assert_return (invoke "read" (i32.const 4)) (i32.const 1))
(assert_return (invoke "read" (i32.const 8)) (i32.const 2))

