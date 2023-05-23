#!/bin/sh
cd $(echo $0 | grep -oE "^\.(\w*\/)*";)/..
rm -rf targets
echo
echo "\e[1;97mCompile phase\e[0;97m"
echo
echo "The compiliation of x64Debug version"
cmake -H. -Btargets/x64/debug -DWALRUS_ARCH=x64 -DWALRUS_HOST=linux -DWALRUS_MODE=debug -DWALRUS_OUTPUT=shell -GNinja
ninja -Ctargets/x64/debug
echo
echo "The compiliation of x64Release version"
cmake -H. -Btargets/x64/release -DWALRUS_ARCH=x64 -DWALRUS_HOST=linux -DWALRUS_MODE=release -DWALRUS_OUTPUT=shell -GNinja
ninja -Ctargets/x64/release
echo
echo "The compiliation of x86Debug version"
cmake -H. -Btargets/x86/debug -DWALRUS_ARCH=x86 -DWALRUS_HOST=linux -DWALRUS_MODE=debug -DWALRUS_OUTPUT=shell -GNinja
ninja -Ctargets/x86/debug
echo
echo "The compiliation of x86Release version"
cmake -H. -Btargets/x86/release -DWALRUS_ARCH=x86 -DWALRUS_HOST=linux -DWALRUS_MODE=release -DWALRUS_OUTPUT=shell -GNinja
ninja -Ctargets/x86/release
bin_num=0
echo
if [ -f "targets/x64/debug/walrus" ]; then
    echo "\e[1;32mCompiliation of x64Debug version was successful!\e[0;97m"
    bin_num=$((bin_num+1))
else
    echo "\e[1;31mCompiliation of x64Debug version has failed!\e[0;97m"
fi
if [ -f "targets/x64/release/walrus" ]; then
    echo "\e[1;32mCompiliation of x64Release version was successful!\e[0;97m"
    bin_num=$((bin_num+1))
else
    echo "\e[1;31mCompiliation of x64Release version has failed!\e[0;97m"
fi
if [ -f "targets/x86/debug/walrus" ]; then
    echo "\e[1;32mCompiliation of x86Debug version was successful!\e[0;97m"
    bin_num=$((bin_num+1))
else
    echo "\e[1;31mCompiliation of x86Debug version has failed!\e[0;97m"
fi
if [ -f "targets/x86/release/walrus" ]; then
    echo "\e[1;32mCompiliation of x86Release version was successful!\e[0;97m"
    bin_num=$((bin_num+1))
else
    echo "\e[1;31mCompiliation of x86Release version has failed!\e[0;97m"
fi
echo
if [ $bin_num -ge 0 ]; then
    echo "\e[1;97mTest phase\e[0;97m"
    echo
    if [ -f "targets/x64/debug/walrus" ]; then
        result=$(python3 tools/run-tests.py --engine targets/x64/debug/walrus)
        basicTests_total=$(echo $result | grep -oP "basic-tests.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        basicTests_pass=$(echo $result | grep -oP "basic-tests.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        basicTests_fail=$(echo $result | grep -oP "basic-tests.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        wtc_total=$(echo $result | grep -oP "wasm-test-core.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        wtc_pass=$(echo $result | grep -oP "wasm-test-core.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        wtc_fail=$(echo $result | grep -oP "wasm-test-core.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        echo "\e[1;97mX64Debug\e[0;97m"
        echo "Basic Tests"
        echo "\e[0;97m$basicTests_total\e[0;97m"
        echo "\e[0;32m$basicTests_pass\e[0;97m"
        echo "\e[0;31m$basicTests_fail\e[0;97m"
        echo "Wasm-Test-Core"
        echo "\e[0;97m$wtc_total\e[0;97m"
        echo "\e[0;32m$wtc_pass\e[0;97m"
        echo "\e[0;31m$wtc_fail\e[0;97m"
        echo
    fi
    if [ -f "targets/x64/release/walrus" ]; then
        result=$(python3 tools/run-tests.py --engine targets/x64/release/walrus)
        basicTests_total=$(echo $result | grep -oP "basic-tests.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        basicTests_pass=$(echo $result | grep -oP "basic-tests.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        basicTests_fail=$(echo $result | grep -oP "basic-tests.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        wtc_total=$(echo $result | grep -oP "wasm-test-core.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        wtc_pass=$(echo $result | grep -oP "wasm-test-core.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        wtc_fail=$(echo $result | grep -oP "wasm-test-core.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        echo "\e[1;97mX64Release\e[0;97m"
        echo "Basic Tests"
        echo "\e[0;97m$basicTests_total\e[0;97m"
        echo "\e[0;32m$basicTests_pass\e[0;97m"
        echo "\e[0;31m$basicTests_fail\e[0;97m"
        echo "Wasm-Test-Core"
        echo "\e[0;97m$wtc_total\e[0;97m"
        echo "\e[0;32m$wtc_pass\e[0;97m"
        echo "\e[0;31m$wtc_fail\e[0;97m"
        echo
    fi
    if [ -f "targets/x86/debug/walrus" ]; then
        result=$(python3 tools/run-tests.py --engine targets/x86/debug/walrus)
        basicTests_total=$(echo $result | grep -oP "basic-tests.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        basicTests_pass=$(echo $result | grep -oP "basic-tests.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        basicTests_fail=$(echo $result | grep -oP "basic-tests.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        wtc_total=$(echo $result | grep -oP "wasm-test-core.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        wtc_pass=$(echo $result | grep -oP "wasm-test-core.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        wtc_fail=$(echo $result | grep -oP "wasm-test-core.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        echo "\e[1;97mX86Debug\e[0;97m"
        echo "Basic Tests"
        echo "\e[0;97m$basicTests_total\e[0;97m"
        echo "\e[0;32m$basicTests_pass\e[0;97m"
        echo "\e[0;31m$basicTests_fail\e[0;97m"
        echo "Wasm-Test-Core"
        echo "\e[0;97m$wtc_total\e[0;97m"
        echo "\e[0;32m$wtc_pass\e[0;97m"
        echo "\e[0;31m$wtc_fail\e[0;97m"
        echo
    fi
    if [ -f "targets/x86/release/walrus" ]; then
        result=$(python3 tools/run-tests.py --engine targets/x86/release/walrus)
        basicTests_total=$(echo $result | grep -oP "basic-tests.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        basicTests_pass=$(echo $result | grep -oP "basic-tests.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        basicTests_fail=$(echo $result | grep -oP "basic-tests.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        wtc_total=$(echo $result | grep -oP "wasm-test-core.*?TOTAL: [0-9]+" | grep -oP "TOTAL: [0-9]+")
        wtc_pass=$(echo $result | grep -oP "wasm-test-core.*?PASS : [0-9]+" | grep -oP "PASS : [0-9]+")
        wtc_fail=$(echo $result | grep -oP "wasm-test-core.*?FAIL : [0-9]+" | grep -oP "FAIL : [0-9]+")
        echo "\e[1;97mX86Release\e[0;97m"
        echo "Basic Tests"
        echo "\e[0;97m$basicTests_total\e[0;97m"
        echo "\e[0;32m$basicTests_pass\e[0;97m"
        echo "\e[0;31m$basicTests_fail\e[0;97m"
        echo "Wasm-Test-Core"
        echo "\e[0;97m$wtc_total\e[0;97m"
        echo "\e[0;32m$wtc_pass\e[0;97m"
        echo "\e[0;31m$wtc_fail\e[0;97m"
        echo
    fi
fi
