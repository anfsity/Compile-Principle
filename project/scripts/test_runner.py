#!/usr/bin/env python3
import os
import sys
import subprocess
import glob
import argparse
from pathlib import Path

# ================= 配置区域 =================
#! 输入文件后缀只能为 .sy ，不能为 .c
COMPILER_PATH = os.path.abspath("cmake-build/compiler")

BUILD_DIR = os.path.abspath("debug/test_temp")

TEST_DIRS = [
    os.path.abspath("tests/resources/functional"),
    os.path.abspath("tests/resources/hidden_functional")
]

# ===========================================

class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

def ensure_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

def read_file(path):
    if not os.path.exists(path):
        return ""
    with open(path, 'r') as f:
        return f.read()

def run_cmd(cmd, input_str=None):
    """运行 shell 命令并获取 stdout, stderr, returncode"""
    try:
        result = subprocess.run(
            cmd,
            input=input_str.encode() if input_str else None,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=True,
            timeout=10
        )
        return result.stdout.decode(errors='ignore'), result.stderr.decode(errors='ignore'), result.returncode
    except subprocess.TimeoutExpired:
        return "", "Timeout", -1

def run_test_case(mode, src_file):
    base_name = os.path.basename(src_file)
    name_no_ext = os.path.splitext(base_name)[0]

    # 文件路径
    input_file = src_file.replace(".sy", ".in")
    expect_file = src_file.replace(".sy", ".out")
    
    # 输出文件路径
    output_koopa = os.path.join(BUILD_DIR, f"{name_no_ext}.koopa")
    output_asm = os.path.join(BUILD_DIR, f"{name_no_ext}.S")
    output_obj = os.path.join(BUILD_DIR, f"{name_no_ext}.o")
    output_exe = os.path.join(BUILD_DIR, f"{name_no_ext}")

    # 读取输入和预期输出
    stdin_content = read_file(input_file)
    expected_output = read_file(expect_file).strip()

    print(f"Testing {name_no_ext} ... ", end='', flush=True)

    # 编译阶段 (SysY -> IR/ASM)
    compile_flag = "-koopa" if mode == "koopa" else "-riscv"
    output_target = output_koopa if mode == "koopa" else output_asm
    
    cmd_compile = f"{COMPILER_PATH} {compile_flag} {src_file} -o {output_target}"
    out, err, ret = run_cmd(cmd_compile)
    
    if ret != 0:
        print(f"{Colors.FAIL}Compile Error{Colors.ENDC}")
        print(err)
        return False

    if mode == "koopa":
        # ./compiler -koopa hello.c -o hello.koopa
        # koopac hello.koopa | llc --filetype=obj -o hello.o
        # clang hello.o -L$CDE_LIBRARY_PATH/native -lsysy -o hello
        # ./hello
        cmd_obj = f"koopac {output_koopa} | llc --filetype=obj -o {output_obj}"
        out, err, ret = run_cmd(cmd_obj)
        if ret != 0:
            print(f"{Colors.FAIL}Koopa->Obj Error{Colors.ENDC}")
            print(err)
            return False
        cmd_link = f"clang {output_obj} -L$CDE_LIBRARY_PATH/native -lsysy -o {output_exe}"
        out, err, ret = run_cmd(cmd_link)
        if ret != 0:
            print(f"{Colors.FAIL}Link Error{Colors.ENDC}")
            print(err)
            return False
        cmd_run = f"{output_exe}"
        
    elif mode == "riscv":
        # ./compiler -riscv hello.c -o hello.S
        # clang hello.S -c -o hello.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
        # ld.lld hello.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o hello
        # qemu-riscv32-static hello
        cmd_asm = f"clang {output_asm} -c -o {output_obj} -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32"
        out, err, ret = run_cmd(cmd_asm)
        if ret != 0:
            print(f"{Colors.FAIL}Assemble Error{Colors.ENDC}")
            print(err)
            return False
            
        cmd_link = f"ld.lld {output_obj} -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o {output_exe}"
        out, err, ret = run_cmd(cmd_link)
        if ret != 0:
            print(f"{Colors.FAIL}Link Error{Colors.ENDC}")
            print(err)
            return False
            
        cmd_run = f"qemu-riscv32-static {output_exe}"

    # 先检查输出值是否匹配
    out, err, ret = run_cmd(cmd_run, stdin_content)
    actual_output = out.strip()
    if actual_output != expected_output:
        # 检查返回值和 .out 是否匹配
        actual_with_ret = (actual_output + "\n" + str(ret)).strip()
        if actual_with_ret == expected_output:
            actual_output = actual_with_ret

    if actual_output == expected_output:
        print(f"{Colors.OKGREEN}Pass{Colors.ENDC}")
        return True
    else:
        print(f"{Colors.FAIL}Fail{Colors.ENDC}")
        print(f"{Colors.WARNING}Expected:{Colors.ENDC}\n{expected_output}")
        print(f"{Colors.WARNING}Got:{Colors.ENDC}\n{actual_output}")
        # print(f"Output File: {output_target}")
        return False

def main():
    parser = argparse.ArgumentParser(description="SysY Compiler Test Script")
    parser.add_argument('mode', choices=['koopa', 'riscv'], help="Test mode")
    parser.add_argument('--file', help="Run specific test file", default=None)
    args = parser.parse_args()

    ensure_dir(BUILD_DIR)

    test_files = []
    if args.file:
        test_files.append(args.file)
    else:
        for d in TEST_DIRS:
            if not os.path.exists(d):
                continue
            test_files.extend(glob.glob(os.path.join(d, "*.sy")))
    
    test_files.sort()

    total = 0
    passed = 0
    
    print(f"{Colors.HEADER}Start Testing ({args.mode})...{Colors.ENDC}")
    print(f"Compiler: {COMPILER_PATH}")
    
    for f in test_files:
        total += 1
        if run_test_case(args.mode, f):
            passed += 1
            
    print("="*30)
    color = Colors.OKGREEN if passed == total else Colors.FAIL
    print(f"{color}Result: {passed} / {total} passed.{Colors.ENDC}")
    
    if passed != total:
        sys.exit(1)

if __name__ == "__main__":
    main()