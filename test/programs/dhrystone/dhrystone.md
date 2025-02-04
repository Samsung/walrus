Dhrystone
------------------------------------------------------------------------------
`dhrystone.wasm` was compiled from
https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/tests/benchmarks/dhrystone
via `wasi-sdk` and `emcc`:
```shell
/opt/wasi-sdk/bin/clang -O3 \
    -o dhrystone_clang_O3.wasm src/dhry_1.c src/dhry_2.c -I include \
    -Wl,--export=__heap_base -Wl,--export=__data_end
    
/opt/wasi-sdk/bin/clang -O2 \
    -o dhrystone_clang_O2.wasm src/dhry_1.c src/dhry_2.c -I include \
    -Wl,--export=__heap_base -Wl,--export=__data_end
    
/opt/wasi-sdk/bin/clang -O1 \
    -o dhrystone_clang_O1.wasm src/dhry_1.c src/dhry_2.c -I include \
    -Wl,--export=__heap_base -Wl,--export=__data_end
    
/opt/wasi-sdk/bin/clang -O0 \
    -o dhrystone_clang_O0.wasm src/dhry_1.c src/dhry_2.c -I include \
    -Wl,--export=__heap_base -Wl,--export=__data_end
    
emcc \
    -o dhrystone_emcc.wasm src/dhry_1.c src/dhry_2.c -I include \
    -Wl,--export=__heap_base -Wl,--export=__data_end
```
