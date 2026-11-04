(module
  (table $table32 (export "table32") 1 3 (ref null i31))
  (table $table64 (export "table64") i64 1 3 (ref null i31))
  (func (export "grow32")
    (drop (table.grow $table32 (ref.i31 (i32.const 256)) (i32.const 1))))
  (func (export "grow64")
    (drop (table.grow $table64 (ref.i31 (i32.const 256)) (i64.const 1))))
)
