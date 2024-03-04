/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define PROGRAM_MAX_SIZE ((size_t)0x1FF)
#define STACK_MAX_SIZE ((size_t)0xFF)

#define label uint16_t

typedef uint8_t byte;
typedef uint8_t opcode_t;
typedef int64_t I64;
typedef double F64;

enum Opcode {
    halt = 0x00,
    I64Const = 0x01,
    F64Const = 0x02,
    drop = 0x03,
    I64Add = 0x04,
    I64Sub = 0x05,
    I64Mul = 0x06,
    I64Div = 0x07,
    I64Mod = 0x08,
    I64Less = 0x09,
    I64LessEqual = 0x0A,
    I64Equal = 0x0B,
    I64NotEqual = 0x0C,
    I64GreaterEqual = 0x0D,
    I64Greater = 0x0E,
    F64Add = 0x0F,
    F64Sub = 0x10,
    F64Mul = 0x11,
    F64Div = 0x12,
    F64Less = 0x13,
    F64LessEqual = 0x14,
    F64Equal = 0x15,
    F64NotEqual = 0x16,
    F64GreaterEqual = 0x17,
    F64Greater = 0x18,
    jump = 0x19,
    jumpIfTrue = 0x1A,
    jumpIfFalse = 0x1B,
    get = 0x1C,
    set = 0x1D
};

/**
 * @brief return size of bytecode in bytes
*/
size_t getByteCodeSize(opcode_t opcode) {
    switch (opcode) {
        case halt:
        case I64Add:
        case I64Sub:
        case I64Mul:
        case I64Div:
        case I64Mod:
        case I64Less:
        case I64LessEqual:
        case I64Equal:
        case I64NotEqual:
        case I64GreaterEqual:
        case I64Greater:
        case F64Add:
        case F64Sub:
        case F64Mul:
        case F64Div:
        case F64Less:
        case F64LessEqual:
        case F64Equal:
        case F64NotEqual:
        case F64GreaterEqual:
        case F64Greater:
            return sizeof(opcode_t);
        case drop:
            return sizeof(opcode_t) + sizeof(uint8_t);
        case I64Const:
            return sizeof(opcode_t) + sizeof(I64);
        case F64Const:
            return sizeof(opcode_t) + sizeof(F64);
        case jump:
        case jumpIfTrue:
        case jumpIfFalse:
        case get:
        case set:
            return sizeof(opcode_t) + sizeof(uint16_t);
        default:
            return 0;
    }
}

/**
 * @brief create function for opcodes that consists of only the opcode (e.g. halt, I64Add)
*/
#define create_add_Opcode(OPCODE)                       \
void add_##OPCODE(byte* program, size_t* programSize) { \
    program[*programSize] = OPCODE;                     \
    *programSize += getByteCodeSize(OPCODE);            \
}
create_add_Opcode(halt);
create_add_Opcode(I64Add);
create_add_Opcode(I64Sub);
create_add_Opcode(I64Mul);
create_add_Opcode(I64Div);
create_add_Opcode(I64Mod);
create_add_Opcode(I64Less);
create_add_Opcode(I64LessEqual);
create_add_Opcode(I64Equal);
create_add_Opcode(I64NotEqual);
create_add_Opcode(I64GreaterEqual);
create_add_Opcode(I64Greater);
create_add_Opcode(F64Add);
create_add_Opcode(F64Sub);
create_add_Opcode(F64Mul);
create_add_Opcode(F64Div);
create_add_Opcode(F64Less);
create_add_Opcode(F64LessEqual);
create_add_Opcode(F64Equal);
create_add_Opcode(F64NotEqual);
create_add_Opcode(F64GreaterEqual);
create_add_Opcode(F64Greater);
#undef create_add_Opcode

#define create_add_jump(OPCODE)                                              \
void add_##OPCODE(byte* program, size_t* programSize, int16_t jumpValue) {   \
    program[*programSize] = OPCODE;                                          \
    *((uint16_t*)&(program[*programSize + + sizeof(opcode_t)])) = jumpValue; \
    *programSize += getByteCodeSize(OPCODE);                                 \
}
create_add_jump(jump);
create_add_jump(jumpIfFalse);
create_add_jump(jumpIfTrue);
#undef create_add_jump

void add_drop(byte* program, size_t* programSize, uint8_t value) {
    program[*programSize] = drop;
    program[*programSize + sizeof(opcode_t)] = value;
    *programSize += getByteCodeSize(drop);
}

void add_I64Const(byte* program, size_t* programSize, I64 value) {
    program[*programSize] = I64Const;
    *((I64*)&(program[*programSize + sizeof(opcode_t)])) = value;
    *programSize += getByteCodeSize(I64Const);
}

void add_F64Const(byte* program, size_t* programSize, F64 value) {
    program[*programSize] = F64Const;
    *((F64*)&(program[*programSize + sizeof(opcode_t)])) = value;
    *programSize += getByteCodeSize(F64Const);
}

void add_get(byte* program, size_t* programSize, uint16_t value) {
    program[*programSize] = get;
    *((uint16_t*)&(program[*programSize + sizeof(opcode_t)])) = value;
    *programSize += getByteCodeSize(get);
}

void add_set(byte* program, size_t* programSize, uint16_t value) {
    program[*programSize] = set;
    *((uint16_t*)&(program[*programSize + sizeof(opcode_t)])) = value;
    *programSize += getByteCodeSize(set);
}


#define generateOperationCase(NAME, TYPE, OPERATOR) \
case NAME:                                          \
    ((TYPE*)stack)[*stackSize - 2] =                \
        ((TYPE*)stack)[*stackSize - 2]              \
        OPERATOR ((TYPE*)stack)[*stackSize - 1];    \
    (*stackSize)--;                                 \
    programCounter += getByteCodeSize(NAME);        \
    break;

void interpret(byte* program, const size_t programSize, uint64_t* stack, size_t* stackSize) {
    uint16_t programCounter = 0;
    while (true) {
        switch (program[programCounter]) {
            case halt:
                return;
            case I64Const:
                ((I64*)stack)[*stackSize] = *((I64*)&(program[programCounter + sizeof(opcode_t)]));
                (*stackSize)++;
                programCounter += getByteCodeSize(I64Const);
                break;
            case F64Const:
                ((F64*)stack)[*stackSize] = *((F64*)&(program[programCounter + sizeof(opcode_t)]));
                (*stackSize)++;
                programCounter += getByteCodeSize(F64Const);
                break;
            case drop:
                *stackSize -= program[programCounter + sizeof(opcode_t)];
                programCounter += getByteCodeSize(drop);
                break;
            // Integer operations
            //   Arithmetic Operations
            generateOperationCase(I64Add, I64, +);
            generateOperationCase(I64Sub, I64, -);
            generateOperationCase(I64Mul, I64, *);
            generateOperationCase(I64Div, I64, /);
            generateOperationCase(I64Mod, I64, %);
            //   Logical Operations
            generateOperationCase(I64Less, I64, <);
            generateOperationCase(I64LessEqual, I64, <=);
            generateOperationCase(I64Equal, I64, ==);
            generateOperationCase(I64NotEqual, I64, !=);
            generateOperationCase(I64GreaterEqual, I64, >=);
            generateOperationCase(I64Greater, I64, >);
            // end of Integer operations
            // Float operations
            //   Arithmetic Operations
            generateOperationCase(F64Add, F64, +);
            generateOperationCase(F64Sub, F64, -);
            generateOperationCase(F64Mul, F64, *);
            generateOperationCase(F64Div, F64, /);
            generateOperationCase(F64Less, F64, <);
            //   Logical Operations
            generateOperationCase(F64LessEqual, F64, <=);
            generateOperationCase(F64Equal, F64, ==);
            generateOperationCase(F64NotEqual, F64, !=);
            generateOperationCase(F64GreaterEqual, F64, >=);
            generateOperationCase(F64Greater, F64, >);
            // end of Float operations
            case jump:
                programCounter = *((int16_t*)&(program[programCounter + 1]));
                break;
            case jumpIfTrue:
                if (stack[--(*stackSize)]) {
                    programCounter = *((int16_t*)&(program[programCounter + 1]));
                } else {
                    programCounter += getByteCodeSize(jumpIfTrue);
                }
                break;
            case jumpIfFalse:
                if (stack[--(*stackSize)]) {
                    programCounter += getByteCodeSize(jumpIfFalse);
                } else {
                    programCounter = *((int16_t*)&(program[programCounter + 1]));
                }
                break;
            case get:
                stack[*stackSize] = stack[*stackSize - 1 - *((uint16_t*)&(program[programCounter + 1]))];
                (*stackSize)++;
                programCounter += getByteCodeSize(get);
                break;
            case set:
                stack[*stackSize - 1 - *((uint16_t*)&(program[programCounter + 1]))] = stack[*stackSize - 1];
                if (*((uint16_t*)&(program[programCounter + 1])) > 0) (*stackSize)--;
                programCounter += getByteCodeSize(set);
                break;
            default:
                return;
        }
    }
}
#undef generateOperationCase

label getAddressOfNextCommand(byte* program, size_t* programSize) {
    return *programSize;
}

void set_jump(byte* program, uint16_t addrOfJump, int16_t jumpValue) {
    *((uint16_t*)&(program[addrOfJump + sizeof(opcode_t)])) = jumpValue;
}

I64 runtime() {
    byte program[PROGRAM_MAX_SIZE]; // getNthPrime(I64 n);
    size_t programSize = 0;

    add_I64Const(program, &programSize, 1);
    label while0_start = getAddressOfNextCommand(program, &programSize);
    add_I64Const(program, &programSize, 0);
    add_get(program, &programSize, 2);
    add_I64GreaterEqual(program, &programSize);
    label jump_to_be_set_0 = getAddressOfNextCommand(program, &programSize);
    add_jumpIfTrue(program, &programSize, 0);
    add_I64Const(program, &programSize, 1);
    add_I64Add(program, &programSize);
    add_I64Const(program, &programSize, 0);
    label while1_start = getAddressOfNextCommand(program, &programSize);
    add_get(program, &programSize, 0);
    add_I64Const(program, &programSize, 1);
    add_I64Add(program, &programSize);
    add_get(program, &programSize, 0);
    add_I64Mul(program, &programSize);
    add_get(program, &programSize, 2);
    add_I64Greater(program, &programSize);
    label jump_to_be_set_1 = getAddressOfNextCommand(program, &programSize);
    add_jumpIfTrue(program, &programSize, 0);
    add_I64Const(program, &programSize, 1);
    add_I64Add(program, &programSize);
    add_jump(program, &programSize, while1_start);
    label while1_end = getAddressOfNextCommand(program, &programSize);
    add_I64Const(program, &programSize, 2);
    label while2_start = getAddressOfNextCommand(program, &programSize);
    add_get(program, &programSize, 0);
    add_get(program, &programSize, 2);
    add_I64Greater(program, &programSize);
    label jump_to_be_set_2 = getAddressOfNextCommand(program, &programSize);
    add_jumpIfTrue(program, &programSize, 0);
    // if0_start
    add_get(program, &programSize, 2);
    add_get(program, &programSize, 1);
    add_I64Mod(program, &programSize);
    label jump_to_be_set_3 = getAddressOfNextCommand(program, &programSize);
    add_jumpIfTrue(program, &programSize, 0);
    // if0_trueBranch
    add_drop(program, &programSize, 2);
    add_jump(program, &programSize, while0_start);
    label if0_end = getAddressOfNextCommand(program, &programSize);
    add_I64Const(program, &programSize, 1);
    add_I64Add(program, &programSize);
    add_jump(program, &programSize, while2_start);
    label while2_end = getAddressOfNextCommand(program, &programSize);
    add_drop(program, &programSize, 2);
    add_get(program, &programSize, 1);
    add_I64Const(program, &programSize, 1);
    add_I64Sub(program, &programSize);
    add_set(program, &programSize, 2);
    add_jump(program, &programSize, while0_start);
    label while0_end = getAddressOfNextCommand(program, &programSize);
    add_set(program, &programSize, 1);
    add_halt(program, &programSize);

    set_jump(program, jump_to_be_set_0, while0_end);
    set_jump(program, jump_to_be_set_1, while1_end);
    set_jump(program, jump_to_be_set_2, while2_end);
    set_jump(program, jump_to_be_set_3, if0_end);

    uint64_t stack[STACK_MAX_SIZE];
    size_t stackSize = 0;
    stack[stackSize++] = 3000;
    interpret(program, programSize, stack, &stackSize);
    return ((I64*)stack)[0];
}

int main() {
    printf("%lld\n", runtime());
    return 0;
}
